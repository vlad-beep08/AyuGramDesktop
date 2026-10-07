// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace Ui {
class InputField;
} // namespace Ui

namespace AyuDesign {

enum class LiveUserpic {
	None,
	Online,
	Typing,
};

enum class Effect : int {
	Spotlight = (1 << 0),
	Burst = (1 << 1),
	Pulse = (1 << 2),
	OnlineRing = (1 << 3),
	TypingArc = (1 << 4),
	Cascade = (1 << 5),
	Peanuts = (1 << 6),
	MessageAppear = (1 << 7),
};

enum class EffectsLevel {
	Saving,
	Light,
	Full,
};

[[nodiscard]] bool EffectOn(Effect effect);
[[nodiscard]] int EffectsPreset(EffectsLevel level);
void ApplyEffectsLevel(EffectsLevel level);
void SetEffectEnabled(Effect effect, bool enabled);
void SetupEffects();

void PaintWebRowSpotlight(QPainter &p, QRect row, bool active);
void Burst(not_null<QWidget*> source);

[[nodiscard]] float64 PulseScale(
	QPainter &p,
	const void *key,
	int count,
	QRect area);
[[nodiscard]] float64 StaggerProgress(crl::time started, int index);
[[nodiscard]] crl::time StaggerDuration(int count);
[[nodiscard]] float64 BounceScale(float64 progress);
[[nodiscard]] float64 BounceAngle(float64 progress);

void SetupPowerMode(not_null<Ui::InputField*> field);

void PaintLiveUserpic(
	QPainter &p,
	const void *key,
	QRect userpic,
	LiveUserpic state);

} // namespace AyuDesign
