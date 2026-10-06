// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/data_transfer/data_transfer.h"

#include "lang_auto.h"
#include "ayu/libs/json.hpp"
#include "ayu/ui/settings/settings_ayu_utils.h"
#include "core/file_utilities.h"
#include "settings.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSaveFile>

namespace AyuFeatures::DataTransfer {
namespace {

constexpr auto kFormatVersion = 1;

[[nodiscard]] std::string FormatName() {
	return "ayugram-extras-backup";
}

[[nodiscard]] QString DataFolder() {
	return cWorkingDir() + u"tdata/"_q;
}

[[nodiscard]] bool IsAllowedName(const QString &name) {
	static const auto regex = QRegularExpression(
		u"^(ayu_settings|ayu/[a-z_]+)\\.json$"_q);
	return regex.match(name).hasMatch();
}

[[nodiscard]] QStringList CollectNames() {
	auto result = QStringList{ u"ayu_settings.json"_q };
	const auto folder = QDir(DataFolder() + u"ayu"_q);
	const auto files = folder.entryList(
		{ u"*.json"_q },
		QDir::Files,
		QDir::Name);
	for (const auto &file : files) {
		const auto name = u"ayu/"_q + file;
		if (IsAllowedName(name)) {
			result.push_back(name);
		}
	}
	return result;
}

[[nodiscard]] QByteArray ReadFile(const QString &path) {
	auto file = QFile(path);
	return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

[[nodiscard]] bool WriteFile(const QString &path, const QByteArray &bytes) {
	QDir().mkpath(QFileInfo(path).absolutePath());
	auto file = QSaveFile(path);
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}
	file.write(bytes);
	return file.commit();
}

void DoExport(
		not_null<Window::SessionController*> controller,
		const QString &path) {
	auto files = nlohmann::json::object();
	for (const auto &name : CollectNames()) {
		const auto bytes = ReadFile(DataFolder() + name);
		if (!bytes.isEmpty()) {
			files[name.toStdString()] = bytes.toStdString();
		}
	}
	auto json = nlohmann::json::object();
	json["format"] = FormatName();
	json["version"] = kFormatVersion;
	json["files"] = std::move(files);
	const auto dumped = json.dump(2);
	const auto ok = WriteFile(
		path,
		QByteArray(dumped.data(), int(dumped.size())));
	controller->showToast(ok
		? tr::ayu_DataExported(tr::now)
		: tr::ayu_DataTransferFailed(tr::now));
}

void DoImport(
		not_null<Window::SessionController*> controller,
		const QByteArray &bytes) {
	auto json = nlohmann::json();
	try {
		json = nlohmann::json::parse(
			bytes.constData(),
			bytes.constData() + bytes.size());
	} catch (...) {
		controller->showToast(tr::ayu_DataTransferFailed(tr::now));
		return;
	}
	const auto files = json.find("files");
	if (!json.is_object()
		|| json.value("format", std::string()) != FormatName()
		|| files == json.end()
		|| !files->is_object()) {
		controller->showToast(tr::ayu_DataTransferFailed(tr::now));
		return;
	}
	auto written = 0;
	for (const auto &[key, value] : files->items()) {
		const auto name = QString::fromStdString(key);
		if (!IsAllowedName(name) || !value.is_string()) {
			continue;
		}
		const auto content = value.get<std::string>();
		const auto ok = WriteFile(
			DataFolder() + name,
			QByteArray(content.data(), int(content.size())));
		written += ok ? 1 : 0;
	}
	if (!written) {
		controller->showToast(tr::ayu_DataTransferFailed(tr::now));
		return;
	}
	Settings::ShowRestartPrompt(controller);
}

} // namespace

void Export(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	FileDialog::GetWritePath(
		controller->widget().get(),
		tr::ayu_DataExport(tr::now),
		u"JSON (*.json);;"_q + FileDialog::AllFilesFilter(),
		filedialogDefaultName(u"ayugram_backup"_q, u".json"_q),
		[=](const QString &path) {
			if (const auto strong = weak.get(); strong && !path.isEmpty()) {
				DoExport(strong, path);
			}
		});
}

void Import(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	FileDialog::GetOpenPath(
		controller->widget().get(),
		tr::ayu_DataImport(tr::now),
		u"JSON (*.json);;"_q + FileDialog::AllFilesFilter(),
		[=](FileDialog::OpenResult &&result) {
			const auto strong = weak.get();
			if (!strong) {
				return;
			} else if (!result.remoteContent.isEmpty()) {
				DoImport(strong, result.remoteContent);
			} else if (!result.paths.isEmpty()) {
				DoImport(strong, ReadFile(result.paths.front()));
			}
		});
}

} // namespace AyuFeatures::DataTransfer
