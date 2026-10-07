// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_effects.h"

#include "ayu/ayu_settings.h"
#include "base/battery_saving.h"
#include "core/application.h"
#include "ui/power_saving.h"
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
#include <QtCore/QThread>
#include <QtGui/QCursor>
#include <QtGui/QRadialGradient>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTextEdit>

#include <array>
#include <random>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace AyuDesign {
namespace {

constexpr auto kPi = 3.14159265358979323846;
constexpr auto kLightEffects = int(Effect::Spotlight)
	| int(Effect::Burst)
	| int(Effect::Pulse)
	| int(Effect::OnlineRing)
	| int(Effect::MessageAppear);
constexpr auto kAllEffects = 0xFF;
constexpr auto kWeakThreads = 4;
constexpr auto kAutoRecheck = crl::time(5000);
constexpr auto kSpotRadius = 150;
constexpr auto kSpotAlpha = 0.22;
constexpr auto kSpotActiveAlpha = 0.14;
constexpr auto kSpotButtonAlpha = 0.12;
constexpr auto kBurstDuration = crl::time(650);
constexpr auto kAlarmReach = 120;
constexpr auto kAlarmParticles = 30;
constexpr auto kParticleMin = 2;
constexpr auto kParticleExtra = 3;
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
constexpr auto kNutsPerStroke = 2;
constexpr auto kNutBumps = 3;
constexpr auto kNutDuration = crl::time(760);
constexpr auto kNutSizeMin = 9;
constexpr auto kNutSizeExtra = 3;
constexpr auto kNutAspect = 1.2;
constexpr auto kNutSpreadX = 55;
constexpr auto kNutLiftMin = 35;
constexpr auto kNutLiftExtra = 40;
constexpr auto kNutGravity = 170;
constexpr auto kNutSpin = 420.;
constexpr auto kNutFadeFrom = 0.65;
constexpr auto kNutBumpSize = 0.22;
constexpr auto kNutShineAlpha = 150;
constexpr auto kLiveSpin = crl::time(1100);
constexpr auto kLiveFrame = crl::time(66);
constexpr auto kLiveStale = crl::time(1000);
constexpr auto kLiveGap = 3;
constexpr auto kLiveWidth = 2;
constexpr auto kLiveOnlineAlpha = 0.55;
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

[[nodiscard]] BurstConfig AlarmConfig() {
	return {
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
}

class BurstOverlay final : public Ui::RpWidget {
public:
	BurstOverlay(not_null<QWidget*> parent, QPoint center);

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	const BurstConfig _config;
	QPointF _center;
	std::vector<Particle> _particles;
	Ui::Animations::Simple _animation;

};

BurstOverlay::BurstOverlay(not_null<QWidget*> parent, QPoint center)
: RpWidget(parent)
, _config(AlarmConfig()) {
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
		const auto window = raw->window();
		if (!window || !window->isActiveWindow()) {
			state->animation->stop();
			return;
		}
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

struct Nut {
	QPointF origin;
	QPointF velocity;
	float64 size = 0.;
	float64 angle = 0.;
	float64 spin = 0.;
	crl::time started = 0;
	std::array<QPointF, kNutBumps> bumps;
};

[[nodiscard]] std::minstd_rand &NutRandom() {
	static auto result = std::minstd_rand(uint32(crl::now()));
	return result;
}

void PaintNut(QPainter &p, const Nut &nut, QPointF center, float64 t) {
	const auto width = nut.size * kNutAspect;
	const auto height = nut.size;
	const auto opacity = (t < kNutFadeFrom)
		? 1.
		: std::max(1. - (t - kNutFadeFrom) / (1. - kNutFadeFrom), 0.);
	p.save();
	p.setOpacity(opacity);
	p.translate(center);
	p.rotate(nut.angle + nut.spin * t);
	auto gradient = QRadialGradient(
		QPointF(-width * 0.2, -height * 0.25),
		width * 0.8);
	gradient.setColorAt(0., QColor(0xD9, 0xF2, 0x84));
	gradient.setColorAt(0.5, QColor(0x93, 0xC8, 0x3E));
	gradient.setColorAt(1., QColor(0x56, 0x86, 0x1F));
	p.setPen(Qt::NoPen);
	p.setBrush(gradient);
	p.drawEllipse(QRectF(-width / 2., -height / 2., width, height));
	for (const auto &bump : nut.bumps) {
		p.drawEllipse(
			QPointF(bump.x() * width / 2., bump.y() * height / 2.),
			height * kNutBumpSize,
			height * kNutBumpSize);
	}
	p.setBrush(QColor(255, 255, 255, kNutShineAlpha));
	p.drawEllipse(QRectF(
		-width * 0.3,
		-height * 0.34,
		width * 0.26,
		height * 0.2));
	p.restore();
}

class NutLayer final : public Ui::RpWidget {
public:
	explicit NutLayer(not_null<QWidget*> window);

	void spawn(QPoint center);

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	[[nodiscard]] QPointF position(const Nut &nut, float64 t) const;
	[[nodiscard]] QRect area(crl::time now) const;
	void step(crl::time now);

	std::vector<Nut> _nuts;
	QRect _painted;
	Ui::Animations::Basic _animation;

};

NutLayer::NutLayer(not_null<QWidget*> window)
: RpWidget(window)
, _animation([=](crl::time now) { step(now); }) {
	setAttribute(Qt::WA_TransparentForMouseEvents);
	hide();
}

void NutLayer::spawn(QPoint center) {
	auto &random = NutRandom();
	auto unit = std::uniform_real_distribution<float64>(0., 1.);
	auto sign = std::uniform_real_distribution<float64>(-1., 1.);
	const auto now = crl::now();
	for (auto i = 0; i != kNutsPerStroke; ++i) {
		auto nut = Nut{
			.origin = QPointF(center),
			.velocity = QPointF(
				sign(random) * style::ConvertScale(kNutSpreadX),
				-style::ConvertScale(kNutLiftMin)
					- unit(random) * style::ConvertScale(kNutLiftExtra)),
			.size = style::ConvertScale(kNutSizeMin)
				+ unit(random) * style::ConvertScale(kNutSizeExtra),
			.angle = unit(random) * 360.,
			.spin = sign(random) * kNutSpin,
			.started = now,
		};
		for (auto &bump : nut.bumps) {
			const auto angle = unit(random) * 2. * kPi;
			bump = QPointF(std::cos(angle), std::sin(angle)) * 0.8;
		}
		_nuts.push_back(nut);
	}
	setGeometry(parentWidget()->rect());
	show();
	raise();
	if (!_animation.animating()) {
		_animation.start();
	}
}

QPointF NutLayer::position(const Nut &nut, float64 t) const {
	return nut.origin
		+ nut.velocity * t
		+ QPointF(0., style::ConvertScale(kNutGravity) * t * t);
}

QRect NutLayer::area(crl::time now) const {
	auto result = QRect();
	for (const auto &nut : _nuts) {
		const auto t = std::clamp(
			float64(now - nut.started) / kNutDuration,
			0.,
			1.);
		const auto reach = int(std::ceil(nut.size * kNutAspect)) + 2;
		const auto center = position(nut, t).toPoint();
		result = result.united(QRect(
			center - QPoint(reach, reach),
			QSize(2 * reach, 2 * reach)));
	}
	return result;
}

void NutLayer::step(crl::time now) {
	_nuts.erase(ranges::remove_if(_nuts, [&](const Nut &nut) {
		return (now - nut.started) >= kNutDuration;
	}), end(_nuts));
	const auto current = area(now);
	update(_painted.united(current));
	_painted = current;
	if (_nuts.empty()) {
		_animation.stop();
		_painted = QRect();
		hide();
	}
}

void NutLayer::paintEvent(QPaintEvent *e) {
	if (_nuts.empty()) {
		return;
	}
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	const auto now = crl::now();
	for (const auto &nut : _nuts) {
		const auto t = std::clamp(
			float64(now - nut.started) / kNutDuration,
			0.,
			1.);
		PaintNut(p, nut, position(nut, t), t);
	}
}

void SpawnNuts(not_null<QWidget*> source, QPoint position) {
	const auto window = source->window();
	if (!window) {
		return;
	}
	static auto layers = base::flat_map<QWidget*, QPointer<NutLayer>>();
	auto &layer = layers[window];
	if (!layer) {
		layer = Ui::CreateChild<NutLayer>(window);
	}
	layer->spawn(source->mapTo(window, position));
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
				if (EffectOn(Effect::Spotlight)
					&& dynamic_cast<Ui::SettingsButton*>(widget)) {
					widget->update();
				}
			}
			break;
		case QEvent::Paint:
			if (!EffectOn(Effect::Spotlight)) {
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

[[nodiscard]] bool OnBattery() {
#ifdef Q_OS_WIN
	auto status = SYSTEM_POWER_STATUS();
	if (GetSystemPowerStatus(&status) && !(status.BatteryFlag & 128)) {
		return (status.ACLineStatus == 0);
	}
#endif
	return false;
}

[[nodiscard]] bool PreferLightEffects() {
	static auto checked = crl::time(0);
	static auto light = false;
	const auto now = crl::now();
	if (!checked || now - checked >= kAutoRecheck) {
		checked = now;
		light = (QThread::idealThreadCount() <= kWeakThreads)
			|| OnBattery()
			|| Core::App().batterySaving().enabled().value_or(false);
	}
	return light;
}

[[nodiscard]] PowerSaving::Flags PowerSavingPreset(EffectsLevel level) {
	switch (level) {
	case EffectsLevel::Saving: return PowerSaving::kAll;
	case EffectsLevel::Light: return PowerSaving::kChatBackground
		| PowerSaving::kChatEffects
		| PowerSaving::kCalls;
	case EffectsLevel::Full: return PowerSaving::Flags();
	}
	return PowerSaving::Flags();
}

} // namespace

bool EffectOn(Effect effect) {
	if (DurationMs(Duration::Normal) <= 0) {
		return false;
	}
	const auto &settings = AyuSettings::getInstance();
	auto mask = settings.designEffectsFlags();
	if (settings.designEffectsAuto() && PreferLightEffects()) {
		mask &= kLightEffects;
	}
	return (mask & int(effect)) != 0;
}

int EffectsPreset(EffectsLevel level) {
	switch (level) {
	case EffectsLevel::Saving: return 0;
	case EffectsLevel::Light: return kLightEffects;
	case EffectsLevel::Full: return kAllEffects;
	}
	return kAllEffects;
}

void ApplyEffectsLevel(EffectsLevel level) {
	PowerSaving::Set(PowerSavingPreset(level));
	Core::App().saveSettingsDelayed();
	auto &settings = AyuSettings::getInstance();
	settings.setDesignEffectsFlags(EffectsPreset(level));
	settings.setDesignEffectsLevel(int(level));
}

void SetEffectEnabled(Effect effect, bool enabled) {
	auto &settings = AyuSettings::getInstance();
	const auto flags = settings.designEffectsFlags();
	settings.setDesignEffectsFlags(enabled
		? (flags | int(effect))
		: (flags & ~int(effect)));
}

void SetupEffects() {
	static auto installed = false;
	if (installed) {
		return;
	}
	installed = true;
	qApp->installEventFilter(new EffectsFilter(qApp));
}

void PaintWebRowSpotlight(QPainter &p, QRect row, bool active) {
	if (!EffectOn(Effect::Spotlight)) {
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

void Burst(not_null<QWidget*> source) {
	if (!EffectOn(Effect::Burst) || !source->isVisible()) {
		return;
	}
	const auto window = source->window();
	if (!window) {
		return;
	}
	const auto center = source->mapTo(window, source->rect().center());
	Ui::CreateChild<BurstOverlay>(window, center);
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
	if (count > entry.count && EffectOn(Effect::Pulse)) {
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

void SetupPowerMode(not_null<Ui::InputField*> field) {
	struct State {
		int length = 0;
	};
	const auto state = field->lifetime().make_state<State>();
	state->length = int(field->getLastText().size());
	field->changes(
	) | rpl::on_next([=] {
		const auto length = int(field->getLastText().size());
		const auto grown = (length > state->length);
		state->length = length;
		const auto edit = field->rawTextEdit();
		if (!grown || !EffectOn(Effect::Peanuts) || !edit->hasFocus()) {
			return;
		}
		SpawnNuts(edit->viewport(), edit->cursorRect().center());
	}, field->lifetime());
}

void PaintLiveUserpic(
		QPainter &p,
		const void *key,
		QRect userpic,
		LiveUserpic state) {
	if (state == LiveUserpic::None) {
		return;
	}
	const auto arc = (state == LiveUserpic::Typing)
		&& EffectOn(Effect::TypingArc);
	if (!arc && !EffectOn(Effect::OnlineRing)) {
		return;
	}
	const auto gap = style::ConvertScale(kLiveGap);
	const auto line = float64(style::ConvertScale(kLiveWidth));
	const auto ring = QRectF(userpic).marginsAdded(
		QMarginsF(gap, gap, gap, gap));
	auto hq = PainterHighQualityEnabler(p);
	const auto accent = st::windowBgActive->c;
	p.setBrush(Qt::NoBrush);
	if (!arc) {
		p.setPen(QPen(WithAlpha(accent, kLiveOnlineAlpha), line));
		p.drawEllipse(ring);
		return;
	}
	const auto widget = dynamic_cast<QWidget*>(p.device());
	const auto now = crl::now();
	if (widget) {
		const auto live = ResolveLive(widget);
		const auto extra = int(std::ceil(line));
		auto &entry = live->entries[key];
		entry.area = p.transform().mapRect(
			ring.toAlignedRect().marginsAdded(
				{ extra, extra, extra, extra }));
		entry.painted = now;
		if (!live->animation->animating()
			&& widget->window()
			&& widget->window()->isActiveWindow()) {
			live->animation->start();
		}
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
