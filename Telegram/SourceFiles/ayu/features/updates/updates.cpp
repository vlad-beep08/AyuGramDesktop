#include "ayu/features/updates/updates.h"

#include "base/timer.h"
#include "core/application.h"
#include "settings.h"
#include "ui/layers/generic_box.h"
#include "ui/layers/show.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "window/window_controller.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#ifndef CHICKENGRAM_BUILD
#define CHICKENGRAM_BUILD 0
#endif

namespace AyuFeatures::Updates {
namespace {

constexpr auto kLatestUrl = "https://api.github.com/repos/"
	"vlad-beep08/ChickengramWindows/releases/latest";
constexpr auto kTagPrefix = "build-";
constexpr auto kAssetName = "Chickengram.exe";
constexpr auto kUserAgent = "Chickengram";
constexpr auto kFirstCheckDelay = 30 * crl::time(1000);
constexpr auto kCheckInterval = 6 * 60 * 60 * crl::time(1000);
constexpr auto kRequestTimeout = 20 * 1000;

struct Release {
	int build = 0;
	QString notes;
	QString url;
	QByteArray sha256;
};

enum class CheckStatus {
	Failed,
	UpToDate,
	Available,
};

struct CheckResult {
	CheckStatus status = CheckStatus::Failed;
	Release release;
};

struct DownloadState {
	QPointer<QNetworkReply> reply;
	QFile file;
	QCryptographicHash hash{ QCryptographicHash::Sha256 };
	bool started = false;
	bool cancelled = false;
	bool writeFailed = false;
};

int Offered = 0;

[[nodiscard]] QNetworkAccessManager *Manager() {
	static const auto result = new QNetworkAccessManager(
		QCoreApplication::instance());
	return result;
}

[[nodiscard]] QNetworkRequest MakeRequest(const QString &url) {
	auto result = QNetworkRequest(QUrl(url));
	result.setRawHeader("User-Agent", kUserAgent);
	result.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
	return result;
}

[[nodiscard]] QString SiblingPath(const QString &suffix) {
	const auto info = QFileInfo(QCoreApplication::applicationFilePath());
	return info.absolutePath() + '/' + info.completeBaseName() + suffix;
}

[[nodiscard]] std::optional<Release> ParseRelease(const QByteArray &data) {
	const auto document = QJsonDocument::fromJson(data);
	if (!document.isObject()) {
		return std::nullopt;
	}
	const auto root = document.object();
	const auto tag = root.value(u"tag_name"_q).toString();
	const auto prefix = QString::fromLatin1(kTagPrefix);
	if (!tag.startsWith(prefix)) {
		return std::nullopt;
	}
	auto result = Release{ .build = tag.mid(prefix.size()).toInt() };
	static const auto shaLine = QRegularExpression(
		u"SHA256:\\s*([0-9a-fA-F]{64})"_q);
	const auto body = root.value(u"body"_q).toString();
	const auto match = shaLine.match(body);
	if (match.hasMatch()) {
		result.sha256 = match.captured(1).toLower().toLatin1();
	}
	result.notes = QString(body).remove(shaLine).trimmed();
	const auto name = QString::fromLatin1(kAssetName);
	for (const auto &asset : root.value(u"assets"_q).toArray()) {
		const auto object = asset.toObject();
		if (object.value(u"name"_q).toString() == name) {
			result.url = object.value(
				u"browser_download_url"_q).toString();
		}
	}
	if (result.build <= 0
		|| result.url.isEmpty()
		|| result.sha256.isEmpty()) {
		return std::nullopt;
	}
	return result;
}

void RequestLatest(Fn<void(CheckResult)> done) {
	auto request = MakeRequest(QString::fromLatin1(kLatestUrl));
	request.setRawHeader("Accept", "application/vnd.github+json");
	request.setTransferTimeout(kRequestTimeout);
	const auto reply = Manager()->get(request);
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		reply->deleteLater();
		const auto code = reply->attribute(
			QNetworkRequest::HttpStatusCodeAttribute).toInt();
		if (code == 404) {
			done({ .status = CheckStatus::UpToDate });
			return;
		} else if (reply->error() != QNetworkReply::NoError) {
			LOG(("Chickengram Updates: Check failed, %1."
				).arg(reply->errorString()));
			done({});
			return;
		}
		const auto release = ParseRelease(reply->readAll());
		if (!release) {
			LOG(("Chickengram Updates: Bad latest release."));
			done({});
			return;
		}
		done({
			.status = (release->build > CurrentBuild())
				? CheckStatus::Available
				: CheckStatus::UpToDate,
			.release = *release,
		});
	});
}

void CancelDownload(const std::shared_ptr<DownloadState> &state) {
	state->cancelled = true;
	if (const auto reply = state->reply.data()) {
		reply->abort();
	}
}

void StartDownload(
		const std::shared_ptr<DownloadState> &state,
		const Release &release,
		Fn<void(int)> progress,
		Fn<void(QString)> done) {
	const auto path = SiblingPath(u".update.exe"_q);
	state->file.setFileName(path);
	state->writeFailed = false;
	state->hash.reset();
	if (!state->file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		done(u"Нет доступа к папке программы."_q);
		return;
	}
	const auto reply = Manager()->get(MakeRequest(release.url));
	state->reply = reply;
	const auto write = [=] {
		const auto chunk = reply->readAll();
		state->hash.addData(chunk);
		if (state->file.write(chunk) != chunk.size()) {
			state->writeFailed = true;
		}
	};
	QObject::connect(reply, &QNetworkReply::readyRead, write);
	QObject::connect(reply, &QNetworkReply::downloadProgress, [=](
			qint64 received,
			qint64 total) {
		if (total > 0 && !state->cancelled) {
			progress(int(received * 100 / total));
		}
	});
	const auto sha256 = release.sha256;
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		reply->deleteLater();
		write();
		state->file.close();
		const auto failed = state->cancelled
			|| state->writeFailed
			|| (reply->error() != QNetworkReply::NoError);
		if (failed || state->hash.result().toHex() != sha256) {
			QFile::remove(path);
			if (!state->cancelled) {
				done(failed
					? u"Не удалось скачать обновление."_q
					: u"Скачанный файл повреждён, попробуйте ещё раз."_q);
			}
			return;
		}
		done(QString());
	});
}

[[nodiscard]] bool Install() {
	const auto current = QCoreApplication::applicationFilePath();
	const auto backup = SiblingPath(u".old.exe"_q);
	if (QFile::exists(backup) && !QFile::remove(backup)) {
		return false;
	} else if (!QFile::rename(current, backup)) {
		return false;
	} else if (!QFile::rename(SiblingPath(u".update.exe"_q), current)) {
		QFile::rename(backup, current);
		return false;
	}
	return true;
}

void ShowOffer(std::shared_ptr<Ui::Show> show, Release release) {
	show->showBox(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(u"Обновление Чикенграма"_q));
		box->setWidth(st::boxWideWidth);

		const auto current = CurrentBuild();
		auto text = u"Доступна новая сборка %1."_q.arg(release.build);
		if (current > 0) {
			text += u" Сейчас у вас сборка %1."_q.arg(current);
		}
		if (!release.notes.isEmpty()) {
			text += u"\n\n"_q + release.notes;
		}
		box->addRow(object_ptr<Ui::FlatLabel>(box, text, st::boxLabel));
		const auto status = box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			u"После обновления Чикенграм перезапустится, "
			"прежняя версия останется рядом как .old.exe."_q,
			st::boxDividerLabel));

		const auto state = std::make_shared<DownloadState>();
		box->lifetime().add([=] {
			CancelDownload(state);
		});
		const auto progress = crl::guard(box, [=](int percent) {
			status->setText(u"Загрузка… %1%"_q.arg(percent));
		});
		const auto done = crl::guard(box, [=](QString error) {
			state->started = false;
			if (!error.isEmpty()) {
				status->setText(error);
				return;
			} else if (!Install()) {
				status->setText(
					u"Не удалось заменить файл программы."_q);
				return;
			}
			cSetRestarting(true);
			Core::Quit();
		});
		box->addButton(rpl::single(u"Обновить"_q), [=] {
			if (state->started) {
				return;
			}
			state->started = true;
			state->cancelled = false;
			status->setText(u"Загрузка… 0%"_q);
			StartDownload(state, release, progress, done);
		});
		box->addButton(rpl::single(u"Позже"_q), [=] {
			box->closeBox();
		});
	}));
}

void CheckInBackground() {
	RequestLatest([](CheckResult result) {
		if (result.status != CheckStatus::Available
			|| Offered >= result.release.build
			|| Core::App().passcodeLocked()) {
			return;
		}
		const auto window = Core::App().activePrimaryWindow();
		if (!window) {
			return;
		}
		Offered = result.release.build;
		ShowOffer(window->uiShow(), result.release);
	});
}

} // namespace

void Start() {
	if (!CurrentBuild()) {
		return;
	}
	static auto first = std::unique_ptr<base::Timer>();
	static auto regular = std::unique_ptr<base::Timer>();
	if (first) {
		return;
	}
	first = std::make_unique<base::Timer>(CheckInBackground);
	first->callOnce(kFirstCheckDelay);
	regular = std::make_unique<base::Timer>(CheckInBackground);
	regular->callEach(kCheckInterval);
}

void CheckNow(std::shared_ptr<Ui::Show> show) {
	show->showToast(u"Проверяю обновления…"_q);
	RequestLatest([=](CheckResult result) {
		if (!show->valid()) {
			return;
		}
		switch (result.status) {
		case CheckStatus::Failed:
			show->showToast(u"Не удалось проверить обновления."_q);
			break;
		case CheckStatus::UpToDate:
			show->showToast(u"У вас последняя версия Чикенграма."_q);
			break;
		case CheckStatus::Available:
			ShowOffer(show, result.release);
			break;
		}
	});
}

int CurrentBuild() {
	return CHICKENGRAM_BUILD;
}

} // namespace AyuFeatures::Updates
