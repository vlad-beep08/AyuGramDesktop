// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_themes.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/design/design_system.h"
#include "data/data_wall_paper.h"
#include "window/themes/window_theme.h"

namespace AyuDesign {
namespace {

constexpr auto kWebWallPaperIntensity = 38;

[[nodiscard]] QString ThemePath(const QString &id) {
	return u":/gui/chickengram/themes/%1.tdesktop-theme"_q.arg(id);
}

void ApplyWallPaper(const ThemeInfo &theme) {
	if (theme.wallpaper.empty()) {
		return;
	}
	Window::Theme::Background()->set(
		Data::DefaultWallPaper().withBackgroundColors(
			theme.wallpaper
		).withPatternIntensity(theme.wallpaperIntensity));
}

} // namespace

std::vector<ThemeInfo> Themes() {
	return {
		{
			.id = u"web-light"_q,
			.title = tr::ayu_DesignThemeWebLight(tr::now),
			.path = ThemePath(u"web-light"_q),
			.swatches = { QColor(0xff, 0xff, 0xff), QColor(0x33, 0x90, 0xec), QColor(0xee, 0xff, 0xde) },
			.wallpaper = { QColor(0xbd, 0xcd, 0x8c), QColor(0x8e, 0xba, 0x89), QColor(0x83, 0xb2, 0x8f), QColor(0xc5, 0xd3, 0xb0) },
			.wallpaperIntensity = kWebWallPaperIntensity,
		},
		{
			.id = u"web-dark"_q,
			.title = tr::ayu_DesignThemeWebDark(tr::now),
			.path = ThemePath(u"web-dark"_q),
			.swatches = { QColor(0x21, 0x21, 0x21), QColor(0x87, 0x74, 0xe1), QColor(0x76, 0x6a, 0xc8) },
			.wallpaper = { QColor(0x4f, 0x5b, 0xd5), QColor(0x96, 0x2f, 0xbf), QColor(0xdd, 0x6c, 0xb9), QColor(0xfe, 0xc4, 0x96) },
			.wallpaperIntensity = -kWebWallPaperIntensity,
		},
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
		ApplyWallPaper(theme);
		AyuSettings::getInstance().setDesignTheme(id);
		return true;
	}
	return false;
}

void EnsureWebTheme() {
	if (!WebLayout() || !CurrentThemeId().isEmpty()) {
		return;
	}
	ApplyTheme(Window::Theme::IsNightMode()
		? u"web-dark"_q
		: u"web-light"_q);
}

} // namespace AyuDesign
