// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/undo_send/undo_send.h"

#include "lang_auto.h"
#include "apiwrap.h"
#include "ayu/ayu_settings.h"
#include "base/call_delayed.h"
#include "chat_helpers/compose/compose_show.h"
#include "data/data_changes.h"
#include "history/history.h"
#include "lang/lang_text_entity.h"
#include "main/main_session.h"
#include "ui/toast/toast.h"

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

namespace AyuFeatures::UndoSend {
namespace {

struct Pending {
	Api::MessageToSend message;
	base::weak_ptr<Ui::Toast::Instance> toast;
};

uint64 LastId = 0;
std::map<uint64, std::unique_ptr<Pending>> PendingMessages;

[[nodiscard]] std::unique_ptr<Pending> Take(uint64 id) {
	const auto i = PendingMessages.find(id);
	if (i == end(PendingMessages)) {
		return nullptr;
	}
	auto result = std::move(i->second);
	PendingMessages.erase(i);
	return result;
}

void Send(uint64 id) {
	const auto pending = Take(id);
	if (!pending) {
		return;
	}
	const auto history = pending->message.action.history;
	history->session().api().sendMessage(std::move(pending->message));
	history->session().changes().historyUpdated(
		history,
		Data::HistoryUpdate::Flag::MessageSent);
}

void Cancel(
		uint64 id,
		const std::shared_ptr<ChatHelpers::Show> &show,
		const Fn<bool(const TextWithTags&)> &restore) {
	const auto pending = Take(id);
	if (!pending) {
		return;
	}
	if (const auto toast = pending->toast.get()) {
		toast->hideAnimated();
	}
	const auto &text = pending->message.textWithTags;
	if (restore && restore(text)) {
		show->showToast(tr::ayu_UndoSendCancelled(tr::now));
		return;
	}
	QGuiApplication::clipboard()->setText(text.text);
	show->showToast(tr::ayu_UndoSendCancelledCopied(tr::now));
}

} // namespace

bool ShouldDelay(const Api::SendOptions &options, bool ephemeral) {
	return !ephemeral
		&& !options.scheduled
		&& !options.shortcutId
		&& (AyuSettings::getInstance().undoSendDelay() > 0);
}

void Delay(
		std::shared_ptr<ChatHelpers::Show> show,
		Api::MessageToSend &&message,
		Fn<bool(const TextWithTags&)> restore) {
	const auto seconds = AyuSettings::getInstance().undoSendDelay();
	const auto delay = seconds * crl::time(1000);
	const auto id = ++LastId;
	const auto session = &message.action.history->session();

	message.action.clearDraft = false;
	auto pending = std::make_unique<Pending>(Pending{
		.message = std::move(message),
	});
	const auto raw = pending.get();
	PendingMessages.emplace(id, std::move(pending));

	auto text = tr::ayu_UndoSendToast(
		tr::now,
		lt_count,
		seconds,
		tr::marked);
	text.append(' ').append(tr::link(tr::ayu_UndoSendButton(tr::now)));
	raw->toast = show->showToast({
		.text = std::move(text),
		.filter = [=](const auto &...) {
			Cancel(id, show, restore);
			return false;
		},
		.duration = delay,
	});
	base::call_delayed(delay, session, [=] {
		Send(id);
	});
}

} // namespace AyuFeatures::UndoSend
