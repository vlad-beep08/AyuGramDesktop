// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_glass.h"

#include "ayu/ui/design/design_islands.h"
#include "ayu/ui/design/design_system.h"
#include "chat_helpers/field_autocomplete.h"
#include "chat_helpers/tabbed_panel.h"
#include "chat_helpers/tabbed_selector.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"
#include "ui/widgets/popup_menu.h"
#include "styles/palette.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_widgets.h"

#include <QtCore/QTimer>
#include <QtGui/QPainterPath>
#include <QtWidgets/QApplication>

#include <typeinfo>

namespace AyuDesign {
namespace {

constexpr auto kBlurRadius = 24;
constexpr auto kTintAlpha = 0.78;
constexpr auto kHoverAlpha = 0.08;
constexpr auto kRippleAlpha = 0.12;
constexpr auto kSheenAlpha = 0.07;
constexpr auto kShadowOpacity = 0.18;
constexpr auto kDarkBorderAlpha = 0.1;
constexpr auto kLightBorderAlpha = 0.08;
constexpr auto kDarkLightness = 128;
constexpr auto kSurfaceRefreshDelay = 120;

auto Refreshing = false;

[[nodiscard]] QColor WithAlpha(QColor color, float64 alpha) {
	color.setAlphaF(alpha);
	return color;
}

struct GlassColors {
	style::complex_color clear;
	style::complex_color over;
	style::complex_color ripple;
};

[[nodiscard]] GlassColors &Colors() {
	static auto result = GlassColors{
		.clear = style::complex_color([] {
			return QColor(0, 0, 0, 0);
		}),
		.over = style::complex_color([] {
			return WithAlpha(st::windowFg->c, kHoverAlpha);
		}),
		.ripple = style::complex_color([] {
			return WithAlpha(st::windowFg->c, kRippleAlpha);
		}),
	};
	return result;
}

[[nodiscard]] QImage Blurred(QImage image, QRect crop) {
	const auto ratio = style::DevicePixelRatio();
	const auto blur = style::ConvertScale(kBlurRadius);
	image = Images::BlurLargeImage(std::move(image), blur * ratio);
	auto result = image.copy(QRect(
		crop.topLeft() * ratio,
		crop.size() * ratio));
	result.setDevicePixelRatio(ratio);
	return result;
}

void PaintGlass(
		QPainter &p,
		QRect rect,
		int radius,
		const QImage &blurred,
		const style::color &tint) {
	auto hq = PainterHighQualityEnabler(p);
	auto path = QPainterPath();
	path.addRoundedRect(QRectF(rect), radius, radius);
	p.save();
	p.setClipPath(path, Qt::IntersectClip);
	if (!blurred.isNull()) {
		p.drawImage(rect.topLeft(), blurred);
	}
	const auto alpha = blurred.isNull() ? 1. : kTintAlpha;
	p.fillRect(rect, WithAlpha(tint->c, alpha));
	auto sheen = QLinearGradient(
		rect.topLeft(),
		QPoint(rect.x(), rect.y() + rect.height() / 2));
	sheen.setColorAt(0., WithAlpha(QColor(255, 255, 255), kSheenAlpha));
	sheen.setColorAt(1., QColor(255, 255, 255, 0));
	p.fillRect(rect, sheen);
	p.restore();

	const auto dark = (st::windowBg->c.lightness() < kDarkLightness);
	const auto border = dark
		? WithAlpha(QColor(255, 255, 255), kDarkBorderAlpha)
		: WithAlpha(QColor(0, 0, 0), kLightBorderAlpha);
	const auto line = st::lineWidth;
	const auto half = line / 2.;
	p.setPen(QPen(border, line));
	p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(
		QRectF(rect).adjusted(half, half, -half, -half),
		std::max(radius - half, 0.),
		std::max(radius - half, 0.));
}

class Backdrop final : public Ui::RpWidget {
public:
	explicit Backdrop(not_null<Ui::PopupMenu*> menu);

	void refresh();

protected:
	void paintEvent(QPaintEvent *e) override;

private:
	const not_null<Ui::PopupMenu*> _menu;
	QImage _blurred;
	QRect _captured;

};

[[nodiscard]] base::flat_map<QWidget*, Backdrop*> &Menus() {
	static auto result = base::flat_map<QWidget*, Backdrop*>();
	return result;
}

Backdrop::Backdrop(not_null<Ui::PopupMenu*> menu)
: RpWidget(menu)
, _menu(menu) {
	setAttribute(Qt::WA_TransparentForMouseEvents);
	menu->sizeValue(
	) | rpl::on_next([=](QSize size) {
		setGeometry(QRect(QPoint(), size));
		lower();
	}, lifetime());
	show();
}

void Backdrop::refresh() {
	const auto inner = _menu->inner();
	if (Refreshing || inner.isEmpty()) {
		return;
	}
	const auto global = QRect(
		_menu->mapToGlobal(inner.topLeft()),
		inner.size());
	if (global == _captured && !_blurred.isNull()) {
		return;
	}
	_captured = global;
	const auto parent = _menu->parentWidget();
	auto source = parent ? parent->window() : nullptr;
	if (!source || source == _menu->window()) {
		source = QApplication::topLevelAt(global.center());
	}
	if (!source || source == _menu->window()) {
		_blurred = QImage();
		return;
	}
	const auto ratio = style::DevicePixelRatio();
	const auto blur = style::ConvertScale(kBlurRadius);
	const auto area = global.marginsAdded({ blur, blur, blur, blur });
	auto image = QImage(
		area.size() * ratio,
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(ratio);
	image.fill(st::windowBg->c);
	{
		const auto local = QRect(
			source->mapFromGlobal(area.topLeft()),
			area.size());
		const auto visible = local.intersected(source->rect());
		if (!visible.isEmpty()) {
			Refreshing = true;
			const auto grabbed = source->grab(visible);
			Refreshing = false;
			auto p = QPainter(&image);
			p.drawPixmap(visible.topLeft() - local.topLeft(), grabbed);
		}
	}
	_blurred = Blurred(
		std::move(image),
		QRect(QPoint(blur, blur), inner.size()));
	update();
}

void Backdrop::paintEvent(QPaintEvent *e) {
	const auto inner = _menu->inner();
	if (inner.isEmpty()) {
		return;
	}
	auto p = QPainter(this);
	const auto radius = _menu->st().radius;
	PaintSoftShadow(p, inner, radius, kShadowOpacity);
	PaintGlass(p, inner, radius, _blurred, st::windowBg);
}

[[nodiscard]] bool IsRoundingOverlay(not_null<QWidget*> widget) {
	const auto parent = widget->parentWidget();
	return parent
		&& (typeid(*widget) == typeid(Ui::RpWidget))
		&& widget->testAttribute(Qt::WA_TransparentForMouseEvents)
		&& (widget->geometry() == parent->rect());
}

void Attach(not_null<Ui::PopupMenu*> menu) {
	auto &menus = Menus();
	if (menus.contains(menu.get()) || !menu->useTransparency()) {
		return;
	}
	menus.emplace(menu.get(), nullptr);
	auto &st = const_cast<style::PopupMenu&>(menu->st());
	const auto &colors = Colors();
	st.menu.itemBg = colors.clear.color();
	st.menu.itemBgOver = colors.over.color();
	st.menu.ripple.color = colors.ripple.color();
	const auto raw = menu.get();
	const auto backdrop = Ui::CreateChild<Backdrop>(raw);
	menus[raw] = backdrop;
	QObject::connect(backdrop, &QObject::destroyed, [=] {
		auto &menus = Menus();
		const auto i = menus.find(raw);
		if (i != end(menus)) {
			i->second = nullptr;
		}
	});
	QObject::connect(raw, &QObject::destroyed, [=] {
		Menus().remove(raw);
	});
}

enum class SurfaceKind {
	Selector,
	Field,
};

struct Surface {
	SurfaceKind kind = SurfaceKind::Selector;
	QImage blurred;
	QRect captured;
	bool scheduled = false;
};

[[nodiscard]] base::flat_map<QWidget*, Surface> &Surfaces() {
	static auto result = base::flat_map<QWidget*, Surface>();
	return result;
}

[[nodiscard]] QWidget *AnchorFor(
		not_null<QWidget*> widget,
		SurfaceKind kind) {
	if (kind == SurfaceKind::Field) {
		return widget;
	}
	const auto parent = widget->parentWidget();
	return dynamic_cast<ChatHelpers::TabbedPanel*>(parent)
		? parent
		: nullptr;
}

[[nodiscard]] int RadiusFor(SurfaceKind kind) {
	return (kind == SurfaceKind::Selector) ? st::emojiPanRadius : 0;
}

[[nodiscard]] QImage RenderBehind(
		not_null<QWidget*> anchor,
		not_null<QWidget*> parent,
		QRect area) {
	const auto ratio = style::DevicePixelRatio();
	auto image = QImage(
		area.size() * ratio,
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(ratio);
	image.fill(st::windowBg->c);
	auto p = QPainter(&image);
	const auto visible = area.intersected(parent->rect());
	if (visible.isEmpty()) {
		return image;
	}
	parent->render(
		&p,
		visible.topLeft() - area.topLeft(),
		QRegion(visible),
		QWidget::DrawWindowBackground);
	for (const auto child : parent->children()) {
		const auto widget = qobject_cast<QWidget*>(child);
		if (!widget) {
			continue;
		} else if (widget == anchor.get()) {
			break;
		} else if (widget->isWindow() || !widget->isVisible()) {
			continue;
		}
		const auto geometry = widget->geometry();
		const auto part = visible.intersected(geometry);
		if (part.isEmpty()) {
			continue;
		}
		widget->render(
			&p,
			part.topLeft() - area.topLeft(),
			QRegion(part.translated(-geometry.topLeft())),
			QWidget::DrawWindowBackground | QWidget::DrawChildren);
	}
	return image;
}

void RefreshSurface(not_null<QWidget*> widget) {
	if (Refreshing) {
		return;
	}
	const auto i = Surfaces().find(widget.get());
	if (i == end(Surfaces())) {
		return;
	}
	const auto anchor = AnchorFor(widget, i->second.kind);
	if (!anchor || !anchor->isVisible()) {
		return;
	}
	const auto parent = anchor->parentWidget();
	if (!parent || widget->size().isEmpty()) {
		return;
	}
	const auto area = QRect(
		widget->mapTo(parent, QPoint()),
		widget->size());
	if (area == i->second.captured && !i->second.blurred.isNull()) {
		return;
	}
	const auto blur = style::ConvertScale(kBlurRadius);
	Refreshing = true;
	auto image = RenderBehind(
		anchor,
		parent,
		area.marginsAdded({ blur, blur, blur, blur }));
	Refreshing = false;
	auto blurred = Blurred(
		std::move(image),
		QRect(QPoint(blur, blur), area.size()));
	const auto j = Surfaces().find(widget.get());
	if (j == end(Surfaces())) {
		return;
	}
	j->second.captured = area;
	j->second.blurred = std::move(blurred);
	widget->update();
}

void ScheduleRefresh(not_null<QWidget*> widget) {
	const auto i = Surfaces().find(widget.get());
	if (i == end(Surfaces()) || i->second.scheduled) {
		return;
	}
	i->second.scheduled = true;
	const auto raw = widget.get();
	QTimer::singleShot(kSurfaceRefreshDelay, raw, [=] {
		const auto j = Surfaces().find(raw);
		if (j == end(Surfaces())) {
			return;
		}
		j->second.scheduled = false;
		RefreshSurface(raw);
	});
}

void RegisterSurface(not_null<QWidget*> widget, SurfaceKind kind) {
	auto &surfaces = Surfaces();
	const auto raw = widget.get();
	if (!surfaces.contains(raw)) {
		surfaces.emplace(raw, Surface{ .kind = kind });
		QObject::connect(raw, &QObject::destroyed, [=] {
			Surfaces().remove(raw);
		});
	}
	RefreshSurface(widget);
}

void RefreshSurfacesFor(not_null<QWidget*> changed, bool now) {
	if (Refreshing) {
		return;
	}
	auto list = std::vector<QWidget*>();
	for (const auto &[widget, surface] : Surfaces()) {
		if (widget == changed.get()
			|| AnchorFor(widget, surface.kind) == changed.get()) {
			list.push_back(widget);
		}
	}
	for (const auto widget : list) {
		if (now) {
			RefreshSurface(widget);
		} else {
			ScheduleRefresh(widget);
		}
	}
}

void TrackShown(not_null<QObject*> object) {
	if (const auto selector = dynamic_cast<ChatHelpers::TabbedSelector*>(
			object.get())) {
		RegisterSurface(selector, SurfaceKind::Selector);
	} else if (const auto field = dynamic_cast<ChatHelpers::FieldAutocomplete*>(
			object.get())) {
		RegisterSurface(field, SurfaceKind::Field);
	}
}

void PaintSurface(
		not_null<QWidget*> widget,
		const Surface &surface,
		QRect clip) {
	auto p = QPainter(widget);
	p.setClipRect(clip);
	if (!AnchorFor(widget, surface.kind)) {
		p.fillRect(clip, st::emojiPanBg);
		return;
	}
	PaintGlass(
		p,
		widget->rect(),
		RadiusFor(surface.kind),
		surface.blurred,
		st::emojiPanBg);
}

class GlassFilter final : public QObject {
public:
	using QObject::QObject;

protected:
	bool eventFilter(QObject *object, QEvent *e) override {
		switch (e->type()) {
		case QEvent::ChildAdded:
			if (const auto menu = dynamic_cast<Ui::PopupMenu*>(object)) {
				Attach(menu);
			}
			break;
		case QEvent::Paint:
			if (const auto widget = qobject_cast<QWidget*>(object)) {
				const auto parent = widget->parentWidget();
				if (parent
					&& Menus().contains(parent)
					&& IsRoundingOverlay(widget)) {
					return true;
				}
				const auto &surfaces = Surfaces();
				const auto i = surfaces.find(widget);
				if (i != end(surfaces)) {
					PaintSurface(
						widget,
						i->second,
						static_cast<QPaintEvent*>(e)->rect());
					object->event(e);
					return true;
				}
			}
			break;
		case QEvent::Show:
			TrackShown(object);
			[[fallthrough]];
		case QEvent::Move:
		case QEvent::Resize:
			if (const auto widget = qobject_cast<QWidget*>(object)) {
				const auto &menus = Menus();
				const auto i = menus.find(widget);
				if (i != end(menus) && i->second) {
					i->second->refresh();
				}
				RefreshSurfacesFor(widget, e->type() == QEvent::Show);
			}
			break;
		default:
			break;
		}
		return false;
	}

};

} // namespace

void SetupGlassMenus() {
	if (!WebLayout()) {
		return;
	}
	const auto &colors = Colors();
	auto &pan = const_cast<style::EmojiPan&>(st::defaultEmojiPan);
	pan.bg = colors.clear.color();
	pan.categoriesBg = colors.clear.color();
	const auto app = QCoreApplication::instance();
	app->installEventFilter(new GlassFilter(app));
}

} // namespace AyuDesign
