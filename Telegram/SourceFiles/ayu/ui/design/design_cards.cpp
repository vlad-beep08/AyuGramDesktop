// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_cards.h"

#include "ayu/ui/design/design_system.h"
#include "ui/painter.h"
#include "ui/rp_widget.h"
#include "ui/ui_utility.h"
#include "ui/widgets/box_content_divider.h"
#include "styles/palette.h"

#include <QtCore/QPointer>
#include <QtGui/QImage>
#include <QtGui/QPainterPath>

#include <typeinfo>

namespace AyuDesign {
namespace {

constexpr auto kBleedProperty = "ayuWebCardBleed";
constexpr auto kWrapProperty = "ayuWebCardsWrap";

struct Band {
	int top = 0;
	int bottom = 0;
	bool bleed = false;
	QWidget *source = nullptr;
};

struct WallpaperCanvas {
	QPointer<QWidget> owner;
	Fn<void(QPainter&, QSize, QRect)> paint;
	std::vector<QPointer<QWidget>> users;
};

[[nodiscard]] auto WallpaperCanvases()
-> base::flat_map<QWidget*, WallpaperCanvas> & {
	static auto result = base::flat_map<QWidget*, WallpaperCanvas>();
	return result;
}

void RememberWallpaperUser(
		WallpaperCanvas &canvas,
		not_null<QWidget*> widget) {
	auto &users = canvas.users;
	const auto raw = widget.get();
	const auto known = ranges::find_if(users, [&](const auto &user) {
		return user.data() == raw;
	});
	if (known != users.end()) {
		return;
	}
	users.erase(ranges::remove_if(users, [](const auto &user) {
		return !user;
	}), users.end());
	users.push_back(raw);
}

[[nodiscard]] auto BleedPainters()
-> base::flat_map<QWidget*, Fn<void(QPainter&, QRect)>> & {
	static auto result = base::flat_map<
		QWidget*,
		Fn<void(QPainter&, QRect)>>();
	return result;
}

class DividerFilter final : public QObject {
public:
	using QObject::QObject;

protected:
	bool eventFilter(QObject *object, QEvent *e) override {
		if (e->type() != QEvent::Paint) {
			return false;
		}
		const auto divider = static_cast<Ui::BoxContentDivider*>(object);
		const auto clip = static_cast<QPaintEvent*>(e)->rect();
		auto p = QPainter(divider);
		if (!PaintWallpaper(p, divider, clip)) {
			p.fillRect(clip, divider->color());
		}
		return true;
	}

};

struct CardsState {
	not_null<Ui::RpWidget*> wrap;
	not_null<Ui::RpWidget*> content;
	not_null<Ui::RpWidget*> under;
	not_null<Ui::RpWidget*> over;
	DividerFilter *filter = nullptr;
	std::vector<Band> bands;
	bool dirty = true;
};

[[nodiscard]] QRect VisibleRect(
		not_null<QWidget*> widget,
		not_null<QWidget*> root) {
	auto result = QRect(widget->mapTo(root, QPoint()), widget->size());
	for (auto parent = widget->parentWidget()
		; parent && parent != root
		; parent = parent->parentWidget()) {
		result &= QRect(parent->mapTo(root, QPoint()), parent->size());
	}
	return result;
}

void Compute(not_null<CardsState*> state) {
	state->dirty = false;
	state->bands.clear();
	const auto content = state->content;
	const auto area = content->geometry();
	if (area.isEmpty()) {
		return;
	}
	auto breaks = std::vector<Band>();
	for (const auto child : content->findChildren<QWidget*>()) {
		const auto divider = dynamic_cast<Ui::BoxContentDivider*>(child);
		const auto plain = divider
			&& (typeid(*divider) == typeid(Ui::BoxContentDivider));
		const auto gap = plain
			|| (divider && divider->height() <= 2 * st::boxDividerHeight);
		const auto bleed = child->property(kBleedProperty).toBool();
		if ((!gap && !bleed) || !child->isVisibleTo(content)) {
			continue;
		}
		if (plain) {
			divider->installEventFilter(state->filter);
		}
		const auto rect = VisibleRect(child, state->wrap) & area;
		if (rect.height() > 0) {
			breaks.push_back({
				.top = rect.y(),
				.bottom = rect.y() + rect.height(),
				.bleed = bleed,
				.source = child,
			});
		}
	}
	ranges::sort(breaks, ranges::less(), &Band::top);
	const auto minimal = WebCardRadius();
	auto cursor = area.y();
	const auto addCard = [&](int top, int bottom) {
		if (bottom - top >= minimal) {
			state->bands.push_back({ .top = top, .bottom = bottom });
		}
	};
	for (const auto &item : breaks) {
		if (item.top > cursor) {
			addCard(cursor, item.top);
		}
		if (item.bleed && item.bottom > cursor) {
			state->bands.push_back({
				.top = std::max(cursor, item.top),
				.bottom = item.bottom,
				.bleed = true,
				.source = item.source,
			});
		}
		cursor = std::max(cursor, item.bottom);
	}
	addCard(cursor, area.y() + area.height());
}

[[nodiscard]] QRect BandRect(
		not_null<CardsState*> state,
		const Band &band) {
	const auto left = band.bleed ? 0 : state->content->x();
	const auto width = band.bleed
		? state->wrap->width()
		: state->content->width();
	return QRect(left, band.top, width, band.bottom - band.top);
}

void PaintUnder(not_null<CardsState*> state, QRect clip) {
	if (state->dirty) {
		Compute(state);
	}
	auto p = QPainter(state->under);
	auto hq = PainterHighQualityEnabler(p);
	p.setPen(Qt::NoPen);
	p.setBrush(st::windowBg);
	const auto radius = WebCardRadius();
	for (const auto &band : state->bands) {
		const auto rect = BandRect(state, band);
		if (!rect.intersects(clip)) {
			continue;
		} else if (band.bleed) {
			const auto &painters = BleedPainters();
			const auto i = painters.find(band.source);
			if (i != painters.end() && i->second) {
				p.save();
				i->second(p, rect);
				p.restore();
			} else {
				p.fillRect(rect, st::windowBg);
			}
		} else {
			const auto r = std::min(radius, rect.height() / 2);
			p.drawRoundedRect(rect, r, r);
		}
	}
}

void PaintCorner(
		QPainter &p,
		not_null<CardsState*> state,
		const QPainterPath &card,
		QRect area) {
	const auto ratio = style::DevicePixelRatio();
	auto image = QImage(
		area.size() * ratio,
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(ratio);
	image.fill(Qt::transparent);
	{
		auto q = QPainter(&image);
		q.translate(-area.topLeft());
		if (!PaintWallpaper(q, state->over, area)) {
			q.fillRect(area, st::boxDividerBg);
		}
		q.setCompositionMode(QPainter::CompositionMode_DestinationOut);
		auto hq = PainterHighQualityEnabler(q);
		q.fillPath(card, Qt::black);
	}
	p.drawImage(area.topLeft(), image);
}

void PaintOver(not_null<CardsState*> state, QRect clip) {
	if (state->dirty) {
		Compute(state);
	}
	auto p = QPainter(state->over);
	const auto radius = WebCardRadius();
	for (const auto &band : state->bands) {
		if (band.bleed) {
			continue;
		}
		const auto rect = BandRect(state, band);
		const auto r = std::min(radius, rect.height() / 2);
		if (r <= 0 || !rect.intersects(clip)) {
			continue;
		}
		auto card = QPainterPath();
		card.addRoundedRect(rect, r, r);
		const auto right = rect.x() + rect.width() - r;
		const auto bottom = rect.y() + rect.height() - r;
		const auto corners = {
			QRect(rect.x(), rect.y(), r, r),
			QRect(right, rect.y(), r, r),
			QRect(rect.x(), bottom, r, r),
			QRect(right, bottom, r, r),
		};
		for (const auto &corner : corners) {
			const auto area = corner & clip;
			if (!area.isEmpty()) {
				PaintCorner(p, state, card, area);
			}
		}
	}
}

} // namespace

void SetupWebCards(
		not_null<Ui::RpWidget*> wrap,
		not_null<Ui::RpWidget*> content) {
	const auto under = Ui::CreateChild<Ui::RpWidget>(wrap.get());
	const auto over = Ui::CreateChild<Ui::RpWidget>(wrap.get());
	const auto state = under->lifetime().make_state<CardsState>(CardsState{
		.wrap = wrap,
		.content = content,
		.under = under,
		.over = over,
	});
	state->filter = new DividerFilter(under);
	wrap->setProperty(kWrapProperty, true);
	for (const auto widget : { under, over }) {
		widget->setAttribute(Qt::WA_TransparentForMouseEvents);
	}
	const auto refresh = [=] {
		state->dirty = true;
		under->update();
		over->update();
	};
	wrap->sizeValue(
	) | rpl::on_next([=](QSize size) {
		under->setGeometry(QRect(QPoint(), size));
		over->setGeometry(QRect(QPoint(), size));
		under->lower();
		over->raise();
		refresh();
	}, under->lifetime());
	content->geometryValue(
	) | rpl::on_next(refresh, under->lifetime());
	under->paintRequest(
	) | rpl::on_next([=](QRect clip) {
		PaintUnder(state, clip);
	}, under->lifetime());
	over->paintRequest(
	) | rpl::on_next([=](QRect clip) {
		PaintOver(state, clip);
	}, over->lifetime());
	under->show();
	over->show();
}

void MarkWebCardBleed(
		not_null<QWidget*> widget,
		Fn<void(QPainter&, QRect)> paint) {
	widget->setProperty(kBleedProperty, true);
	if (paint) {
		const auto raw = widget.get();
		BleedPainters()[raw] = std::move(paint);
		QObject::connect(raw, &QObject::destroyed, [=] {
			BleedPainters().remove(raw);
		});
	}
}

void RegisterWallpaperCanvas(
		not_null<QWidget*> canvas,
		not_null<QWidget*> owner,
		Fn<void(QPainter&, QSize, QRect)> paint) {
	auto &canvases = WallpaperCanvases();
	const auto raw = canvas.get();
	const auto fresh = !canvases.contains(raw);
	auto &entry = canvases[raw];
	entry.owner = owner.get();
	entry.paint = std::move(paint);
	if (fresh) {
		QObject::connect(raw, &QObject::destroyed, [=] {
			WallpaperCanvases().remove(raw);
		});
	}
}

bool PaintWallpaper(QPainter &p, not_null<QWidget*> widget, QRect clip) {
	auto &canvases = WallpaperCanvases();
	for (auto parent = widget.get(); parent; parent = parent->parentWidget()) {
		const auto i = canvases.find(parent);
		if (i == canvases.end()) {
			continue;
		}
		auto &canvas = i->second;
		if (!canvas.owner || !canvas.paint) {
			return false;
		}
		RememberWallpaperUser(canvas, widget);
		const auto shift = widget->mapTo(parent, QPoint());
		p.save();
		p.translate(-shift);
		canvas.paint(p, parent->size(), clip.translated(shift));
		p.restore();
		return true;
	}
	return false;
}

void RefreshWallpaper(not_null<QWidget*> canvas) {
	auto &canvases = WallpaperCanvases();
	const auto i = canvases.find(canvas.get());
	if (i == canvases.end()) {
		return;
	}
	for (const auto &user : i->second.users) {
		if (user) {
			user->update();
		}
	}
}

void RefreshWebCardBleed(not_null<QWidget*> widget) {
	for (auto parent = widget->parentWidget()
		; parent
		; parent = parent->parentWidget()) {
		if (parent->property(kWrapProperty).toBool()) {
			const auto top = widget->mapTo(parent, QPoint()).y();
			parent->update(0, top, parent->width(), widget->height());
			return;
		}
	}
	widget->update();
}

} // namespace AyuDesign
