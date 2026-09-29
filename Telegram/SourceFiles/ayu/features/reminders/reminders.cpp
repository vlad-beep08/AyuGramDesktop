// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/reminders/reminders.h"

#include "lang_auto.h"
#include "ayu/features/local_store/local_store.h"
#include "base/timer.h"
#include "base/unixtime.h"
#include "core/application.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "lang/lang_keys.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "mainwindow.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/menu/menu_add_action_callback.h"
#include "ui/widgets/menu/menu_add_action_callback_factory.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"

#include <QtWidgets/QApplication>

namespace AyuFeatures::Reminders {
namespace {

constexpr auto kCheckInterval = 15 * crl::time(1000);
constexpr auto kForgetAfter = TimeId(7 * 24 * 60 * 60);
constexpr auto kMorningHour = 9;

struct Reminder {
	uint64 userId = 0;
	PeerId peerId = 0;
	MsgId msgId = 0;
	TimeId when = 0;
	QString preview;
};

[[nodiscard]] QString StoreName() {
	return u"reminders"_q;
}

[[nodiscard]] std::vector<Reminder> Load() {
	auto result = std::vector<Reminder>();
	const auto json = LocalStore::Read(StoreName());
	const auto list = json.find("list");
	if (list == json.end() || !list->is_array()) {
		return result;
	}
	for (const auto &entry : *list) {
		if (!entry.is_object()) {
			continue;
		}
		result.push_back({
			.userId = entry.value("user", uint64()),
			.peerId = PeerId(PeerIdHelper(entry.value("peer", uint64()))),
			.msgId = MsgId(entry.value("msg", int64())),
			.when = entry.value("when", TimeId()),
			.preview = QString::fromStdString(
				entry.value("preview", std::string())),
		});
	}
	return result;
}

[[nodiscard]] std::vector<Reminder> &All() {
	static auto result = Load();
	return result;
}

void Save() {
	auto list = nlohmann::json::array();
	for (const auto &reminder : All()) {
		list.push_back({
			{ "user", reminder.userId },
			{ "peer", reminder.peerId.value },
			{ "msg", reminder.msgId.bare },
			{ "when", reminder.when },
			{ "preview", reminder.preview.toStdString() },
		});
	}
	auto json = nlohmann::json::object();
	json["list"] = std::move(list);
	LocalStore::Write(StoreName(), json);
}

[[nodiscard]] Main::Session *FindSession(uint64 userId) {
	for (const auto &[index, account] : Core::App().domain().accounts()) {
		if (const auto session = account->maybeSession()) {
			if (session->userId().bare == userId) {
				return session;
			}
		}
	}
	return nullptr;
}

void FillReminderBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> window,
		Reminder reminder) {
	box->setTitle(tr::ayu_ReminderTitle());
	box->setWidth(st::boxWideWidth);

	auto text = reminder.preview.isEmpty()
		? tr::ayu_BookmarksNoText(tr::now)
		: reminder.preview;
	const auto peer = window->session().data().peerLoaded(reminder.peerId);
	if (peer) {
		text = peer->name() + u"\n\n"_q + text;
	}
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(text),
		st::boxLabel));

	box->addButton(tr::ayu_ReminderOpen(), [=] {
		box->closeBox();
		window->showPeerHistory(
			reminder.peerId,
			Window::SectionShow::Way::ClearStack,
			reminder.msgId);
	});
	box->addButton(tr::lng_close(), [=] {
		box->closeBox();
	});
}

[[nodiscard]] bool Notify(const Reminder &reminder) {
	const auto session = FindSession(reminder.userId);
	const auto window = session ? session->tryResolveWindow() : nullptr;
	if (!window) {
		return false;
	}
	window->show(Box(FillReminderBox, window, reminder));
	QApplication::alert(window->widget().get());
	return true;
}

void Check() {
	const auto now = base::unixtime::now();
	auto &all = All();
	auto changed = false;
	for (auto i = begin(all); i != end(all);) {
		if (i->when > now) {
			++i;
			continue;
		}
		const auto reminder = *i;
		if (Notify(reminder) || (now - reminder.when > kForgetAfter)) {
			i = all.erase(i);
			changed = true;
		} else {
			++i;
		}
	}
	if (changed) {
		Save();
	}
}

[[nodiscard]] TimeId TomorrowMorning() {
	const auto tomorrow = QDate::currentDate().addDays(1);
	return TimeId(QDateTime(
		tomorrow,
		QTime(kMorningHour, 0)).toSecsSinceEpoch());
}

void Add(not_null<HistoryItem*> item, TimeId when) {
	const auto session = &item->history()->session();
	All().push_back({
		.userId = session->userId().bare,
		.peerId = item->history()->peer->id,
		.msgId = item->id,
		.when = when,
		.preview = LocalStore::PreviewText(item),
	});
	Save();
	if (const auto window = session->tryResolveWindow()) {
		window->showToast(tr::ayu_ReminderSet(
			tr::now,
			lt_date,
			langDateTime(base::unixtime::parse(when))));
	}
}

} // namespace

void Start() {
	static auto timer = std::unique_ptr<base::Timer>();
	if (timer) {
		return;
	}
	timer = std::make_unique<base::Timer>(Check);
	timer->callEach(kCheckInterval);
}

void AddMenuAction(
		not_null<Ui::PopupMenu*> menu,
		not_null<HistoryItem*> item) {
	if (!item->isRegular()) {
		return;
	}
	const auto history = item->history();
	const auto itemId = item->fullId();
	const auto remind = [=](TimeId when) {
		if (const auto item = history->owner().message(itemId)) {
			Add(item, when);
		}
	};
	const auto after = [=](TimeId seconds) {
		return [=] {
			remind(base::unixtime::now() + seconds);
		};
	};
	Ui::Menu::CreateAddActionCallback(menu)({
		.text = tr::ayu_RemindMe(tr::now),
		.handler = nullptr,
		.icon = &st::menuIconSchedule,
		.fillSubmenu = [=](not_null<Ui::PopupMenu*> submenu) {
			submenu->addAction(
				tr::ayu_RemindIn30Minutes(tr::now),
				after(30 * 60));
			submenu->addAction(
				tr::ayu_RemindIn1Hour(tr::now),
				after(60 * 60));
			submenu->addAction(
				tr::ayu_RemindIn3Hours(tr::now),
				after(3 * 60 * 60));
			submenu->addAction(
				tr::ayu_RemindTomorrowMorning(tr::now),
				[=] { remind(TomorrowMorning()); });
		},
	});
}

} // namespace AyuFeatures::Reminders
