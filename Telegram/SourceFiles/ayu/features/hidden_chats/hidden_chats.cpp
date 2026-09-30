// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/hidden_chats/hidden_chats.h"

#include "lang_auto.h"
#include "ayu/features/local_store/local_store.h"
#include "base/random.h"
#include "core/application.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "dialogs/dialogs_key.h"
#include "history/history.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/password_input.h"
#include "ui/widgets/labels.h"
#include "ui/ui_utility.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

#include <QtCore/QCryptographicHash>

namespace AyuFeatures::HiddenChats {
namespace {

constexpr auto kHashRounds = 20000;
constexpr auto kSaltSize = 16;
constexpr auto kMinPinLength = 4;
constexpr auto kMaxPinLength = 12;

struct State {
	QByteArray salt;
	QByteArray hash;
	std::map<uint64, base::flat_set<uint64>> chats;
	bool locked = true;
};

[[nodiscard]] QString StoreName() {
	return u"hidden_chats"_q;
}

[[nodiscard]] QByteArray FromHex(const nlohmann::json &json, const char *key) {
	return QByteArray::fromHex(
		QByteArray::fromStdString(json.value(key, std::string())));
}

[[nodiscard]] State Load() {
	auto result = State();
	const auto json = LocalStore::Read(StoreName());
	result.salt = FromHex(json, "salt");
	result.hash = FromHex(json, "hash");
	const auto chats = json.find("chats");
	if (chats == json.end() || !chats->is_object()) {
		return result;
	}
	for (const auto &[user, list] : chats->items()) {
		if (!list.is_array()) {
			continue;
		}
		auto userId = uint64();
		try {
			userId = std::stoull(user);
		} catch (...) {
			continue;
		}
		auto &peers = result.chats[userId];
		for (const auto &peer : list) {
			if (peer.is_number_unsigned()) {
				peers.emplace(peer.get<uint64>());
			}
		}
	}
	return result;
}

[[nodiscard]] State &Current() {
	static auto result = Load();
	return result;
}

void Save() {
	const auto &state = Current();
	auto chats = nlohmann::json::object();
	for (const auto &[user, peers] : state.chats) {
		if (peers.empty()) {
			continue;
		}
		auto list = nlohmann::json::array();
		for (const auto peer : peers) {
			list.push_back(peer);
		}
		chats[std::to_string(user)] = std::move(list);
	}
	auto json = nlohmann::json::object();
	json["salt"] = state.salt.toHex().toStdString();
	json["hash"] = state.hash.toHex().toStdString();
	json["chats"] = std::move(chats);
	LocalStore::Write(StoreName(), json);
}

[[nodiscard]] QByteArray HashPin(const QString &pin, const QByteArray &salt) {
	auto result = salt + pin.toUtf8();
	for (auto i = 0; i != kHashRounds; ++i) {
		result = QCryptographicHash::hash(
			result + salt,
			QCryptographicHash::Sha256);
	}
	return result;
}

[[nodiscard]] bool HasPin() {
	return !Current().hash.isEmpty();
}

[[nodiscard]] bool CheckPin(const QString &pin) {
	const auto &state = Current();
	return HasPin() && (HashPin(pin, state.salt) == state.hash);
}

void SetPin(const QString &pin) {
	auto &state = Current();
	state.salt = QByteArray(kSaltSize, char(0));
	base::RandomFill(state.salt.data(), state.salt.size());
	state.hash = HashPin(pin, state.salt);
	Save();
}

[[nodiscard]] bool IsValidPin(const QString &pin) {
	if (pin.size() < kMinPinLength || pin.size() > kMaxPinLength) {
		return false;
	}
	return std::all_of(pin.begin(), pin.end(), [](QChar ch) {
		return ch.isDigit();
	});
}

void RefreshLists() {
	const auto &state = Current();
	for (const auto &[index, account] : Core::App().domain().accounts()) {
		const auto session = account->maybeSession();
		if (!session) {
			continue;
		}
		const auto i = state.chats.find(session->userId().bare);
		if (i == end(state.chats)) {
			continue;
		}
		for (const auto peer : i->second) {
			const auto peerId = PeerId(PeerIdHelper(peer));
			if (const auto history = session->data().historyLoaded(peerId)) {
				history->updateChatListExistence();
			}
		}
		if (!state.locked) {
			continue;
		}
		for (const auto window : session->windows()) {
			const auto history = window->activeChatCurrent().history();
			if (history && IsConcealed(history)) {
				window->clearSectionStack();
			}
		}
	}
}

void SetHidden(not_null<History*> history, bool hidden) {
	auto &peers = Current().chats[history->session().userId().bare];
	if (hidden) {
		peers.emplace(history->peer->id.value);
	} else {
		peers.remove(history->peer->id.value);
	}
	Save();
	history->updateChatListExistence();
}

[[nodiscard]] Ui::PasswordInput *AddPinField(
		not_null<Ui::GenericBox*> box,
		rpl::producer<QString> placeholder) {
	const auto &st = st::defaultInputField;
	auto container = object_ptr<Ui::RpWidget>(box);
	container->resize(container->width(), st.heightMin);
	const auto field = Ui::CreateChild<Ui::PasswordInput>(
		container.data(),
		st,
		std::move(placeholder));
	container->widthValue(
	) | rpl::on_next([=](int width) {
		field->resize(width, field->height());
		field->moveToLeft(0, 0);
	}, container->lifetime());
	box->addRow(std::move(container));
	return field;
}

void FillPinBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller,
		Fn<void()> done) {
	box->setTitle(tr::ayu_HiddenChatsPinTitle());
	box->setWidth(st::boxWidth);

	const auto hadPin = HasPin();
	const auto current = hadPin
		? AddPinField(box, tr::ayu_HiddenChatsCurrentPin())
		: nullptr;
	const auto pin = AddPinField(box, tr::ayu_HiddenChatsNewPin());
	const auto repeat = AddPinField(box, tr::ayu_HiddenChatsRepeatPin());
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		tr::ayu_HiddenChatsPinAbout(),
		st::boxDividerLabel));

	box->setFocusCallback([=] {
		(current ? current : pin)->setFocusFast();
	});
	const auto submit = [=] {
		if (current && !CheckPin(current->getLastText())) {
			current->showError();
			return;
		} else if (!IsValidPin(pin->getLastText())) {
			pin->showError();
			return;
		} else if (pin->getLastText() != repeat->getLastText()) {
			repeat->showError();
			return;
		}
		SetPin(pin->getLastText());
		box->closeBox();
		controller->showToast(tr::ayu_HiddenChatsPinSaved(tr::now));
		if (done) {
			done();
		}
	};
	for (const auto field : { current, pin, repeat }) {
		if (field) {
			QObject::connect(
				field,
				&Ui::MaskedInputField::submitted,
				submit);
		}
	}
	box->addButton(tr::lng_settings_save(), submit);
	box->addButton(tr::lng_cancel(), [=] {
		box->closeBox();
	});
}

void FillUnlockBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller) {
	box->setTitle(tr::ayu_HiddenChatsUnlockTitle());
	box->setWidth(st::boxWidth);

	const auto pin = AddPinField(box, tr::ayu_HiddenChatsEnterPin());
	box->setFocusCallback([=] {
		pin->setFocusFast();
	});
	const auto submit = [=] {
		if (!CheckPin(pin->getLastText())) {
			pin->showError();
			return;
		}
		Current().locked = false;
		RefreshLists();
		box->closeBox();
		controller->showToast(tr::ayu_HiddenChatsUnlocked(tr::now));
	};
	QObject::connect(pin, &Ui::MaskedInputField::submitted, submit);
	box->addButton(tr::ayu_HiddenChatsUnlockButton(), submit);
	box->addButton(tr::lng_cancel(), [=] {
		box->closeBox();
	});
}

} // namespace

bool IsHidden(not_null<const History*> history) {
	const auto &chats = Current().chats;
	const auto i = chats.find(history->session().userId().bare);
	return (i != end(chats)) && i->second.contains(history->peer->id.value);
}

bool IsConcealed(not_null<const History*> history) {
	return Current().locked && IsHidden(history);
}

bool IsLocked() {
	return Current().locked;
}

void Toggle(
		not_null<Window::SessionController*> controller,
		not_null<History*> history) {
	if (IsHidden(history)) {
		SetHidden(history, false);
		controller->showToast(tr::ayu_HiddenChatsShownAgain(tr::now));
		return;
	} else if (!HasPin()) {
		const auto weak = base::make_weak(controller);
		controller->show(Box(FillPinBox, controller, [=] {
			if (const auto strong = weak.get()) {
				Toggle(strong, history);
			}
		}));
		return;
	}
	SetHidden(history, true);
	RefreshLists();
	controller->showToast(tr::ayu_HiddenChatsHidden(tr::now));
}

void RequestUnlock(not_null<Window::SessionController*> controller) {
	if (!IsLocked()) {
		Lock();
		controller->showToast(tr::ayu_HiddenChatsLocked(tr::now));
	} else if (!HasPin()) {
		controller->show(Box(FillPinBox, controller, Fn<void()>()));
	} else {
		controller->show(Box(FillUnlockBox, controller));
	}
}

void ShowPinBox(not_null<Window::SessionController*> controller) {
	controller->show(Box(FillPinBox, controller, Fn<void()>()));
}

void Lock() {
	if (Current().locked) {
		return;
	}
	Current().locked = true;
	RefreshLists();
}

void Panic() {
	Current().locked = true;
	RefreshLists();
}

} // namespace AyuFeatures::HiddenChats
