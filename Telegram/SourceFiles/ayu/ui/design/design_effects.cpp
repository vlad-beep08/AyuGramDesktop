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
#include "ui/widgets/fields/input_field.h"
#include "styles/palette.h"

#include <QtCore/QPointer>
#include <QtGui/QCursor>
#include <QtGui/QRadialGradient>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTextEdit>

#include <random>

namespace AyuDesign {
namespace {

constexpr auto kPi = 3.14159265358979323846;
constexpr auto kSpotRadius = 150;
constexpr auto kSpotAlpha = 0.22;
constexpr auto kSpotActiveAlpha = 0.14;
constexpr auto kSpotButtonAlpha = 0.12;
constexpr auto kBurstDuration = crl::time(650);
constexpr auto kTypingDuration = crl::time(380);
constexpr auto kSendReach = 72;
constexpr auto kAlarmReach = 120;
constexpr auto kTypingReach = 26;
constexpr auto kSendParticles = 16;
constexpr auto kAlarmParticles = 30;
constexpr auto kTypingParticles = 6;
constexpr auto kParticleMin = 2;
constexpr auto kParticleExtra = 3;
constexpr auto kSendGravity = 0.15;
constexpr auto kAlarmGravity = 0.45;
constexpr auto kTypingGravity = 0.6;
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
constexpr auto kBackOvershoot = 1.70158;
constexpr auto kPowerWindow = crl::time(700);
constexpr auto kPowerKeystrokes = 5;
constexpr auto kPowerShakeCooldown = crl::time(260);
constexpr auto kShakeAmplitude = 2.5;
constexpr auto kMagnetReach = 60;
constexpr auto kMagnetMax = 6.;
constexpr auto kMagnetStrength = 0.22;
constexpr auto kMagnetReturnDuration = crl::time(360);
constexpr auto kParallaxMargin = 6;
constexpr auto kParallaxFollow = 0.18;
constexpr auto kParallaxEpsilon = 0.05;
constexpr auto kWaveDuration = crl::time(1100);
constexpr auto kWaveCooldown = crl::time(1500);
constexpr auto kWaveAlpha = 0.22;
constexpr auto kWaveWidth = 70;
constexpr auto kWaveReach = 0.6;
constexpr auto kLivePeriod = crl::time(2400);
constexpr auto kLiveSpin = crl::time(1100);
constexpr auto kLiveFrame = crl::time(50);
constexpr auto kLiveStale = crl::time(1000);
constexpr auto kLiveGap = 3;
constexpr auto kLiveWidth = 2;
constexpr auto kLiveBaseAlpha = 0.35;
constexpr auto kLiveBreathAlpha = 0.3;
constexpr auto kLiveTrackAlpha = 0.25;
constexpr auto kLiveArcSpan = 100;

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

struct Magnet {
	QPointer<QWidget> widget;
	QPoint base;
	QPoint applied;
	bool applying = false;
	Ui::Animations::Simple back;
};

std::vector<std::unique_ptr<Magnet>> Magnets;
QPoint LastCursor;

QPointer<QWidget> Canvas;
QPointF ParallaxCurrent;
QPointF ParallaxTarget;
QPoint ParallaxApplied;
std::unique_ptr<Ui::Animations::Basic> ParallaxAnimation;

struct Wave {
	QPoint center;
	crl::time started = 0;
	QRect painted;
};

Wave CurrentWave;
crl::time LastWave = 0;
std::unique_ptr<Ui::Animations::Basic> WaveAnimation;

struct LiveEntry {
	QRect area;
	crl::time painted = 0;
};

struct LiveState {
	base::flat_map<const void*, LiveEntry> entries;
	std::unique_ptr<Ui::Animations::Basic> animation;
	crl::time lastFrame = 0;
};

base::flat_map<QWidget*, std::unique_ptr<LiveState>> LiveStates;

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

struct BurstConfig {
	int reach = 0;
	int count = 0;
	float64 gravity = 0.;
	crl::time duration = 0;
	bool ring = false;
	QColor accent;
	std::vector<QColor> colors;
};

[[nodiscard]] BurstConfig ConfigFor(BurstKind kind) {
	const auto gold = QColor(0xFF, 0xC8, 0x6B);
	switch (kind) {
	case BurstKind::Alarm: return {
		.reach = style::ConvertScale(kAlarmReach),
		.count = kAlarmParticles,
		.gravity = kAlarmGravity,
		.duration = kBurstDuration,
		.ring = true,
		.accent = QColor(0xFF, 0x4A, 0x2A),
		.colors = {
			QColor(0xFF, 0x3B, 0x2A),
			QColor(0xFF, 0x8A, 0x1E),
			QColor(0xFF, 0xD2, 0x3F),
		},
	};
	case BurstKind::Typing: return {
		.reach = style::ConvertScale(kTypingReach),
		.count = kTypingParticles,
		.gravity = kTypingGravity,
		.duration = kTypingDuration,
		.ring = false,
		.accent = st::windowBgActive->c,
		.colors = { st::windowBgActive->c, gold },
	};
	case BurstKind::Send: break;
	}
	return {
		.reach = style::ConvertScale(kSendReach),
		.count = kSendParticles,
		.gravity = kSendGravity,
		.duration = kBurstDuration,
		.ring = true,
		.accent = st::windowBgActive->c,
		.colors = {
			st::windowBgActive->c,
			gold,
			QColor(0xFF, 0xFF, 0xFF),
		},
	};
}

class BurstOverlay final : public Ui::RpWidget {
public:
	BurstOverlay(not_null<QWidget*> parent, QPoint center, BurstKind kind);

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	const BurstConfig _config;
	QPointF _center;
	std::vector<Particle> _particles;
	Ui::Animations::Simple _animation;

};

BurstOverlay::BurstOverlay(
	not_null<QWidget*> parent,
	QPoint center,
	BurstKind kind)
: RpWidget(parent)
, _config(ConfigFor(kind)) {
	const auto reach = _config.reach;
	const auto area = QRect(
		center - QPoint(reach, reach),
		QSize(2 * reach, 2 * reach)).intersected(parent->rect());
	setGeometry(area);
	_center = QPointF(center - area.topLeft());
	setAttribute(Qt::WA_TransparentForMouseEvents);

	auto generator = std::minstd_rand(uint32(crl::now()));
	auto unit = std::uniform_real_distribution<float64>(0., 1.);
	_particles.reserve(_config.count);
	for (auto i = 0; i != _config.count; ++i) {
		const auto angle = (2. * kPi * i) / _config.count
			+ unit(generator) * 0.6;
		const auto speed = reach * (0.5 + 0.45 * unit(generator));
		_particles.push_back({
			.velocity = QPointF(std::cos(angle), std::sin(angle)) * speed,
			.size = style::ConvertScale(kParticleMin)
				+ unit(generator) * style::ConvertScale(kParticleExtra),
			.color = _config.colors[i % _config.colors.size()],
		});
	}
	show();
	raise();
	_animation.start([=] {
		update();
		if (!_animation.animating()) {
			deleteLater();
		}
	}, 0., 1., _config.duration, anim::linear);
}

void BurstOverlay::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	const auto t = _animation.value(1.);
	const auto eased = 1. - std::pow(1. - t, 3.);
	const auto fade = 1. - t;
	const auto reach = _config.reach;

	if (_config.ring) {
		p.setBrush(Qt::NoBrush);
		p.setPen(QPen(
			WithAlpha(_config.accent, kRingAlpha * fade),
			std::max(style::ConvertScale(2) * fade, 0.5)));
		const auto ring = reach * (0.15 + 0.6 * eased);
		p.drawEllipse(_center, ring, ring);

		p.setPen(Qt::NoPen);
		p.setBrush(WithAlpha(_config.accent, kFlashAlpha * fade * fade));
		const auto flash = reach * 0.25 * eased;
		p.drawEllipse(_center, flash, flash);
	}

	p.setPen(Qt::NoPen);
	const auto gravity = reach * _config.gravity;
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

void ApplyMagnet(not_null<Magnet*> magnet, QPoint offset) {
	const auto widget = magnet->widget.data();
	if (!widget || offset == magnet->applied) {
		return;
	}
	magnet->applying = true;
	widget->move(magnet->base + offset);
	magnet->applying = false;
	magnet->applied = offset;
}

void UpdateMagnets(QPoint cursor) {
	Magnets.erase(ranges::remove_if(Magnets, [](const auto &magnet) {
		return !magnet->widget;
	}), end(Magnets));
	const auto enabled = EffectsEnabled();
	const auto reach = style::ConvertScale(kMagnetReach);
	const auto max = float64(style::ConvertScale(kMagnetMax));
	for (const auto &owned : Magnets) {
		const auto magnet = owned.get();
		const auto widget = magnet->widget.data();
		if (!widget->isVisible()) {
			continue;
		} else if (!enabled) {
			magnet->back.stop();
			ApplyMagnet(magnet, QPoint());
			continue;
		}
		const auto center = widget->mapToGlobal(widget->rect().center())
			- magnet->applied;
		const auto delta = QPointF(cursor - center);
		const auto distance = std::hypot(delta.x(), delta.y());
		if (distance < reach) {
			magnet->back.stop();
			auto offset = delta * kMagnetStrength;
			const auto length = std::hypot(offset.x(), offset.y());
			if (length > max) {
				offset *= max / length;
			}
			ApplyMagnet(magnet, offset.toPoint());
		} else if (!magnet->applied.isNull() && !magnet->back.animating()) {
			const auto from = QPointF(magnet->applied);
			magnet->back.start([=] {
				const auto progress = magnet->back.value(1.);
				ApplyMagnet(magnet, (from * (1. - progress)).toPoint());
			}, 0., 1., kMagnetReturnDuration, anim::easeOutBack);
		}
	}
}

void HandleMagnetMove(not_null<QWidget*> widget) {
	for (const auto &owned : Magnets) {
		const auto magnet = owned.get();
		if (magnet->widget.data() == widget.get() && !magnet->applying) {
			magnet->back.stop();
			magnet->base = widget->pos();
			magnet->applied = QPoint();
		}
	}
}

void StepParallax() {
	ParallaxCurrent += (ParallaxTarget - ParallaxCurrent) * kParallaxFollow;
	const auto rounded = ParallaxCurrent.toPoint();
	if (rounded != ParallaxApplied) {
		ParallaxApplied = rounded;
		if (const auto canvas = Canvas.data()) {
			canvas->update();
		}
	}
	const auto left = ParallaxTarget - ParallaxCurrent;
	if (std::abs(left.x()) < kParallaxEpsilon
		&& std::abs(left.y()) < kParallaxEpsilon) {
		ParallaxCurrent = ParallaxTarget;
		ParallaxAnimation->stop();
	}
}

void UpdateParallax(QPoint cursor) {
	const auto canvas = Canvas.data();
	if (!canvas || !canvas->isVisible() || !EffectsEnabled()) {
		return;
	}
	const auto size = canvas->size();
	if (size.width() <= 0 || size.height() <= 0) {
		return;
	}
	const auto local = canvas->mapFromGlobal(cursor);
	const auto halfWidth = size.width() / 2.;
	const auto halfHeight = size.height() / 2.;
	const auto x = std::clamp((local.x() - halfWidth) / halfWidth, -1., 1.);
	const auto y = std::clamp((local.y() - halfHeight) / halfHeight, -1., 1.);
	const auto margin = ParallaxMargin();
	ParallaxTarget = QPointF(-x * margin, -y * margin);
	if (!ParallaxAnimation) {
		ParallaxAnimation = std::make_unique<Ui::Animations::Basic>([] {
			StepParallax();
		});
	}
	if (!ParallaxAnimation->animating()) {
		ParallaxAnimation->start();
	}
}

[[nodiscard]] float64 WaveProgress(crl::time now) {
	return std::clamp(
		float64(now - CurrentWave.started) / kWaveDuration,
		0.,
		1.);
}

[[nodiscard]] float64 WaveRadius(float64 progress) {
	const auto canvas = Canvas.data();
	if (!canvas) {
		return 0.;
	}
	const auto reach = std::hypot(canvas->width(), canvas->height())
		* kWaveReach;
	return reach * (1. - std::pow(1. - progress, 3.));
}

[[nodiscard]] QRect WaveRect(float64 progress) {
	const auto outer = int(std::ceil(WaveRadius(progress)))
		+ style::ConvertScale(kWaveWidth);
	return QRect(
		CurrentWave.center - QPoint(outer, outer),
		QSize(2 * outer, 2 * outer));
}

void StepWave() {
	const auto canvas = Canvas.data();
	const auto previous = CurrentWave.painted;
	const auto now = crl::now();
	const auto progress = WaveProgress(now);
	if (!canvas || progress >= 1.) {
		CurrentWave.started = 0;
		CurrentWave.painted = QRect();
		WaveAnimation->stop();
		if (canvas && !previous.isEmpty()) {
			canvas->update(previous);
		}
		return;
	}
	const auto ring = WaveRect(progress).intersected(canvas->rect());
	CurrentWave.painted = ring;
	canvas->update(previous.united(ring));
}

[[nodiscard]] not_null<LiveState*> ResolveLive(not_null<QWidget*> widget) {
	const auto raw = widget.get();
	const auto i = LiveStates.find(raw);
	if (i != end(LiveStates)) {
		return i->second.get();
	}
	const auto state = LiveStates.emplace(
		raw,
		std::make_unique<LiveState>()).first->second.get();
	QObject::connect(raw, &QObject::destroyed, [=] {
		LiveStates.remove(raw);
	});
	state->animation = std::make_unique<Ui::Animations::Basic>([=] {
		const auto now = crl::now();
		if (now - state->lastFrame < kLiveFrame) {
			return;
		}
		state->lastFrame = now;
		auto region = QRegion();
		for (auto i = begin(state->entries); i != end(state->entries);) {
			if (now - i->second.painted > kLiveStale) {
				i = state->entries.erase(i);
			} else {
				region += i->second.area;
				++i;
			}
		}
		if (region.isEmpty()) {
			state->animation->stop();
			return;
		}
		raw->update(region);
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
			if (e->type() == QEvent::MouseMove) {
				const auto cursor = QCursor::pos();
				if (cursor != LastCursor) {
					LastCursor = cursor;
					UpdateMagnets(cursor);
					UpdateParallax(cursor);
				}
			}
			break;
		case QEvent::Move:
			if (!Magnets.empty()) {
				if (const auto widget = qobject_cast<QWidget*>(object)) {
					HandleMagnetMove(widget);
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
	BurstAt(source, source->rect().center(), kind);
}

void BurstAt(not_null<QWidget*> source, QPoint position, BurstKind kind) {
	if (!EffectsEnabled() || !source->isVisible()) {
		return;
	}
	const auto window = source->window();
	if (!window) {
		return;
	}
	const auto center = source->mapTo(window, position);
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

float64 FlightEase(float64 progress) {
	const auto t = std::clamp(progress, 0., 1.) - 1.;
	return 1.
		+ (kBackOvershoot + 1.) * t * t * t
		+ kBackOvershoot * t * t;
}

void SetupPowerMode(not_null<Ui::InputField*> field, Fn<void()> shake) {
	struct State {
		int length = 0;
		std::vector<crl::time> strokes;
		crl::time lastShake = 0;
	};
	const auto state = field->lifetime().make_state<State>();
	state->length = int(field->getLastText().size());
	field->changes(
	) | rpl::on_next([=] {
		const auto length = int(field->getLastText().size());
		const auto grown = (length > state->length);
		state->length = length;
		const auto edit = field->rawTextEdit();
		if (!grown || !EffectsEnabled() || !edit->hasFocus()) {
			return;
		}
		BurstAt(
			edit->viewport(),
			edit->cursorRect().center(),
			BurstKind::Typing);
		const auto now = crl::now();
		auto &strokes = state->strokes;
		strokes.push_back(now);
		strokes.erase(ranges::remove_if(strokes, [&](crl::time stroke) {
			return (now - stroke) > kPowerWindow;
		}), end(strokes));
		if (shake
			&& (int(strokes.size()) >= kPowerKeystrokes)
			&& (now - state->lastShake) >= kPowerShakeCooldown) {
			state->lastShake = now;
			shake();
		}
	}, field->lifetime());
}

QPoint ShakeOffset(float64 progress) {
	if (progress <= 0. || progress >= 1.) {
		return QPoint();
	}
	const auto amplitude = style::ConvertScale(kShakeAmplitude)
		* (1. - progress);
	return QPoint(
		qRound(std::sin(progress * kPi * 8.) * amplitude),
		qRound(std::cos(progress * kPi * 6.) * amplitude * 0.5));
}

void MakeMagnetic(QWidget *widget) {
	if (!widget) {
		return;
	}
	for (const auto &magnet : Magnets) {
		if (magnet->widget.data() == widget) {
			return;
		}
	}
	auto magnet = std::make_unique<Magnet>();
	magnet->widget = widget;
	magnet->base = widget->pos();
	Magnets.push_back(std::move(magnet));
}

void SetWallpaperCanvas(not_null<QWidget*> canvas) {
	Canvas = canvas.get();
}

bool IsWallpaperCanvas(QSize fill) {
	const auto canvas = Canvas.data();
	return canvas && WebLayout() && (canvas->size() == fill);
}

int ParallaxMargin() {
	return EffectsEnabled() ? style::ConvertScale(kParallaxMargin) : 0;
}

QPoint ParallaxOffset() {
	return EffectsEnabled() ? ParallaxApplied : QPoint();
}

void StartWallWave(not_null<QWidget*> source, QPoint position) {
	const auto canvas = Canvas.data();
	if (!canvas
		|| !EffectsEnabled()
		|| !canvas->isAncestorOf(source)
		|| !source->window()->isActiveWindow()) {
		return;
	}
	const auto now = crl::now();
	if (CurrentWave.started || (now - LastWave) < kWaveCooldown) {
		return;
	}
	LastWave = now;
	CurrentWave = Wave{
		.center = source->mapTo(canvas, position),
		.started = now,
	};
	if (!WaveAnimation) {
		WaveAnimation = std::make_unique<Ui::Animations::Basic>([] {
			StepWave();
		});
	}
	WaveAnimation->start();
}

void PaintWallWave(QPainter &p, QRect clip) {
	if (!CurrentWave.started) {
		return;
	}
	const auto progress = WaveProgress(crl::now());
	if (progress >= 1.) {
		return;
	}
	const auto radius = WaveRadius(progress);
	const auto width = float64(style::ConvertScale(kWaveWidth));
	const auto outer = radius + width / 2.;
	if (outer <= 0.) {
		return;
	}
	const auto inner = std::max(radius - width / 2., 0.) / outer;
	const auto middle = radius / outer;
	const auto color = st::windowBgActive->c;
	const auto alpha = kWaveAlpha * (1. - progress);
	auto gradient = QRadialGradient(QPointF(CurrentWave.center), outer);
	gradient.setColorAt(0., WithAlpha(color, 0.));
	gradient.setColorAt(inner, WithAlpha(color, 0.));
	gradient.setColorAt(middle, WithAlpha(color, alpha));
	gradient.setColorAt(1., WithAlpha(color, 0.));
	const auto area = WaveRect(progress).intersected(clip);
	if (area.isEmpty()) {
		return;
	}
	p.fillRect(area, gradient);
}

void PaintLiveUserpic(
		QPainter &p,
		const void *key,
		QRect userpic,
		LiveUserpic state) {
	if (state == LiveUserpic::None || !EffectsEnabled()) {
		return;
	}
	const auto widget = dynamic_cast<QWidget*>(p.device());
	if (!widget) {
		return;
	}
	const auto live = ResolveLive(widget);
	const auto now = crl::now();
	const auto gap = style::ConvertScale(kLiveGap);
	const auto line = float64(style::ConvertScale(kLiveWidth));
	const auto ring = QRectF(userpic).marginsAdded(
		QMarginsF(gap, gap, gap, gap));
	const auto extra = int(std::ceil(line));
	auto &entry = live->entries[key];
	entry.area = p.transform().mapRect(ring.toAlignedRect().marginsAdded(
		{ extra, extra, extra, extra }));
	entry.painted = now;
	if (!live->animation->animating()) {
		live->animation->start();
	}
	auto hq = PainterHighQualityEnabler(p);
	const auto accent = st::windowBgActive->c;
	p.setBrush(Qt::NoBrush);
	if (state == LiveUserpic::Online) {
		const auto phase = std::sin(
			2. * kPi * float64(now % kLivePeriod) / kLivePeriod);
		p.setPen(QPen(
			WithAlpha(accent, kLiveBaseAlpha + kLiveBreathAlpha * phase),
			line));
		p.drawEllipse(ring);
		return;
	}
	p.setPen(QPen(WithAlpha(accent, kLiveTrackAlpha), line));
	p.drawEllipse(ring);
	auto pen = QPen(accent, line);
	pen.setCapStyle(Qt::RoundCap);
	p.setPen(pen);
	const auto angle = 360. * float64(now % kLiveSpin) / kLiveSpin;
	p.drawArc(ring, int(-angle * 16), kLiveArcSpan * 16);
}

} // namespace AyuDesign
