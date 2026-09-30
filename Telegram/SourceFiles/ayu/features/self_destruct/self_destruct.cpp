// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/self_destruct/self_destruct.h"

#include "lang_auto.h"

#include "base/call_delayed.h"
#include "data/data_histories.h"
#include "data/data_session.h"
#include "history/history_item.h"
#include "main/main_session.h"

namespace AyuFeatures::SelfDestruct {
namespace {

constexpr auto kRetryDelay = 2 * crl::time(1000);
constexpr auto kMaxRetries = 30;

struct SessionState {
	std::map<uint64, FullMsgId> tracked;
	uint64 lastToken = 0;
	bool subscribed = false;
};

[[nodiscard]] SessionState &StateFor(not_null<Main::Session*> session) {
	static auto states = std::map<not_null<Main::Session*>, SessionState>();
	auto &result = states[session];
	if (!result.subscribed) {
		result.subscribed = true;
		session->data().itemIdChanged(
		) | rpl::on_next([=](const Data::Session::IdChange &change) {
			auto &state = StateFor(session);
			for (auto &[token, id] : state.tracked) {
				if (id.peer == change.newId.peer && id.msg == change.oldId) {
					id = change.newId;
				}
			}
		}, session->lifetime());
		session->lifetime().add([=] {
			states.erase(session);
		});
	}
	return result;
}

void TryDelete(not_null<Main::Session*> session, uint64 token, int retries) {
	auto &state = StateFor(session);
	const auto i = state.tracked.find(token);
	if (i == end(state.tracked)) {
		return;
	}
	const auto id = i->second;
	const auto item = session->data().message(id);
	if (!item) {
		state.tracked.erase(i);
		return;
	} else if (!item->isRegular()) {
		if (retries > 0) {
			base::call_delayed(kRetryDelay, session, [=] {
				TryDelete(session, token, retries - 1);
			});
		} else {
			state.tracked.erase(i);
		}
		return;
	}
	state.tracked.erase(i);
	session->data().histories().deleteMessages({ id }, true);
	session->data().sendHistoryChangeNotifications();
}

} // namespace

void Track(not_null<Main::Session*> session, FullMsgId id, int seconds) {
	auto &state = StateFor(session);
	const auto token = ++state.lastToken;
	state.tracked.emplace(token, id);
	base::call_delayed(seconds * crl::time(1000), session, [=] {
		TryDelete(session, token, kMaxRetries);
	});
}

QString DurationText(int seconds) {
	constexpr auto kMinute = 60;
	constexpr auto kHour = 60 * kMinute;
	if (seconds >= kHour && !(seconds % kHour)) {
		return tr::ayu_DurationHours(tr::now, lt_count, seconds / kHour);
	} else if (seconds >= kMinute && !(seconds % kMinute)) {
		return tr::ayu_DurationMinutes(tr::now, lt_count, seconds / kMinute);
	}
	return tr::ayu_DurationSeconds(tr::now, lt_count, seconds);
}

std::vector<int> DurationOptions() {
	return { 10, 30, 60, 300, 900, 3600 };
}

} // namespace AyuFeatures::SelfDestruct
