// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/keyword_alerts/keyword_alerts.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "history/history_item.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_widgets.h"

#include <QtCore/QRegularExpression>

namespace AyuFeatures::KeywordAlerts {
namespace {

struct Compiled {
	QString source;
	std::optional<QRegularExpression> regex;
};

[[nodiscard]] const std::optional<QRegularExpression> &Regex() {
	static auto cache = Compiled();
	const auto &source = AyuSettings::getInstance().alertKeywords();
	if (cache.source == source) {
		return cache.regex;
	}
	cache.source = source;
	cache.regex = std::nullopt;
	auto words = QStringList();
	for (const auto &part : source.split(QRegularExpression(u"[,\n]"_q))) {
		const auto word = part.trimmed();
		if (!word.isEmpty()) {
			words.push_back(QRegularExpression::escape(word));
		}
	}
	if (!words.isEmpty()) {
		cache.regex = QRegularExpression(
			u"(?<![\w])("_q + words.join(u'|') + u")(?![\w])"_q,
			QRegularExpression::CaseInsensitiveOption
				| QRegularExpression::UseUnicodePropertiesOption);
	}
	return cache.regex;
}

void FillEditBox(not_null<Ui::GenericBox*> box) {
	box->setTitle(tr::ayu_KeywordAlertsTitle());
	box->setWidth(st::boxWideWidth);

	const auto field = box->addRow(object_ptr<Ui::InputField>(
		box,
		st::newGroupDescription,
		Ui::InputField::Mode::MultiLine,
		tr::ayu_KeywordAlertsPlaceholder(),
		AyuSettings::getInstance().alertKeywords()));
	box->addRow(object_ptr<Ui::FlatLabel>(
		box,
		tr::ayu_KeywordAlertsAbout(),
		st::boxDividerLabel));

	box->setFocusCallback([=] {
		field->setFocusFast();
	});
	box->addButton(tr::lng_settings_save(), [=] {
		AyuSettings::getInstance().setAlertKeywords(
			field->getLastText().trimmed());
		box->closeBox();
	});
	box->addButton(tr::lng_cancel(), [=] {
		box->closeBox();
	});
}

} // namespace

bool Matches(not_null<HistoryItem*> item) {
	if (item->out()) {
		return false;
	}
	const auto &regex = Regex();
	return regex && regex->match(item->originalText().text).hasMatch();
}

void ShowEditBox(not_null<Window::SessionController*> controller) {
	controller->show(Box(FillEditBox));
}

} // namespace AyuFeatures::KeywordAlerts
