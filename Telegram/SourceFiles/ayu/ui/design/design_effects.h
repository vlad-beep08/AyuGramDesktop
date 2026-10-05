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

[[nodiscard]] bool EffectsEnabled();
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
[[nodiscard]] float64 FlightEase(float64 progress);

void SetupPowerMode(not_null<Ui::InputField*> field);

void MakeMagnetic(QWidget *widget);

void SetWallpaperCanvas(not_null<QWidget*> canvas);
[[nodiscard]] bool IsWallpaperCanvas(QSize fill);
void StartWallWave(not_null<QWidget*> source, QPoint position);
void PaintWallWave(QPainter &p, QRect clip);

void PaintLiveUserpic(
	QPainter &p,
	const void *key,
	QRect userpic,
	LiveUserpic state);

} // namespace AyuDesign
