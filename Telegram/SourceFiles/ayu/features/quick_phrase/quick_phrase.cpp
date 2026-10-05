// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/quick_phrase/quick_phrase.h"

#include "ayu/ui/design/design_effects.h"
#include "ui/painter.h"
#include "styles/style_ayu_icons.h"
#include "styles/style_basic.h"

namespace AyuFeatures::QuickPhrase {
namespace {

constexpr auto kBadgeSize = 34;
constexpr auto kTextPadding = 3;
constexpr auto kMaxFontSize = 13;
constexpr auto kMinFontSize = 7;
constexpr auto kRadiusRatio = 0.24;
constexpr auto kGlossInsetRatio = 0.07;
constexpr auto kGlossHeightRatio = 0.5;
constexpr auto kBounceDuration = crl::time(520);

[[nodiscard]] QFont FitFont(const QString &text, int width) {
	auto result = st::semiboldFont->f;
	result.setWeight(QFont::Bold);
	auto size = style::ConvertScale(kMaxFontSize);
	const auto minimal = style::ConvertScale(kMinFontSize);
	while (true) {
		result.setPixelSize(size);
		if (size <= minimal
			|| QFontMetrics(result).horizontalAdvance(text) <= width) {
			return result;
		}
		--size;
	}
}

} // namespace

QString Text() {
	return u"жопа"_q;
}

Button::Button(QWidget *parent)
: IconButton(parent, st::ayuQuickPhraseToggle)
, _label(Text().toUpper())
, _font(FitFont(
	_label,
	style::ConvertScale(kBadgeSize - 2 * kTextPadding))) {
	clicks(
	) | rpl::on_next([=] {
		if (!AyuDesign::EffectsEnabled()) {
			return;
		}
		_bounce.stop();
		_bounce.start([=] {
			update();
		}, 0., 1., kBounceDuration, anim::linear);
		AyuDesign::Burst(this);
	}, lifetime());
}

void Button::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);

	const auto side = style::ConvertScale(kBadgeSize);
	const auto shift = isDown() ? style::ConvertScale(1) : 0;
	const auto badge = QRectF(
		(width() - side) / 2.,
		(height() - side) / 2. + shift,
		side,
		side);
	const auto radius = side * kRadiusRatio;
	const auto line = std::max(style::ConvertScale(1), 1);
	if (_bounce.animating()) {
		const auto progress = _bounce.value(1.);
		const auto scale = AyuDesign::BounceScale(progress);
		const auto center = badge.center();
		p.translate(center);
		p.rotate(AyuDesign::BounceAngle(progress));
		p.scale(scale, scale);
		p.translate(-center);
	}

	auto base = QLinearGradient(badge.topLeft(), badge.bottomLeft());
	base.setColorAt(0., QColor(0xFF, 0x5B, 0x45));
	base.setColorAt(0.45, QColor(0xF1, 0x2D, 0x19));
	base.setColorAt(1., QColor(0xCB, 0x1A, 0x0D));
	p.setPen(QPen(QColor(0xB5, 0x13, 0x09), line));
	p.setBrush(base);
	const auto half = line / 2.;
	p.drawRoundedRect(
		badge.adjusted(half, half, -half, -half),
		radius,
		radius);

	const auto inset = side * kGlossInsetRatio;
	const auto gloss = QRectF(
		badge.x() + inset,
		badge.y() + inset,
		badge.width() - 2 * inset,
		badge.height() * kGlossHeightRatio);
	auto shine = QLinearGradient(gloss.topLeft(), gloss.bottomLeft());
	shine.setColorAt(0., QColor(255, 255, 255, 120));
	shine.setColorAt(1., QColor(255, 255, 255, 0));
	p.setPen(Qt::NoPen);
	p.setBrush(shine);
	p.drawRoundedRect(gloss, radius - inset, radius - inset);

	if (isDown()) {
		p.setBrush(QColor(0, 0, 0, 36));
		p.drawRoundedRect(badge, radius, radius);
	} else if (isOver()) {
		p.setBrush(QColor(255, 255, 255, 30));
		p.drawRoundedRect(badge, radius, radius);
	}

	p.setFont(_font);
	p.setPen(QColor(0xA8, 0xB8, 0xD2));
	p.drawText(badge.translated(0, line), Qt::AlignCenter, _label);
	p.setPen(QColor(255, 255, 255));
	p.drawText(badge, Qt::AlignCenter, _label);
}

void Button::onStateChanged(State was, StateChangeSource source) {
	IconButton::onStateChanged(was, source);
	update();
}

} // namespace AyuFeatures::QuickPhrase
