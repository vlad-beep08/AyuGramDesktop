// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace AyuDesign {

enum class BurstKind {
	Send,
	Alarm,
};

[[nodiscard]] bool EffectsEnabled();
void SetupEffects();

void PaintWebRowSpotlight(QPainter &p, QRect row, bool active);
void Burst(not_null<QWidget*> source, BurstKind kind);

[[nodiscard]] float64 PulseScale(
	QPainter &p,
	const void *key,
	int count,
	QRect area);
[[nodiscard]] float64 StaggerProgress(crl::time started, int index);
[[nodiscard]] crl::time StaggerDuration(int count);
[[nodiscard]] float64 BounceScale(float64 progress);
[[nodiscard]] float64 BounceAngle(float64 progress);

} // namespace AyuDesign
