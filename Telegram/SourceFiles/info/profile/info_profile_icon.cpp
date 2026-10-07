/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "info/profile/info_profile_icon.h"

#include "ayu/ui/design/design_system.h"
#include "ui/painter.h"
#include "ui/style/style_core_scale.h"

namespace Info {
namespace Profile {
namespace {

constexpr auto kWebIconSquare = 30;
constexpr auto kWebIconGlyph = 0.75;

[[nodiscard]] int WebIconExtra(const style::icon &icon) {
	const auto side = style::ConvertScale(kWebIconSquare);
	return std::max(0, (side - std::min(icon.width(), icon.height()) + 1) / 2);
}

} // namespace

FloatingIcon::FloatingIcon(
	RpWidget *parent,
	const style::icon &icon,
	QPoint position)
: FloatingIcon(parent, icon, position, Tag{}) {
}

FloatingIcon::FloatingIcon(
	RpWidget *parent,
	const style::icon &icon,
	QPoint position,
	const Tag &)
: RpWidget(parent)
, _icon(&icon)
, _point(position) {
	const auto extra = AyuDesign::WebLayout() ? WebIconExtra(icon) : 0;
	setGeometry(QRect(
		QPoint(0, 0),
		QSize(
			_point.x() + _icon->width() + extra,
			_point.y() + _icon->height() + extra)));
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

void FloatingIcon::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	if (AyuDesign::WebLayout()) {
		const auto glyph = QRectF(QRect(_point, _icon->size()));
		const auto center = glyph.center();
		const auto side = float64(style::ConvertScale(kWebIconSquare));
		const auto square = QRectF(
			center - QPointF(side / 2., side / 2.),
			QSizeF(side, side));
		const auto radius = side / 4.;
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(Qt::NoPen);
		p.setBrush(AyuDesign::WebIconBackground(_icon.get()));
		p.drawRoundedRect(square, radius, radius);
		const auto scale = std::min(
			1.,
			side * kWebIconGlyph / std::max(glyph.width(), 1.));
		p.translate(center);
		p.scale(scale, scale);
		p.translate(-center);
		_icon->paint(
			p,
			_point.x(),
			_point.y(),
			width(),
			st::settingsIconFg->c);
		return;
	}
	_icon->paint(p, _point, width());
}

} // namespace Profile
} // namespace Info
