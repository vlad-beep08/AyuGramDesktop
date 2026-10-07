#include "ayu/ui/settings/settings_effects.h"

#include "lang_auto.h"
#include "ayu/ayu_settings.h"
#include "ayu/ui/design/design_effects.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "base/battery_saving.h"
#include "boxes/peers/edit_peer_permissions_box.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "settings/sections/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "ui/effects/animations.h"
#include "ui/painter.h"
#include "ui/power_saving.h"
#include "ui/ui_utility.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

using AyuDesign::Effect;
using AyuDesign::EffectsLevel;

constexpr auto kLevels = 3;
constexpr auto kSliderHeight = 52;
constexpr auto kTrackTop = 14;
constexpr auto kTrackWidth = 2;
constexpr auto kDotRadius = 6;
constexpr auto kLabelTop = 30;
constexpr auto kTrackAlpha = 0.3;
constexpr auto kMoveDuration = crl::time(150);

class LevelSlider final : public Ui::RpWidget {
public:
	LevelSlider(
		QWidget *parent,
		std::vector<QString> labels,
		int active,
		Fn<void(int)> changed);

	void setActive(int index);

protected:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	int resizeGetHeight(int newWidth) override;

private:
	[[nodiscard]] int left() const;
	[[nodiscard]] int span() const;
	[[nodiscard]] int stopAt(int x) const;
	void choose(int index, bool notify);

	std::vector<QString> _labels;
	Fn<void(int)> _changed;
	int _active = 0;
	bool _pressed = false;
	Ui::Animations::Simple _move;

};

LevelSlider::LevelSlider(
	QWidget *parent,
	std::vector<QString> labels,
	int active,
	Fn<void(int)> changed)
: RpWidget(parent)
, _labels(std::move(labels))
, _changed(std::move(changed))
, _active(std::clamp(active, 0, kLevels - 1)) {
	setCursor(style::cur_pointer);
	resizeToWidth(width());
}

void LevelSlider::setActive(int index) {
	choose(index, false);
}

int LevelSlider::left() const {
	return st::boxRowPadding.left() + kDotRadius;
}

int LevelSlider::span() const {
	return std::max(width() - 2 * left(), 1);
}

int LevelSlider::stopAt(int x) const {
	const auto step = float64(span()) / (kLevels - 1);
	const auto index = int(std::round((x - left()) / step));
	return std::clamp(index, 0, kLevels - 1);
}

void LevelSlider::choose(int index, bool notify) {
	index = std::clamp(index, 0, kLevels - 1);
	if (index == _active) {
		return;
	}
	const auto from = _move.value(_active);
	_active = index;
	_move.start([=] { update(); }, from, index, kMoveDuration);
	update();
	if (notify && _changed) {
		_changed(index);
	}
}

int LevelSlider::resizeGetHeight(int) {
	return style::ConvertScale(kSliderHeight);
}

void LevelSlider::paintEvent(QPaintEvent *) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);
	const auto trackTop = style::ConvertScale(kTrackTop);
	const auto line = style::ConvertScale(kTrackWidth);
	const auto radius = style::ConvertScale(kDotRadius);
	const auto from = left();
	const auto till = from + span();
	const auto position = _move.value(_active) / (kLevels - 1);
	const auto dot = from + int(std::round(span() * position));

	auto track = st::windowSubTextFg->c;
	track.setAlphaF(kTrackAlpha);
	p.setPen(Qt::NoPen);
	p.setBrush(track);
	p.drawRoundedRect(
		QRect(from, trackTop - line / 2, till - from, line),
		line / 2.,
		line / 2.);
	p.setBrush(st::windowBgActive);
	p.drawRoundedRect(
		QRect(from, trackTop - line / 2, dot - from, line),
		line / 2.,
		line / 2.);
	p.drawEllipse(QPoint(dot, trackTop), radius, radius);

	const auto top = style::ConvertScale(kLabelTop);
	const auto outer = st::boxRowPadding.left();
	for (auto i = 0; i != int(_labels.size()) && i != kLevels; ++i) {
		const auto active = (i == _active);
		const auto &font = active ? st::semiboldFont : st::normalFont;
		p.setFont(font);
		p.setPen(active ? st::windowFg : st::windowSubTextFg);
		const auto &text = _labels[i];
		const auto textWidth = font->width(text);
		const auto stop = from + span() * i / (kLevels - 1);
		const auto x = (i == 0)
			? outer
			: (i == kLevels - 1)
			? (width() - outer - textWidth)
			: (stop - textWidth / 2);
		p.drawText(x, top + font->ascent, text);
	}
}

void LevelSlider::mousePressEvent(QMouseEvent *e) {
	_pressed = true;
	choose(stopAt(e->pos().x()), true);
}

void LevelSlider::mouseMoveEvent(QMouseEvent *e) {
	if (_pressed) {
		choose(stopAt(e->pos().x()), true);
	}
}

void LevelSlider::mouseReleaseEvent(QMouseEvent *) {
	_pressed = false;
}

struct EffectRow {
	Effect effect;
	rpl::producer<QString> title;
	QString id;
};

[[nodiscard]] std::vector<EffectRow> EffectRows() {
	return {
		{ Effect::Spotlight, tr::ayu_EffectSpotlight(), u"spotlight"_q },
		{ Effect::Burst, tr::ayu_EffectBurst(), u"burst"_q },
		{ Effect::Pulse, tr::ayu_EffectPulse(), u"pulse"_q },
		{ Effect::OnlineRing, tr::ayu_EffectOnlineRing(), u"onlineRing"_q },
		{ Effect::TypingArc, tr::ayu_EffectTypingArc(), u"typingArc"_q },
		{ Effect::Cascade, tr::ayu_EffectCascade(), u"cascade"_q },
		{ Effect::Peanuts, tr::ayu_EffectPeanuts(), u"peanuts"_q },
		{
			Effect::MessageAppear,
			tr::ayu_EffectMessageAppear(),
			u"messageAppear"_q,
		},
	};
}

void BuildLevel(SectionBuilder &builder) {
	builder.addSubsectionTitle({
		.id = u"ayu/effectsLevel"_q,
		.title = tr::ayu_EffectsLevelHeader(),
		.keywords = { u"animations"_q, u"effects"_q, u"performance"_q },
	});
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto slider = object_ptr<LevelSlider>(
			ctx.container,
			std::vector<QString>{
				tr::ayu_EffectsLevelSaving(tr::now),
				tr::ayu_EffectsLevelLight(tr::now),
				tr::ayu_EffectsLevelAll(tr::now),
			},
			AyuSettings::getInstance().designEffectsLevel(),
			[](int index) {
				AyuDesign::ApplyEffectsLevel(EffectsLevel(index));
			});
		const auto raw = slider.data();
		AyuSettings::getInstance().designEffectsLevelValue(
		) | rpl::on_next([=](int level) {
			raw->setActive(level);
		}, raw->lifetime());
		return { .widget = std::move(slider) };
	});
	builder.addSkip();
	builder.addDividerText(tr::ayu_EffectsLevelAbout());
	builder.addSkip();
}

void BuildAuto(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	ayu.addToggle({
		.id = u"ayu/effectsAuto"_q,
		.title = tr::ayu_EffectsAuto(),
		.getter = [] {
			return AyuSettings::getInstance().designEffectsAuto();
		},
		.setter = [](bool enabled) {
			AyuSettings::getInstance().setDesignEffectsAuto(enabled);
			Core::App().settings().setIgnoreBatterySavingValue(!enabled);
			Core::App().saveSettingsDelayed();
		},
		.keywords = { u"battery"_q, u"power"_q, u"laptop"_q },
	});
	builder.addSkip();
	builder.addDividerText(tr::ayu_EffectsAutoAbout());
	builder.addSkip();
}

void BuildEffects(SectionBuilder &builder) {
	builder.addSubsectionTitle(tr::ayu_DesignEffects());
	for (auto &row : EffectRows()) {
		const auto effect = row.effect;
		const auto button = builder.addButton({
			.id = u"ayu/effect/"_q + row.id,
			.title = std::move(row.title),
			.st = &st::settingsButtonNoIcon,
			.toggled = AyuSettings::getInstance().designEffectsFlagsValue(
			) | rpl::map([=](int flags) {
				return (flags & int(effect)) != 0;
			}),
			.keywords = { u"effects"_q },
		});
		if (button) {
			button->toggledChanges(
			) | rpl::on_next([=](bool enabled) {
				AyuDesign::SetEffectEnabled(effect, enabled);
			}, button->lifetime());
		}
	}
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();
}

void BuildResources(SectionBuilder &builder) {
	builder.addSubsectionTitle({
		.id = u"ayu/effectsResources"_q,
		.title = tr::ayu_EffectsResourceHeader(),
		.keywords = { u"stickers"_q, u"emoji"_q, u"autoplay"_q },
	});
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto holder = object_ptr<Ui::VerticalLayout>(ctx.container);
		const auto raw = holder.data();
		const auto rebuild = [=] {
			raw->clear();
			auto disabled = rpl::combine(
				Core::App().batterySaving().value(),
				Core::App().settings().ignoreBatterySavingValue()
			) | rpl::map([](bool saving, bool ignore) {
				return (saving && !ignore)
					? tr::lng_settings_power_turn_off(tr::now)
					: QString();
			});
			auto control = CreateEditPowerSaving(
				raw,
				PowerSaving::kAll & ~PowerSaving::Current(),
				std::move(disabled));
			const auto widget = raw->add(std::move(control.widget));
			const auto value = control.value;
			std::move(
				control.changes
			) | rpl::on_next([=] {
				PowerSaving::Set(PowerSaving::kAll & ~value());
				Core::App().saveSettingsDelayed();
			}, widget->lifetime());
		};
		rebuild();
		AyuSettings::getInstance().designEffectsLevelValue(
		) | rpl::skip(1) | rpl::on_next(rebuild, raw->lifetime());
		return { .widget = std::move(holder) };
	});
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuEffects::Id(),
	.parentId = MainId(),
	.title = &tr::ayu_EffectsPageTitle,
	.icon = &st::menuIconPowerUsage,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	builder.addSkip();
	BuildLevel(builder);
	BuildAuto(builder, ayu);
	BuildEffects(builder);
	BuildResources(builder);
});

} // namespace

rpl::producer<QString> AyuEffects::title() {
	return tr::ayu_EffectsPageTitle();
}

AyuEffects::AyuEffects(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuEffects::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuEffectsId() {
	return AyuEffects::Id();
}

} // namespace Settings
