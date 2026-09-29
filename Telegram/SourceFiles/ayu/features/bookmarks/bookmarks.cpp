// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/bookmarks/bookmarks.h"

#include "lang_auto.h"
#include "ayu/features/local_store/local_store.h"
#include "base/unixtime.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"

namespace AyuFeatures::Bookmarks {
namespace {

using Entries = std::vector<Entry>;

[[nodiscard]] QString StoreName() {
	return u"bookmarks"_q;
}

[[nodiscard]] std::map<uint64, Entries> Load() {
	auto result = std::map<uint64, Entries>();
	const auto json = LocalStore::Read(StoreName());
	for (const auto &[user, list] : json.items()) {
		if (!list.is_array()) {
			continue;
		}
		auto userId = uint64();
		try {
			userId = std::stoull(user);
		} catch (...) {
			continue;
		}
		auto &entries = result[userId];
		for (const auto &entry : list) {
			if (!entry.is_object()) {
				continue;
			}
			entries.push_back({
				.peerId = PeerId(PeerIdHelper(entry.value("peer", uint64()))),
				.msgId = MsgId(entry.value("msg", int64())),
				.added = entry.value("added", TimeId()),
				.preview = QString::fromStdString(
					entry.value("preview", std::string())),
			});
		}
	}
	return result;
}

[[nodiscard]] std::map<uint64, Entries> &All() {
	static auto result = Load();
	return result;
}

void Save() {
	auto json = nlohmann::json::object();
	for (const auto &[user, entries] : All()) {
		if (entries.empty()) {
			continue;
		}
		auto list = nlohmann::json::array();
		for (const auto &entry : entries) {
			list.push_back({
				{ "peer", entry.peerId.value },
				{ "msg", entry.msgId.bare },
				{ "added", entry.added },
				{ "preview", entry.preview.toStdString() },
			});
		}
		json[std::to_string(user)] = std::move(list);
	}
	LocalStore::Write(StoreName(), json);
}

[[nodiscard]] Entries &ForSession(not_null<Main::Session*> session) {
	return All()[session->userId().bare];
}

[[nodiscard]] auto Find(Entries &entries, PeerId peerId, MsgId msgId) {
	return ranges::find_if(entries, [&](const Entry &entry) {
		return (entry.peerId == peerId) && (entry.msgId == msgId);
	});
}

void FillBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller,
		PeerId peerId) {
	const auto session = &controller->session();
	box->setTitle(tr::ayu_BookmarksTitle());
	box->setWidth(st::boxWideWidth);

	auto entries = List(session, peerId);
	ranges::reverse(entries);
	if (entries.empty()) {
		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			tr::ayu_BookmarksEmpty(),
			st::boxLabel));
	}
	const auto content = box->verticalLayout();
	for (const auto &entry : entries) {
		auto title = entry.preview.isEmpty()
			? tr::ayu_BookmarksNoText(tr::now)
			: entry.preview;
		if (!peerId) {
			if (const auto peer = session->data().peerLoaded(entry.peerId)) {
				title = peer->name() + u": "_q + title;
			}
		}
		const auto date = QLocale().toString(
			base::unixtime::parse(entry.added).date(),
			QLocale::ShortFormat);
		const auto button = Settings::AddButtonWithLabel(
			content,
			rpl::single(title),
			rpl::single(date),
			st::settingsButtonNoIcon);
		const auto jumpPeerId = entry.peerId;
		const auto jumpMsgId = entry.msgId;
		button->setClickedCallback([=] {
			box->closeBox();
			controller->showPeerHistory(
				jumpPeerId,
				Window::SectionShow::Way::ClearStack,
				jumpMsgId);
		});
	}

	if (!entries.empty()) {
		box->addLeftButton(tr::ayu_BookmarksClear(), [=] {
			Clear(session, peerId);
			box->closeBox();
		}, st::attentionBoxButton);
	}
	box->addButton(tr::lng_close(), [=] {
		box->closeBox();
	});
}

} // namespace

bool Has(not_null<HistoryItem*> item) {
	auto &entries = ForSession(&item->history()->session());
	return Find(entries, item->history()->peer->id, item->id)
		!= end(entries);
}

bool Toggle(not_null<HistoryItem*> item) {
	auto &entries = ForSession(&item->history()->session());
	const auto peerId = item->history()->peer->id;
	const auto i = Find(entries, peerId, item->id);
	const auto added = (i == end(entries));
	if (added) {
		entries.push_back({
			.peerId = peerId,
			.msgId = item->id,
			.added = base::unixtime::now(),
			.preview = LocalStore::PreviewText(item),
		});
	} else {
		entries.erase(i);
	}
	Save();
	return added;
}

std::vector<Entry> List(not_null<Main::Session*> session, PeerId peerId) {
	const auto &entries = ForSession(session);
	if (!peerId) {
		return entries;
	}
	return entries | ranges::views::filter([&](const Entry &entry) {
		return (entry.peerId == peerId);
	}) | ranges::to_vector;
}

void Clear(not_null<Main::Session*> session, PeerId peerId) {
	auto &entries = ForSession(session);
	if (!peerId) {
		entries.clear();
	} else {
		entries.erase(
			ranges::remove(entries, peerId, &Entry::peerId),
			end(entries));
	}
	Save();
}

void ShowBox(
		not_null<Window::SessionController*> controller,
		PeerId peerId) {
	controller->show(Box(FillBox, controller, peerId));
}

} // namespace AyuFeatures::Bookmarks
