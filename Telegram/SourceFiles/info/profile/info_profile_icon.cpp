/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "info/profile/info_profile_icon.h"

#include "ayu/ui/design/design_system.h"
#include "ui/painter.h"
#include "styles/style_ayu_icons.h"

namespace Info {
namespace Profile {

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
	const auto extra = AyuDesign::WebLayout()
		? std::min({
			int(st::ayuWebSettingsIconPadding),
			_point.x(),
			_point.y() })
		: 0;
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
		const auto padding = std::min({
			int(st::ayuWebSettingsIconPadding),
			_point.x(),
			_point.y() });
		const auto square = QRect(_point, _icon->size()).marginsAdded(
			{ padding, padding, padding, padding });
		const auto radius = std::min(square.width(), square.height()) / 4;
		auto hq = PainterHighQualityEnabler(p);
		p.setPen(Qt::NoPen);
		p.setBrush(AyuDesign::WebIconBackground(_icon.get()));
		p.drawRoundedRect(square, radius, radius);
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
