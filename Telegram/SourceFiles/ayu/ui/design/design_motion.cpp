// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_motion.h"

#include "ayu/ui/design/design_system.h"
#include "ui/effects/animations.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"

namespace AyuDesign {
namespace {

struct HoverEntry {
	bool hovered = false;
	float64 from = 0.;
	crl::time started = 0;
};

struct HoverState {
	base::flat_map<const void*, HoverEntry> entries;
	std::unique_ptr<Ui::Animations::Basic> animation;
};

base::flat_map<QWidget*, std::unique_ptr<HoverState>> HoverStates;

[[nodiscard]] float64 HoverProgress(
		const HoverEntry &entry,
		crl::time now,
		crl::time duration) {
	const auto passed = std::clamp(
		float64(now - entry.started) / duration,
		0.,
		1.);
	return entry.hovered
		? (entry.from + (1. - entry.from) * passed)
		: (entry.from * (1. - passed));
}

[[nodiscard]] not_null<HoverState*> ResolveHoverState(
		not_null<QWidget*> widget,
		crl::time duration) {
	const auto raw = widget.get();
	auto i = HoverStates.find(raw);
	if (i != end(HoverStates)) {
		return i->second.get();
	}
	i = HoverStates.emplace(raw, std::make_unique<HoverState>()).first;
	const auto state = i->second.get();
	QObject::connect(raw, &QObject::destroyed, [=] {
		HoverStates.remove(raw);
	});
	state->animation = std::make_unique<Ui::Animations::Basic>([=] {
		raw->update();
		const auto now = crl::now();
		auto animating = false;
		for (auto j = state->entries.begin(); j != state->entries.end();) {
			if (now - j->second.started < duration) {
				animating = true;
				++j;
			} else if (!j->second.hovered) {
				j = state->entries.erase(j);
			} else {
				++j;
			}
		}
		if (!animating) {
			state->animation->stop();
		}
	});
	return state;
}

class FadeOverlay final : public Ui::RpWidget {
public:
	FadeOverlay(
		not_null<QWidget*> parent,
		QPixmap snapshot,
		QRect geometry,
		crl::time duration)
	: RpWidget(parent)
	, _snapshot(std::move(snapshot)) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setGeometry(geometry);
		show();
		raise();
		_animation.start([=] {
			update();
			if (!_animation.animating()) {
				deleteLater();
			}
		}, 1., 0., duration, anim::easeOutCubic);
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		p.setOpacity(_animation.value(0.));
		p.drawPixmap(0, 0, _snapshot);
	}

private:
	QPixmap _snapshot;
	Ui::Animations::Simple _animation;

};

class SlideOverlay final : public Ui::RpWidget {
public:
	SlideOverlay(
		not_null<QWidget*> parent,
		QImage snapshot,
		QRect geometry,
		crl::time duration)
	: RpWidget(parent)
	, _snapshot(std::move(snapshot))
	, _from(geometry.x())
	, _to(parent->width()) {
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setGeometry(geometry);
		show();
		raise();
		_animation.start([=] {
			move(anim::interpolate(_from, _to, _animation.value(1.)), y());
			if (!_animation.animating()) {
				deleteLater();
			}
		}, 0., 1., duration, anim::easeOutCubic);
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		auto p = QPainter(this);
		p.drawImage(0, 0, _snapshot);
	}

private:
	QImage _snapshot;
	int _from = 0;
	int _to = 0;
	Ui::Animations::Simple _animation;

};

[[nodiscard]] QImage RoundSnapshot(QPixmap snapshot, int radius) {
	auto result = snapshot.toImage().convertToFormat(
		QImage::Format_ARGB32_Premultiplied);
	result.setDevicePixelRatio(snapshot.devicePixelRatio());
	auto p = QPainter(&result);
	auto hq = PainterHighQualityEnabler(p);
	const auto area = QRectF(
		QPointF(),
		QSizeF(snapshot.size()) / snapshot.devicePixelRatio());
	auto outside = QPainterPath();
	outside.addRect(area);
	auto inside = QPainterPath();
	inside.addRoundedRect(area, radius, radius);
	p.setCompositionMode(QPainter::CompositionMode_Clear);
	p.fillPath(outside.subtracted(inside), Qt::black);
	p.end();
	return result;
}

} // namespace

float64 HoverValue(QPainter &p, const void *key, bool hovered) {
	const auto duration = DurationMs(Duration::Fast);
	const auto widget = dynamic_cast<QWidget*>(p.device());
	if (!widget || duration <= 0) {
		return hovered ? 1. : 0.;
	}
	const auto state = ResolveHoverState(widget, duration);
	const auto now = crl::now();
	auto i = state->entries.find(key);
	if (i == end(state->entries)) {
		if (!hovered) {
			return 0.;
		}
		i = state->entries.emplace(key, HoverEntry{
			.hovered = true,
			.from = 0.,
			.started = now,
		}).first;
		state->animation->start();
	} else if (i->second.hovered != hovered) {
		i->second = HoverEntry{
			.hovered = hovered,
			.from = HoverProgress(i->second, now, duration),
			.started = now,
		};
		if (!state->animation->animating()) {
			state->animation->start();
		}
	}
	return HoverProgress(i->second, now, duration);
}

void CrossFade(not_null<QWidget*> target) {
	const auto duration = DurationMs(Duration::Normal);
	const auto parent = target->parentWidget();
	if (duration <= 0
		|| !parent
		|| target->isHidden()
		|| target->width() <= 0
		|| target->height() <= 0) {
		return;
	}
	Ui::CreateChild<FadeOverlay>(
		parent,
		Ui::GrabWidget(target),
		target->geometry(),
		duration);
}

void SlideOut(
		not_null<QWidget*> parent,
		QPixmap snapshot,
		QRect geometry,
		int radius) {
	const auto duration = DurationMs(Duration::Slow);
	if (duration <= 0 || snapshot.isNull() || geometry.isEmpty()) {
		return;
	}
	Ui::CreateChild<SlideOverlay>(
		parent.get(),
		RoundSnapshot(std::move(snapshot), radius),
		geometry,
		duration);
}

} // namespace AyuDesign
