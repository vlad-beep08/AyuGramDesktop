// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace AyuDesign {

enum class Density {
	Compact = 0,
	Normal = 1,
	Comfortable = 2,
};

enum class Corners {
	Telegram = 0,
	Sharp = 1,
	Soft = 2,
	Round = 3,
};

enum class Motion {
	Full = 0,
	Reduced = 1,
	Off = 2,
};

enum class Radius {
	Small,
	Medium,
	Large,
	Window,
	Pill,
};

enum class Space {
	XXS,
	XS,
	S,
	M,
	L,
	XL,
};

enum class Duration {
	Fast,
	Normal,
	Slow,
};

enum class Opacity {
	Disabled,
	Secondary,
	Scrim,
	Overlay,
};

enum class Role {
	Surface,
	Raised,
	Accent,
	Neutral,
	Danger,
};

enum class State {
	Normal,
	Hover,
	Pressed,
	Selected,
	Disabled,
};

[[nodiscard]] Density CurrentDensity();
[[nodiscard]] Corners CurrentCorners();
[[nodiscard]] Motion CurrentMotion();

[[nodiscard]] int RadiusPx(Radius radius);
[[nodiscard]] int SpacePx(Space space);
[[nodiscard]] float64 DensityFactor();
[[nodiscard]] int Dense(int scaledPixels);
[[nodiscard]] crl::time DurationMs(Duration duration);
[[nodiscard]] float64 OpacityValue(Opacity opacity);

[[nodiscard]] QColor Background(Role role, State state);
[[nodiscard]] QColor Foreground(Role role, State state);
[[nodiscard]] QColor Mix(QColor from, QColor to, float64 progress);

void ApplyStyleOverrides();
void ApplyMotion();
void ApplyWindowOpacity(not_null<QWidget*> window);
void ApplyWindowOpacityToAll();

} // namespace AyuDesign
