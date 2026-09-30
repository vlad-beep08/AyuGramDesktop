// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/command_palette/command_palette.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/features/bookmarks/bookmarks.h"
#include "ayu/features/contact_card/contact_card.h"
#include "ayu/features/data_transfer/data_transfer.h"
#include "ayu/features/drafts_history/drafts_history.h"
#include "ayu/features/hidden_chats/hidden_chats.h"
#include "ayu/features/keyword_alerts/keyword_alerts.h"
#include "ayu/ui/settings/settings_main.h"
#include "dialogs/dialogs_key.h"
#include "history/history.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"

namespace AyuFeatures::CommandPalette {
namespace {

struct Command {
	QString title;
	Fn<void()> run;
};

[[nodiscard]] std::vector<Command> Commands(
		not_null<Window::SessionController*> controller) {
	auto result = std::vector<Command>();
	const auto add = [&](QString title, Fn<void()> run) {
		result.push_back({ std::move(title), std::move(run) });
	};
	const auto session = &controller->session();
	const auto history = controller->activeChatCurrent().history();

	add(tr::ayu_PaletteAyuSettings(tr::now), [=] {
		controller->showSettings(Settings::AyuMainId());
	});
	add(tr::ayu_PaletteGhostMode(tr::now), [=] {
		auto &ghost = AyuSettings::ghost(session);
		ghost.setGhostModeEnabled(!ghost.isGhostModeActive());
		controller->showToast(ghost.isGhostModeActive()
			? tr::ayu_PaletteGhostModeOn(tr::now)
			: tr::ayu_PaletteGhostModeOff(tr::now));
	});
	add(tr::ayu_AllBookmarksMenuText(tr::now), [=] {
		Bookmarks::ShowBox(controller);
	});
	if (history) {
		add(tr::ayu_BookmarksMenuText(tr::now), [=] {
			Bookmarks::ShowBox(controller, history->peer->id);
		});
		add(tr::ayu_DraftsHistoryMenuText(tr::now), [=] {
			DraftsHistory::ShowBox(controller, history);
		});
		add(tr::ayu_ContactCardMenuText(tr::now), [=] {
			ContactCard::Show(controller, history->peer);
		});
		add((HiddenChats::IsHidden(history)
			? tr::ayu_HiddenChatsUnhideChat(tr::now)
			: tr::ayu_HiddenChatsHideChat(tr::now)), [=] {
			HiddenChats::Toggle(controller, history);
		});
	}
	add((HiddenChats::IsLocked()
		? tr::ayu_HiddenChatsShow(tr::now)
		: tr::ayu_HiddenChatsLock(tr::now)), [=] {
		HiddenChats::RequestUnlock(controller);
	});
	add(tr::ayu_PalettePanic(tr::now), [] {
		HiddenChats::Panic();
	});
	add(tr::ayu_KeywordAlertsTitle(tr::now), [=] {
		KeywordAlerts::ShowEditBox(controller);
	});
	add(tr::ayu_DataExport(tr::now), [=] {
		DataTransfer::Export(controller);
	});
	add(tr::ayu_DataImport(tr::now), [=] {
		DataTransfer::Import(controller);
	});
	return result;
}

void FillBox(
		not_null<Ui::GenericBox*> box,
		not_null<Window::SessionController*> controller) {
	box->setTitle(tr::ayu_PaletteTitle());
	box->setWidth(st::boxWideWidth);

	const auto field = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::defaultInputField,
		tr::ayu_PalettePlaceholder()));

	struct Row {
		QString search;
		Fn<void()> run;
		not_null<Ui::SlideWrap<Ui::SettingsButton>*> wrap;
	};
	const auto rows = std::make_shared<std::vector<Row>>();
	const auto content = box->verticalLayout();
	for (auto &command : Commands(controller)) {
		const auto wrap = content->add(
			object_ptr<Ui::SlideWrap<Ui::SettingsButton>>(
				content,
				object_ptr<Ui::SettingsButton>(
					content,
					rpl::single(command.title),
					st::settingsButtonNoIcon)));
		wrap->toggle(true, anim::type::instant);
		const auto run = command.run;
		wrap->entity()->setClickedCallback([=] {
			box->closeBox();
			run();
		});
		rows->push_back({
			.search = command.title.toLower(),
			.run = run,
			.wrap = wrap,
		});
	}

	const auto filter = [=] {
		const auto query = field->getLastText().trimmed().toLower();
		for (const auto &row : *rows) {
			row.wrap->toggle(
				query.isEmpty() || row.search.contains(query),
				anim::type::instant);
		}
	};
	field->changes(
	) | rpl::on_next(filter, field->lifetime());
	field->submits(
	) | rpl::on_next([=](Qt::KeyboardModifiers) {
		for (const auto &row : *rows) {
			if (row.wrap->toggled()) {
				const auto run = row.run;
				box->closeBox();
				run();
				return;
			}
		}
	}, field->lifetime());

	box->setFocusCallback([=] {
		field->setFocusFast();
	});
	box->addButton(tr::lng_close(), [=] {
		box->closeBox();
	});
}

} // namespace

void Show(not_null<Window::SessionController*> controller) {
	controller->show(Box(FillBox, controller));
}

} // namespace AyuFeatures::CommandPalette
