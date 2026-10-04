// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace AyuDesign {

struct ThemeInfo {
	QString id;
	QString title;
	QString path;
	std::vector<QColor> swatches;
	std::vector<QColor> wallpaper;
	int wallpaperIntensity = 0;
};

struct AccentInfo {
	QString id;
	QString title;
	QColor color;
};

[[nodiscard]] std::vector<ThemeInfo> Themes();
[[nodiscard]] QString CurrentThemeId();
bool ApplyTheme(const QString &id);
void EnsureWebTheme();

[[nodiscard]] std::vector<AccentInfo> Accents();
[[nodiscard]] QString CurrentAccentId();
bool ApplyAccent(const QString &id);

} // namespace AyuDesign
