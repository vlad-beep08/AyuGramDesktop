// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/contact_card/contact_card.h"

#include "lang_auto.h"
#include "ayu/features/local_store/local_store.h"
#include "base/unixtime.h"
#include "data/data_birthday.h"
#include "data/data_peer.h"
#include "data/data_user.h"
#include "history/history.h"
#include "main/main_session.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

namespace AyuFeatures::ContactCard {
namespace {

constexpr auto kSecondsInDay = 24 * 60 * 60;

struct Card {
	QString notes;
	QString tags;
	TimeId lastSent = 0;
};

using Key = std::pair<uint64, uint64>;

[[nodiscard]] QString StoreName() {
	return u"contact_cards"_q;
}

[[nodiscard]] std::map<Key, Card> Load() {
	auto result = std::map<Key, Card>();
	const auto json = LocalStore::Read(StoreName());
	for (const auto &[user, peers] : json.items()) {
		if (!peers.is_object()) {
			continue;
		}
		for (const auto &[peer, card] : peers.items()) {
			if (!card.is_object()) {
				continue;
			}
			auto key = Key();
			try {
				key = Key(std::stoull(user), std::stoull(peer));
			} catch (...) {
				continue;
			}
			result[key] = Card{
				.notes = QString::fromStdString(
					card.value("notes", std::string())),
				.tags = QString::fromStdString(
					card.value("tags", std::string())),
				.lastSent = card.value("lastSent", TimeId()),
			};
		}
	}
	return result;
}

[[nodiscard]] std::map<Key, Card> &All() {
	static auto result = Load();
	return result;
}

void Save() {
	auto json = nlohmann::json::object();
	for (const auto &[key, card] : All()) {
		json[std::to_string(key.first)][std::to_string(key.second)] = {
			{ "notes", card.notes.toStdString() },
			{ "tags", card.tags.toStdString() },
			{ "lastSent", card.lastSent },
		};
	}
	LocalStore::Write(StoreName(), json);
}

[[nodiscard]] Key KeyFor(not_null<PeerData*> peer) {
	return Key(peer->session().userId().bare, peer->id.value);
}

[[nodiscard]] QString BirthdayText(const Data::Birthday &birthday) {
	auto result = QString::number(birthday.day()).rightJustified(2, u'0');
	result.append(u'.');
	result.append(QString::number(birthday.month()).rightJustified(2, u'0'));
	if (birthday.year()) {
		result.append(u'.');
		result.append(QString::number(birthday.year()));
	}
	return result;
}

[[nodiscard]] QString LastSentText(TimeId lastSent) {
	if (!lastSent) {
		return tr::ayu_ContactCardLastSentNever(tr::now);
	}
	const auto days = (base::unixtime::now() - lastSent) / kSecondsInDay;
	return days
		? tr::ayu_ContactCardLastSentDays(tr::now, lt_count, days)
		: tr::ayu_ContactCardLastSentToday(tr::now);
}

void FillBox(
		not_null<Ui::GenericBox*> box,
		not_null<PeerData*> peer) {
	box->setTitle(rpl::single(peer->name()));
	box->setWidth(st::boxWideWidth);

	const auto key = KeyFor(peer);
	const auto card = All()[key];

	auto info = QStringList();
	if (const auto username = peer->username(); !username.isEmpty()) {
		info.push_back(u"@"_q + username);
	}
	if (const auto user = peer->asUser()) {
		if (const auto birthday = user->birthday(); birthday.valid()) {
			info.push_back(tr::ayu_ContactCardBirthday(
				tr::now,
				lt_date,
				BirthdayText(birthday)));
		}
	}
	info.push_back(LastSentText(card.lastSent));
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		rpl::single(info.join(u'\n')),
		st::boxLabel));
	box->addSkip(st::boxPadding.bottom());

	const auto notes = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::newGroupDescription,
		Ui::InputField::Mode::MultiLine,
		tr::ayu_ContactCardNotes(),
		card.notes));
	const auto tags = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		tr::ayu_ContactCardTags(),
		card.tags));

	box->setFocusCallback([=] {
		notes->setFocusFast();
	});
	box->addButton(tr::lng_settings_save(), [=] {
		auto &saved = All()[key];
		saved.notes = notes->getLastText().trimmed();
		saved.tags = tags->getLastText().trimmed();
		Save();
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] {
		box->closeBox();
	});
}

} // namespace

void RememberSent(not_null<History*> history) {
	if (!history->peer->isUser()) {
		return;
	}
	All()[KeyFor(history->peer)].lastSent = base::unixtime::now();
	Save();
}

void Show(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	controller->show(Box(FillBox, peer));
}

} // namespace AyuFeatures::ContactCard
