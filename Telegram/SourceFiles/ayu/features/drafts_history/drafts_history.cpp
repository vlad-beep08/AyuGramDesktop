// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/drafts_history/drafts_history.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/features/local_store/local_store.h"
#include "base/timer.h"
#include "base/unixtime.h"
#include "data/data_drafts.h"
#include "data/data_peer.h"
#include "history/history.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

namespace AyuFeatures::DraftsHistory {
namespace {

constexpr auto kMaxPerChat = 30;
constexpr auto kSaveDelay = 3 * crl::time(1000);
constexpr auto kButtonTextLength = 80;

struct Entry {
	QString text;
	TimeId date = 0;
};

using Key = std::pair<uint64, uint64>;
using Entries = std::vector<Entry>;

[[nodiscard]] QString StoreName() {
	return u"drafts_history"_q;
}

[[nodiscard]] std::map<Key, Entries> Load() {
	auto result = std::map<Key, Entries>();
	const auto json = LocalStore::Read(StoreName());
	for (const auto &[user, peers] : json.items()) {
		if (!peers.is_object()) {
			continue;
		}
		for (const auto &[peer, list] : peers.items()) {
			if (!list.is_array()) {
				continue;
			}
			auto key = Key();
			try {
				key = Key(std::stoull(user), std::stoull(peer));
			} catch (...) {
				continue;
			}
			auto &entries = result[key];
			for (const auto &entry : list) {
				if (!entry.is_object()) {
					continue;
				}
				entries.push_back({
					.text = QString::fromStdString(
						entry.value("text", std::string())),
					.date = entry.value("date", TimeId()),
				});
			}
		}
	}
	return result;
}

[[nodiscard]] std::map<Key, Entries> &All() {
	static auto result = Load();
	return result;
}

void Save() {
	auto json = nlohmann::json::object();
	for (const auto &[key, entries] : All()) {
		if (entries.empty()) {
			continue;
		}
		auto list = nlohmann::json::array();
		for (const auto &entry : entries) {
			list.push_back({
				{ "text", entry.text.toStdString() },
				{ "date", entry.date },
			});
		}
		json[std::to_string(key.first)][std::to_string(key.second)]
			= std::move(list);
	}
	LocalStore::Write(StoreName(), json);
}

void SaveDelayed() {
	static auto timer = std::unique_ptr<base::Timer>();
	if (!timer) {
		timer = std::make_unique<base::Timer>(Save);
	}
	if (!timer->isActive()) {
		timer->callOnce(kSaveDelay);
	}
}

[[nodiscard]] Key KeyFor(not_null<History*> history) {
	return Key(history->session().userId().bare, history->peer->id.value);
}

void RememberText(not_null<History*> history, const QString &text) {
	if (text.isEmpty()) {
		return;
	}
	auto &entries = All()[KeyFor(history)];
	const auto now = base::unixtime::now();
	if (!entries.empty()) {
		auto &last = entries.back();
		if (last.text == text || last.text.startsWith(text)) {
			return;
		} else if (text.startsWith(last.text)) {
			last.text = text;
			last.date = now;
			SaveDelayed();
			return;
		}
	}
	entries.erase(
		ranges::remove(entries, text, &Entry::text),
		end(entries));
	entries.push_back({ .text = text, .date = now });
	if (int(entries.size()) > kMaxPerChat) {
		entries.erase(begin(entries));
	}
	SaveDelayed();
}

void FillBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller,
		not_null<History*> history) {
	box->setTitle(tr::ayu_DraftsHistoryTitle());
	box->setWidth(st::boxWideWidth);

	const auto i = All().find(KeyFor(history));
	auto entries = (i != end(All())) ? i->second : Entries();
	ranges::reverse(entries);

	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		(entries.empty()
			? tr::ayu_DraftsHistoryEmpty()
			: tr::ayu_DraftsHistoryAbout()),
		st::boxDividerLabel));

	const auto content = box->verticalLayout();
	for (const auto &entry : entries) {
		auto title = entry.text.simplified();
		if (title.size() > kButtonTextLength) {
			title = title.mid(0, kButtonTextLength - 1) + QChar(0x2026);
		}
		const auto date = QLocale().toString(
			base::unixtime::parse(entry.date),
			QLocale::ShortFormat);
		const auto button = Settings::AddButtonWithLabel(
			content,
			rpl::single(title),
			rpl::single(date),
			st::settingsButtonNoIcon);
		const auto text = entry.text;
		button->setClickedCallback([=] {
			QGuiApplication::clipboard()->setText(text);
			controller->showToast(tr::ayu_DraftsHistoryCopied(tr::now));
			box->closeBox();
		});
	}

	if (!entries.empty()) {
		box->addLeftButton(tr::ayu_DraftsHistoryClear(), [=] {
			All().erase(KeyFor(history));
			Save();
			box->closeBox();
		}, st::attentionBoxButton);
	}
	box->addButton(tr::lng_close(), [=] {
		box->closeBox();
	});
}

} // namespace

void Remember(not_null<History*> history) {
	if (!AyuSettings::getInstance().saveDraftsHistory()) {
		return;
	}
	for (const auto &[key, draft] : history->draftsMap()) {
		if (key.isLocal() && draft) {
			RememberText(history, draft->textWithTags.text.trimmed());
		}
	}
}

void Forget(not_null<History*> history, const QString &sentText) {
	const auto i = All().find(KeyFor(history));
	if (i == end(All())) {
		return;
	}
	const auto sent = sentText.trimmed();
	auto &entries = i->second;
	const auto was = entries.size();
	entries.erase(ranges::remove_if(entries, [&](const Entry &entry) {
		return sent.startsWith(entry.text);
	}), end(entries));
	if (entries.size() != was) {
		SaveDelayed();
	}
}

bool HasEntries(not_null<History*> history) {
	const auto i = All().find(KeyFor(history));
	return (i != end(All())) && !i->second.empty();
}

void ShowBox(
		not_null<Window::SessionController*> controller,
		not_null<History*> history) {
	controller->show(Box(FillBox, controller, history));
}

} // namespace AyuFeatures::DraftsHistory
