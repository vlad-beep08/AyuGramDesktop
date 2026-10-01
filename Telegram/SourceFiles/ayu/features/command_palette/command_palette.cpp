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
#include "ayu/ui/design/design_widgets.h"
#include "base/event_filter.h"
#include "core/application.h"
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
#include "window/window_controller.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"
#include "styles/style_widgets.h"

#include <QtGui/QKeyEvent>
#include <QtWidgets/QAbstractSpinBox>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QTextEdit>

namespace AyuFeatures::CommandPalette {
namespace {

struct Command {
	QString title;
	Fn<void()> run;
	QString hotkey;
};

[[nodiscard]] std::vector<Command> Commands(
		not_null<Window::SessionController*> controller) {
	auto result = std::vector<Command>();
	const auto add = [&](
			QString title,
			Fn<void()> run,
			QString hotkey = QString()) {
		result.push_back({ std::move(title), std::move(run), std::move(hotkey) });
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
	}, u"Ctrl+Shift+H"_q);
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
		not_null<Ui::SlideWrap<AyuDesign::ListRow>*> wrap;
	};
	struct State {
		std::vector<Row> rows;
		int selected = -1;
	};
	const auto state = std::make_shared<State>();
	const auto content = box->verticalLayout();
	for (auto &command : Commands(controller)) {
		const auto wrap = content->add(
			object_ptr<Ui::SlideWrap<AyuDesign::ListRow>>(
				content,
				object_ptr<AyuDesign::ListRow>(
					content,
					command.title,
					command.hotkey)));
		wrap->toggle(true, anim::type::instant);
		const auto run = command.run;
		wrap->entity()->setClickedCallback([=] {
			box->closeBox();
			run();
		});
		state->rows.push_back({
			.search = command.title.toLower(),
			.run = run,
			.wrap = wrap,
		});
	}

	const auto visible = [=] {
		auto result = std::vector<int>();
		for (auto i = 0; i != int(state->rows.size()); ++i) {
			if (state->rows[i].wrap->toggled()) {
				result.push_back(i);
			}
		}
		return result;
	};
	const auto select = [=](int index) {
		const auto count = int(state->rows.size());
		if (state->selected >= 0 && state->selected < count) {
			state->rows[state->selected].wrap->entity()->setSelected(false);
		}
		state->selected = index;
		if (index >= 0 && index < count) {
			state->rows[index].wrap->entity()->setSelected(true);
		}
	};
	const auto selectFirst = [=] {
		const auto indices = visible();
		select(indices.empty() ? -1 : indices.front());
	};
	const auto move = [=](int delta) {
		const auto indices = visible();
		if (indices.empty()) {
			return;
		}
		const auto i = ranges::find(indices, state->selected);
		const auto position = (i != end(indices))
			? int(i - begin(indices)) + delta
			: (delta > 0 ? 0 : int(indices.size()) - 1);
		select(indices[std::clamp(position, 0, int(indices.size()) - 1)]);
	};
	const auto runSelected = [=] {
		const auto indices = visible();
		const auto i = ranges::find(indices, state->selected);
		const auto index = (i != end(indices))
			? state->selected
			: (indices.empty() ? -1 : indices.front());
		if (index < 0) {
			return;
		}
		const auto run = state->rows[index].run;
		box->closeBox();
		run();
	};

	field->changes(
	) | rpl::on_next([=] {
		const auto query = field->getLastText().trimmed().toLower();
		for (const auto &row : state->rows) {
			row.wrap->toggle(
				query.isEmpty() || row.search.contains(query),
				anim::type::instant);
		}
		selectFirst();
	}, field->lifetime());
	field->submits(
	) | rpl::on_next([=](Qt::KeyboardModifiers) {
		runSelected();
	}, field->lifetime());
	base::install_event_filter(field->rawTextEdit(), [=](
			not_null<QEvent*> event) {
		if (event->type() != QEvent::KeyPress) {
			return base::EventFilterResult::Continue;
		}
		const auto key = static_cast<QKeyEvent*>(event.get())->key();
		if (key == Qt::Key_Up || key == Qt::Key_Down) {
			move(key == Qt::Key_Up ? -1 : 1);
			return base::EventFilterResult::Cancel;
		}
		return base::EventFilterResult::Continue;
	});
	selectFirst();

	box->setFocusCallback([=] {
		field->setFocusFast();
	});
	box->addButton(tr::lng_close(), [=] {
		box->closeBox();
	});
}

[[nodiscard]] bool IsTextInput(QWidget *widget) {
	return qobject_cast<QTextEdit*>(widget)
		|| qobject_cast<QLineEdit*>(widget)
		|| qobject_cast<QPlainTextEdit*>(widget)
		|| qobject_cast<QAbstractSpinBox*>(widget);
}

} // namespace

void Show(not_null<Window::SessionController*> controller) {
	controller->show(Box(FillBox, controller));
}

void InstallGlobalHotkey() {
	static auto installed = false;
	if (installed) {
		return;
	}
	installed = true;
	base::install_event_filter(QCoreApplication::instance(), [](
			not_null<QEvent*> event) {
		if (event->type() != QEvent::KeyPress) {
			return base::EventFilterResult::Continue;
		}
		const auto key = static_cast<QKeyEvent*>(event.get());
		const auto modifiers = key->modifiers() & ~Qt::KeypadModifier;
		if (key->key() != Qt::Key_K
			|| modifiers != Qt::ControlModifier
			|| key->isAutoRepeat()
			|| IsTextInput(QApplication::focusWidget())) {
			return base::EventFilterResult::Continue;
		}
		const auto window = Core::App().activeWindow();
		const auto controller = window ? window->sessionController() : nullptr;
		if (!controller) {
			return base::EventFilterResult::Continue;
		}
		Show(controller);
		return base::EventFilterResult::Cancel;
	});
}

} // namespace AyuFeatures::CommandPalette
