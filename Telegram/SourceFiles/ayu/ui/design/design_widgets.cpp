// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_widgets.h"

#include "ayu/ui/design/design_system.h"
#include "ui/effects/ripple_animation.h"
#include "ui/painter.h"
#include "ui/style/style_core_scale.h"
#include "styles/style_basic.h"
#include "styles/style_widgets.h"

namespace AyuDesign {
namespace {

constexpr auto kListRowBaseHeight = 36;
constexpr auto kThemeCardBaseHeight = 52;
constexpr auto kSwatchBaseSize = 20;
constexpr auto kActiveBorderBaseWidth = 2;

[[nodiscard]] State StateFor(
		bool down,
		bool over,
		bool selected,
		bool disabled) {
	if (disabled) {
		return State::Disabled;
	} else if (down) {
		return State::Pressed;
	} else if (selected) {
		return State::Selected;
	} else if (over) {
		return State::Hover;
	}
	return State::Normal;
}

[[nodiscard]] QRect SurfaceRect(QSize size) {
	const auto horizontal = SpacePx(Space::S);
	const auto vertical = SpacePx(Space::XXS);
	return QRect(QPoint(), size).marginsRemoved(
		{ horizontal, vertical, horizontal, vertical });
}

void PaintSurface(QPainter &p, QRect rect, Role role, State state) {
	if (state == State::Normal || state == State::Disabled) {
		return;
	}
	auto hq = PainterHighQualityEnabler(p);
	const auto radius = RadiusPx(Radius::Medium);
	p.setPen(Qt::NoPen);
	p.setBrush(Background(role, state));
	p.drawRoundedRect(rect, radius, radius);
}

} // namespace

ListRow::ListRow(
	QWidget *parent,
	const QString &title,
	const QString &label)
: RippleButton(parent, st::defaultRippleAnimation)
, _title(title)
, _label(label) {
}

void ListRow::setSelected(bool selected) {
	if (_selected != selected) {
		_selected = selected;
		update();
	}
}

bool ListRow::selected() const {
	return _selected;
}

int ListRow::resizeGetHeight(int newWidth) {
	return Dense(style::ConvertScale(kListRowBaseHeight))
		+ 2 * SpacePx(Space::XXS);
}

QRect ListRow::surfaceRect() const {
	return SurfaceRect(size());
}

void ListRow::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	const auto rect = surfaceRect();
	const auto state = StateFor(
		isDown(),
		isOver(),
		_selected,
		isDisabled());
	PaintSurface(p, rect, Role::Surface, state);
	paintRipple(p, rect.topLeft());

	const auto &font = st::normalFont;
	const auto padding = SpacePx(Space::M);
	const auto baseline = rect.y()
		+ (rect.height() - font->height) / 2
		+ font->ascent;
	p.setFont(font);

	auto labelWidth = 0;
	if (!_label.isEmpty()) {
		labelWidth = font->width(_label);
		p.setPen(Foreground(Role::Surface, AyuDesign::State::Disabled));
		p.drawText(
			rect.x() + rect.width() - padding - labelWidth,
			baseline,
			_label);
	}
	const auto available = rect.width()
		- 2 * padding
		- (labelWidth ? (labelWidth + padding) : 0);
	p.setPen(Foreground(Role::Surface, state));
	p.drawText(
		rect.x() + padding,
		baseline,
		font->elided(_title, std::max(available, 0)));
}

void ListRow::onStateChanged(State was, StateChangeSource source) {
	RippleButton::onStateChanged(was, source);
	update();
}

QImage ListRow::prepareRippleMask() const {
	return Ui::RippleAnimation::RoundRectMask(
		surfaceRect().size(),
		RadiusPx(Radius::Medium));
}

QPoint ListRow::prepareRippleStartPosition() const {
	return mapFromGlobal(QCursor::pos()) - surfaceRect().topLeft();
}

ThemeCard::ThemeCard(
	QWidget *parent,
	const QString &title,
	std::vector<QColor> swatches)
: RippleButton(parent, st::defaultRippleAnimation)
, _title(title)
, _swatches(std::move(swatches)) {
}

void ThemeCard::setActive(bool active) {
	if (_active != active) {
		_active = active;
		update();
	}
}

int ThemeCard::resizeGetHeight(int newWidth) {
	return Dense(style::ConvertScale(kThemeCardBaseHeight))
		+ 2 * SpacePx(Space::XXS);
}

QRect ThemeCard::surfaceRect() const {
	return SurfaceRect(size());
}

void ThemeCard::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	const auto rect = surfaceRect();
	const auto radius = RadiusPx(Radius::Medium);
	const auto state = StateFor(isDown(), isOver(), false, isDisabled());

	p.setPen(Qt::NoPen);
	p.setBrush(Background(Role::Raised, state));
	p.drawRoundedRect(rect, radius, radius);
	paintRipple(p, rect.topLeft());

	if (_active) {
		const auto border = style::ConvertScale(kActiveBorderBaseWidth);
		auto pen = QPen(Background(Role::Accent, AyuDesign::State::Normal));
		pen.setWidth(border);
		p.setPen(pen);
		p.setBrush(Qt::NoBrush);
		const auto half = border / 2.;
		p.drawRoundedRect(
			QRectF(rect).marginsRemoved({ half, half, half, half }),
			radius,
			radius);
	}

	const auto padding = SpacePx(Space::M);
	const auto swatch = style::ConvertScale(kSwatchBaseSize);
	const auto overlap = swatch / 3;
	auto left = rect.x() + padding;
	const auto top = rect.y() + (rect.height() - swatch) / 2;
	for (const auto &color : _swatches) {
		p.setPen(QPen(Background(Role::Raised, AyuDesign::State::Normal), 2));
		p.setBrush(color);
		p.drawEllipse(QRect(left, top, swatch, swatch));
		left += swatch - overlap;
	}
	left += overlap + padding;

	const auto &font = _active ? st::semiboldFont : st::normalFont;
	p.setFont(font);
	p.setPen(Foreground(Role::Raised, AyuDesign::State::Normal));
	p.drawText(
		left,
		rect.y() + (rect.height() - font->height) / 2 + font->ascent,
		font->elided(_title, std::max(rect.right() - padding - left, 0)));
}

void ThemeCard::onStateChanged(State was, StateChangeSource source) {
	RippleButton::onStateChanged(was, source);
	update();
}

QImage ThemeCard::prepareRippleMask() const {
	return Ui::RippleAnimation::RoundRectMask(
		surfaceRect().size(),
		RadiusPx(Radius::Medium));
}

QPoint ThemeCard::prepareRippleStartPosition() const {
	return mapFromGlobal(QCursor::pos()) - surfaceRect().topLeft();
}

} // namespace AyuDesign
