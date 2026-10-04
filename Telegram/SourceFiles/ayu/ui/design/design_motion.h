// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace AyuDesign {

[[nodiscard]] float64 HoverValue(
	QPainter &p,
	const void *key,
	bool hovered,
	QRect rect = QRect());

void CrossFade(not_null<QWidget*> target);
void SlideOut(
	not_null<QWidget*> parent,
	QPixmap snapshot,
	QRect geometry,
	int radius);

} // namespace AyuDesign
