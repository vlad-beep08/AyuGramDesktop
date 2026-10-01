// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_themes.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "window/themes/window_theme.h"

namespace AyuDesign {
namespace {

[[nodiscard]] QString ThemePath(const QString &id) {
	return u":/gui/chickengram/themes/%1.tdesktop-theme"_q.arg(id);
}

} // namespace

std::vector<ThemeInfo> Themes() {
	return {
		{
			.id = u"dawn"_q,
			.title = tr::ayu_DesignThemeDawn(tr::now),
			.path = ThemePath(u"dawn"_q),
			.swatches = { QColor(0xff, 0xff, 0xff), QColor(0xe7, 0x8c, 0x3c), QColor(0xfe, 0xec, 0xdd) },
		},
		{
			.id = u"sunset"_q,
			.title = tr::ayu_DesignThemeSunset(tr::now),
			.path = ThemePath(u"sunset"_q),
			.swatches = { QColor(0x27, 0x20, 0x1c), QColor(0xc9, 0x77, 0x40), QColor(0x96, 0x5c, 0x36) },
		},
		{
			.id = u"midnight"_q,
			.title = tr::ayu_DesignThemeMidnight(tr::now),
			.path = ThemePath(u"midnight"_q),
			.swatches = { QColor(0x0f, 0x0f, 0x0f), QColor(0xd2, 0xa2, 0x42), QColor(0xa1, 0x7c, 0x33) },
		},
		{
			.id = u"graphite"_q,
			.title = tr::ayu_DesignThemeGraphite(tr::now),
			.path = ThemePath(u"graphite"_q),
			.swatches = { QColor(0x21, 0x21, 0x21), QColor(0xc5, 0x73, 0x44), QColor(0x94, 0x5a, 0x38) },
		},
	};
}

QString CurrentThemeId() {
	return AyuSettings::getInstance().designTheme();
}

bool ApplyTheme(const QString &id) {
	for (const auto &theme : Themes()) {
		if (theme.id != id) {
			continue;
		}
		if (!Window::Theme::Apply(theme.path)) {
			return false;
		}
		Window::Theme::KeepApplied();
		AyuSettings::getInstance().setDesignTheme(id);
		return true;
	}
	return false;
}

} // namespace AyuDesign
