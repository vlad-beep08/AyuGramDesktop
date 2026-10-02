// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_glass.h"

#include "ayu/ui/design/design_islands.h"
#include "ayu/ui/design/design_system.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"
#include "ui/widgets/popup_menu.h"
#include "styles/palette.h"
#include "styles/style_widgets.h"

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
	image = Images::BlurLargeImage(std::move(image), blur * ratio);
	_blurred = image.copy(QRect(
		QPoint(blur, blur) * ratio,
		inner.size() * ratio));
	_blurred.setDevicePixelRatio(ratio);
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

	auto hq = PainterHighQualityEnabler(p);
	auto path = QPainterPath();
	path.addRoundedRect(QRectF(inner), radius, radius);
	p.save();
	p.setClipPath(path);
	if (!_blurred.isNull()) {
		p.drawImage(inner.topLeft(), _blurred);
	}
	p.fillRect(
		inner,
		WithAlpha(st::windowBg->c, _blurred.isNull() ? 1. : kTintAlpha));
	auto sheen = QLinearGradient(
		inner.topLeft(),
		QPoint(inner.x(), inner.y() + inner.height() / 2));
	sheen.setColorAt(0., WithAlpha(QColor(255, 255, 255), kSheenAlpha));
	sheen.setColorAt(1., QColor(255, 255, 255, 0));
	p.fillRect(inner, sheen);
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
		QRectF(inner).adjusted(half, half, -half, -half),
		radius - half,
		radius - half);
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
			}
			break;
		case QEvent::Move:
		case QEvent::Show:
			if (const auto widget = qobject_cast<QWidget*>(object)) {
				const auto &menus = Menus();
				const auto i = menus.find(widget);
				if (i != end(menus) && i->second) {
					i->second->refresh();
				}
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
	const auto app = QCoreApplication::instance();
	app->installEventFilter(new GlassFilter(app));
}

} // namespace AyuDesign
