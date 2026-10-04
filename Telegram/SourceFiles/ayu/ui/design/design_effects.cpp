// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_effects.h"

#include "ayu/ayu_settings.h"
#include "ayu/ui/design/design_system.h"
#include "ui/effects/animation_value.h"
#include "ui/effects/animations.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"
#include "ui/widgets/buttons.h"
#include "styles/palette.h"

#include <QtGui/QCursor>
#include <QtGui/QRadialGradient>
#include <QtWidgets/QApplication>

#include <random>

namespace AyuDesign {
namespace {

constexpr auto kPi = 3.14159265358979323846;
constexpr auto kSpotRadius = 150;
constexpr auto kSpotAlpha = 0.22;
constexpr auto kSpotActiveAlpha = 0.14;
constexpr auto kSpotButtonAlpha = 0.12;
constexpr auto kBurstDuration = crl::time(650);
constexpr auto kSendReach = 72;
constexpr auto kAlarmReach = 120;
constexpr auto kSendParticles = 16;
constexpr auto kAlarmParticles = 30;
constexpr auto kParticleMin = 2;
constexpr auto kParticleExtra = 3;
constexpr auto kSendGravity = 0.15;
constexpr auto kAlarmGravity = 0.45;
constexpr auto kRingAlpha = 0.7;
constexpr auto kFlashAlpha = 0.35;
constexpr auto kPulseDuration = crl::time(480);
constexpr auto kPulseAmplitude = 0.35;
constexpr auto kPulseDamping = 4.;
constexpr auto kPulseFrequency = 2.5;
constexpr auto kStaggerStep = crl::time(28);
constexpr auto kStaggerDuration = crl::time(240);
constexpr auto kBounceAmplitude = 0.2;
constexpr auto kBounceAngle = 10.;

base::flat_map<QWidget*, QRect> SpotAreas;

struct PulseEntry {
	int count = 0;
	crl::time started = 0;
	QRect area;
};

struct PulseState {
	base::flat_map<const void*, PulseEntry> entries;
	std::unique_ptr<Ui::Animations::Basic> animation;
};

base::flat_map<QWidget*, std::unique_ptr<PulseState>> PulseStates;

[[nodiscard]] QColor WithAlpha(QColor color, float64 alpha) {
	color.setAlphaF(std::clamp(alpha, 0., 1.));
	return color;
}

void TrackSpot(not_null<QWidget*> widget, QRect area) {
	const auto raw = widget.get();
	const auto i = SpotAreas.find(raw);
	if (i != end(SpotAreas)) {
		i->second = area;
		return;
	}
	SpotAreas.emplace(raw, area);
	QObject::connect(raw, &QObject::destroyed, [=] {
		SpotAreas.remove(raw);
	});
}

void PaintSpotlight(
		QPainter &p,
		QRect rect,
		int radius,
		const QColor &color) {
	const auto widget = dynamic_cast<QWidget*>(p.device());
	if (!widget || rect.isEmpty()) {
		return;
	}
	auto invertible = false;
	const auto inverted = p.transform().inverted(&invertible);
	if (!invertible) {
		return;
	}
	const auto local = widget->mapFromGlobal(QCursor::pos());
	const auto point = inverted.map(QPointF(local));
	if (!QRectF(rect).contains(point)) {
		return;
	}
	TrackSpot(widget, p.transform().mapRect(rect));
	auto gradient = QRadialGradient(point, style::ConvertScale(kSpotRadius));
	gradient.setColorAt(0., color);
	gradient.setColorAt(1., WithAlpha(color, 0.));
	auto hq = PainterHighQualityEnabler(p);
	p.setPen(Qt::NoPen);
	p.setBrush(gradient);
	if (radius > 0) {
		p.drawRoundedRect(rect, radius, radius);
	} else {
		p.drawRect(rect);
	}
}

struct Particle {
	QPointF velocity;
	float64 size = 0.;
	QColor color;
};

class BurstOverlay final : public Ui::RpWidget {
public:
	BurstOverlay(not_null<QWidget*> parent, QPoint center, BurstKind kind);

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	const BurstKind _kind;
	const int _reach = 0;
	QPointF _center;
	std::vector<Particle> _particles;
	Ui::Animations::Simple _animation;

};

BurstOverlay::BurstOverlay(
	not_null<QWidget*> parent,
	QPoint center,
	BurstKind kind)
: RpWidget(parent)
, _kind(kind)
, _reach(style::ConvertScale(
	(kind == BurstKind::Send) ? kSendReach : kAlarmReach)) {
	const auto area = QRect(
		center - QPoint(_reach, _reach),
		QSize(2 * _reach, 2 * _reach)).intersected(parent->rect());
	setGeometry(area);
	_center = QPointF(center - area.topLeft());
	setAttribute(Qt::WA_TransparentForMouseEvents);

	const auto colors = (kind == BurstKind::Send)
		? std::vector<QColor>{
			st::windowBgActive->c,
			QColor(0xFF, 0xC8, 0x6B),
			QColor(0xFF, 0xFF, 0xFF),
		}
		: std::vector<QColor>{
			QColor(0xFF, 0x3B, 0x2A),
			QColor(0xFF, 0x8A, 0x1E),
			QColor(0xFF, 0xD2, 0x3F),
		};
	const auto count = (kind == BurstKind::Send)
		? kSendParticles
		: kAlarmParticles;
	auto generator = std::minstd_rand(uint32(crl::now()));
	auto unit = std::uniform_real_distribution<float64>(0., 1.);
	_particles.reserve(count);
	for (auto i = 0; i != count; ++i) {
		const auto angle = (2. * kPi * i) / count + unit(generator) * 0.6;
		const auto speed = _reach * (0.5 + 0.45 * unit(generator));
		_particles.push_back({
			.velocity = QPointF(std::cos(angle), std::sin(angle)) * speed,
			.size = style::ConvertScale(kParticleMin)
				+ unit(generator) * style::ConvertScale(kParticleExtra),
			.color = colors[i % colors.size()],
		});
	}
	show();
	raise();
	_animation.start([=] {
		update();
		if (!_animation.animating()) {
			deleteLater();
		}
	}, 0., 1., kBurstDuration, anim::linear);
}

void BurstOverlay::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	const auto t = _animation.value(1.);
	const auto eased = 1. - std::pow(1. - t, 3.);
	const auto fade = 1. - t;
	const auto accent = (_kind == BurstKind::Send)
		? st::windowBgActive->c
		: QColor(0xFF, 0x4A, 0x2A);

	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(
		WithAlpha(accent, kRingAlpha * fade),
		std::max(style::ConvertScale(2) * fade, 0.5)));
	const auto ring = _reach * (0.15 + 0.6 * eased);
	p.drawEllipse(_center, ring, ring);

	p.setPen(Qt::NoPen);
	p.setBrush(WithAlpha(accent, kFlashAlpha * fade * fade));
	const auto flash = _reach * 0.25 * eased;
	p.drawEllipse(_center, flash, flash);

	const auto gravity = _reach
		* ((_kind == BurstKind::Send) ? kSendGravity : kAlarmGravity);
	for (const auto &particle : _particles) {
		const auto position = _center
			+ particle.velocity * eased
			+ QPointF(0., gravity * t * t);
		const auto size = particle.size * (1. - 0.5 * t);
		p.setBrush(WithAlpha(particle.color, fade));
		p.drawEllipse(position, size, size);
	}
}

[[nodiscard]] not_null<PulseState*> ResolvePulse(not_null<QWidget*> widget) {
	const auto raw = widget.get();
	const auto i = PulseStates.find(raw);
	if (i != end(PulseStates)) {
		return i->second.get();
	}
	const auto state = PulseStates.emplace(
		raw,
		std::make_unique<PulseState>()).first->second.get();
	QObject::connect(raw, &QObject::destroyed, [=] {
		PulseStates.remove(raw);
	});
	state->animation = std::make_unique<Ui::Animations::Basic>([=] {
		const auto now = crl::now();
		auto region = QRegion();
		auto animating = false;
		for (auto &pair : state->entries) {
			auto &entry = pair.second;
			if (!entry.started) {
				continue;
			}
			if (!entry.area.isEmpty()) {
				region += entry.area;
			}
			if (now - entry.started >= kPulseDuration) {
				entry.started = 0;
			} else {
				animating = true;
			}
		}
		if (!region.isEmpty()) {
			raw->update(region);
		}
		if (!animating) {
			state->animation->stop();
		}
	});
	return state;
}

class EffectsFilter final : public QObject {
public:
	using QObject::QObject;

protected:
	bool eventFilter(QObject *object, QEvent *e) override {
		switch (e->type()) {
		case QEvent::MouseMove:
		case QEvent::Leave:
			if (const auto widget = qobject_cast<QWidget*>(object)) {
				const auto i = SpotAreas.find(widget);
				if (i != end(SpotAreas) && !i->second.isEmpty()) {
					widget->update(i->second);
					if (e->type() == QEvent::Leave) {
						i->second = QRect();
					}
				}
				if (EffectsEnabled()
					&& dynamic_cast<Ui::SettingsButton*>(widget)) {
					widget->update();
				}
			}
			break;
		case QEvent::Paint:
			if (!EffectsEnabled()) {
				break;
			}
			if (const auto button = dynamic_cast<Ui::SettingsButton*>(
					object)) {
				if (!button->isOver() || button->isDisabled()) {
					break;
				}
				static_cast<QObject*>(button)->event(e);
				auto p = QPainter(button);
				PaintSpotlight(
					p,
					button->rect(),
					0,
					WithAlpha(st::windowBgActive->c, kSpotButtonAlpha));
				return true;
			}
			break;
		default:
			break;
		}
		return false;
	}

};

} // namespace

bool EffectsEnabled() {
	return WebLayout()
		&& AyuSettings::getInstance().designEffects()
		&& (DurationMs(Duration::Normal) > 0);
}

void SetupEffects() {
	static auto installed = false;
	if (installed || !WebLayout()) {
		return;
	}
	installed = true;
	qApp->installEventFilter(new EffectsFilter(qApp));
}

void PaintWebRowSpotlight(QPainter &p, QRect row, bool active) {
	if (!EffectsEnabled()) {
		return;
	}
	const auto color = active
		? WithAlpha(QColor(255, 255, 255), kSpotActiveAlpha)
		: WithAlpha(st::windowBgActive->c, kSpotAlpha);
	PaintSpotlight(
		p,
		row.marginsRemoved({ WebRowInset(), 0, WebRowInset(), 0 }),
		WebRowRadius(),
		color);
}

void Burst(not_null<QWidget*> source, BurstKind kind) {
	if (!EffectsEnabled() || !source->isVisible()) {
		return;
	}
	const auto window = source->window();
	if (!window) {
		return;
	}
	const auto center = source->mapTo(window, source->rect().center());
	Ui::CreateChild<BurstOverlay>(window, center, kind);
}

float64 PulseScale(
		QPainter &p,
		const void *key,
		int count,
		QRect area) {
	const auto widget = dynamic_cast<QWidget*>(p.device());
	if (!widget) {
		return 1.;
	}
	const auto state = ResolvePulse(widget);
	auto i = state->entries.find(key);
	if (i == end(state->entries)) {
		state->entries.emplace(key, PulseEntry{ .count = count });
		return 1.;
	}
	auto &entry = i->second;
	const auto now = crl::now();
	if (count > entry.count && EffectsEnabled()) {
		entry.started = now;
		if (!state->animation->animating()) {
			state->animation->start();
		}
	}
	entry.count = count;
	if (!entry.started) {
		return 1.;
	}
	entry.area = p.transform().mapRect(area);
	const auto t = float64(now - entry.started) / kPulseDuration;
	if (t >= 1.) {
		return 1.;
	}
	return 1.
		+ kPulseAmplitude
			* std::exp(-kPulseDamping * t)
			* std::sin(kPulseFrequency * kPi * t);
}

float64 StaggerProgress(crl::time started, int index) {
	if (!started) {
		return 1.;
	}
	const auto passed = crl::now()
		- started
		- std::max(index, 0) * kStaggerStep;
	const auto t = std::clamp(float64(passed) / kStaggerDuration, 0., 1.);
	return 1. - std::pow(1. - t, 3.);
}

crl::time StaggerDuration(int count) {
	return std::max(count, 0) * kStaggerStep + kStaggerDuration;
}

float64 BounceScale(float64 progress) {
	if (progress <= 0. || progress >= 1.) {
		return 1.;
	}
	return 1.
		+ kBounceAmplitude
			* std::sin(2. * kPi * progress)
			* (1. - progress);
}

float64 BounceAngle(float64 progress) {
	if (progress <= 0. || progress >= 1.) {
		return 0.;
	}
	return kBounceAngle
		* std::sin(4. * kPi * progress)
		* (1. - progress);
}

} // namespace AyuDesign
