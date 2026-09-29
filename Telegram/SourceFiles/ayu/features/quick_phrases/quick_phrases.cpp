// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/quick_phrases/quick_phrases.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

namespace AyuFeatures::QuickPhrases {
namespace {

constexpr auto kMaxPhrases = 10;

void FillEditBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::ayu_QuickPhrasesTitle());
	box->setWidth(st::boxWideWidth);

	const auto field = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::newGroupDescription,
		Ui::InputField::Mode::MultiLine,
		tr::ayu_QuickPhrasesPlaceholder(),
		AyuSettings::getInstance().quickPhrases()));
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		tr::ayu_QuickPhrasesAbout(),
		st::boxDividerLabel));

	box->setFocusCallback([=] {
		field->setFocusFast();
	});
	box->addButton(tr::lng_settings_save(), [=] {
		AyuSettings::getInstance().setQuickPhrases(
			field->getLastText().trimmed());
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] {
		box->closeBox();
	});
}

} // namespace

std::vector<QString> List() {
	auto result = std::vector<QString>();
	const auto lines = AyuSettings::getInstance().quickPhrases().split(u'\n');
	for (const auto &line : lines) {
		const auto phrase = line.trimmed();
		if (!phrase.isEmpty()) {
			result.push_back(phrase);
			if (int(result.size()) == kMaxPhrases) {
				break;
			}
		}
	}
	return result;
}

void ShowEditBox(not_null<Window::SessionController*> controller) {
	controller->show(Box(FillEditBox));
}

} // namespace AyuFeatures::QuickPhrases
