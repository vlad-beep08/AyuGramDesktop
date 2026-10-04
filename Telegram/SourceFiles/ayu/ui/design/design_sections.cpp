// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_sections.h"

#include "apiwrap.h"
#include "data/data_chat_filters.h"
#include "data/data_premium_limits.h"
#include "data/data_session.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "window/window_session_controller.h"

namespace AyuDesign {
namespace {

using Flag = Data::ChatFilter::Flag;
using Flags = Data::ChatFilter::Flags;

struct Section {
	QString title;
	Flags flags;
};

[[nodiscard]] std::vector<Section> Sections() {
	return {
		{ tr::ayu_SectionPersonal(tr::now), Flag::Contacts | Flag::NonContacts },
		{ tr::ayu_SectionGroups(tr::now), Flags(Flag::Groups) },
		{ tr::ayu_SectionChannels(tr::now), Flags(Flag::Channels) },
		{ tr::ayu_SectionBots(tr::now), Flags(Flag::Bots) },
	};
}

[[nodiscard]] bool Exists(
		const std::vector<Data::ChatFilter> &list,
		Flags flags) {
	return ranges::any_of(list, [&](const Data::ChatFilter &filter) {
		return filter.id()
			&& ((filter.flags() & Flag::RulesMask) == flags)
			&& filter.always().empty()
			&& filter.never().empty();
	});
}

} // namespace

bool HasMissingWebSections(not_null<Main::Session*> session) {
	const auto &list = session->data().chatsFilters().list();
	for (const auto &section : Sections()) {
		if (!Exists(list, section.flags)) {
			return true;
		}
	}
	return false;
}

void AddWebSections(not_null<Window::SessionController*> controller) {
	const auto session = &controller->session();
	auto &filters = session->data().chatsFilters();
	const auto limit = Data::PremiumLimits(session).dialogFiltersCurrent();
	auto used = base::flat_set<FilterId>();
	auto count = 0;
	for (const auto &filter : filters.list()) {
		used.emplace(filter.id());
		if (filter.id()) {
			++count;
		}
	}
	auto nextId = FilterId(1);
	auto added = 0;
	for (const auto &section : Sections()) {
		if (Exists(filters.list(), section.flags)) {
			continue;
		} else if (count >= limit) {
			controller->showToast(tr::ayu_SectionsLimit(tr::now));
			break;
		}
		do {
			++nextId;
		} while (used.contains(nextId));
		used.emplace(nextId);
		const auto filter = Data::ChatFilter(
			nextId,
			Data::ChatFilterTitle{ .text = TextWithEntities{ section.title } },
			QString(),
			std::nullopt,
			section.flags,
			{},
			{},
			{});
		const auto tl = filter.tl(nextId);
		filters.apply(MTP_updateDialogFilter(
			MTP_flags(MTPDupdateDialogFilter::Flag::f_filter),
			MTP_int(nextId),
			tl));
		session->api().request(MTPmessages_UpdateDialogFilter(
			MTP_flags(MTPmessages_UpdateDialogFilter::Flag::f_filter),
			MTP_int(nextId),
			tl
		)).send();
		++count;
		++added;
	}
	if (added) {
		controller->showToast(tr::ayu_SectionsAdded(tr::now));
	}
}

} // namespace AyuDesign
