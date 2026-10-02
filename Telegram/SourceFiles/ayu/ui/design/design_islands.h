// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "base/unique_qptr.h"

namespace Ui {
class RpWidget;
} // namespace Ui

namespace AyuDesign {

void PaintSoftShadow(
	QPainter &p,
	QRect rect,
	int radius,
	float64 opacity);
void PaintIslandShadow(QPainter &p, QRect island);
void PaintPillShadow(QPainter &p, QRect pill, int radius);
void PaintPillSurface(
	QPainter &p,
	QRect pill,
	int radius,
	const QBrush &brush);

class IslandCorners final {
public:
	IslandCorners(
		not_null<QWidget*> parent,
		Fn<void(QPainter&, QRect)> paintBackdrop);
	~IslandCorners();

	void setIsland(QRect island);
	void setVisible(bool visible);
	void bindVisibility(not_null<Ui::RpWidget*> target);
	void raise();
	void refresh();

private:
	struct Corner {
		base::unique_qptr<Ui::RpWidget> widget;
		QImage cache;
	};

	void paintCorner(Corner &corner);
	void updateGeometry();

	const Fn<void(QPainter&, QRect)> _paintBackdrop;
	std::array<Corner, 4> _corners;
	QRect _island;
	bool _visible = true;
	rpl::lifetime _lifetime;

};

} // namespace AyuDesign
