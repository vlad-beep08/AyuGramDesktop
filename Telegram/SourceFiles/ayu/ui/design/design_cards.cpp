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

#include <QtGui/QPainterPath>

#include <typeinfo>

namespace AyuDesign {
namespace {

constexpr auto kBleedProperty = "ayuWebCardBleed";

struct Band {
	int top = 0;
	int bottom = 0;
	bool bleed = false;
};

class DividerFilter final : public QObject {
public:
	using QObject::QObject;

protected:
	bool eventFilter(QObject *object, QEvent *e) override {
		if (e->type() != QEvent::Paint) {
			return false;
		}
		const auto divider = static_cast<Ui::BoxContentDivider*>(object);
		auto p = QPainter(divider);
		p.fillRect(
			static_cast<QPaintEvent*>(e)->rect(),
			divider->color());
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
			p.fillRect(rect, st::windowBg);
		} else {
			const auto r = std::min(radius, rect.height() / 2);
			p.drawRoundedRect(rect, r, r);
		}
	}
}

void PaintOver(not_null<CardsState*> state, QRect clip) {
	if (state->dirty) {
		Compute(state);
	}
	auto path = QPainterPath();
	path.setFillRule(Qt::OddEvenFill);
	const auto radius = WebCardRadius();
	for (const auto &band : state->bands) {
		if (band.bleed) {
			continue;
		}
		const auto rect = BandRect(state, band);
		if (!rect.intersects(clip)) {
			continue;
		}
		const auto r = std::min(radius, rect.height() / 2);
		path.addRect(rect);
		path.addRoundedRect(rect, r, r);
	}
	if (path.isEmpty()) {
		return;
	}
	auto p = QPainter(state->over);
	auto hq = PainterHighQualityEnabler(p);
	p.fillPath(path, st::boxDividerBg);
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

void MarkWebCardBleed(not_null<QWidget*> widget) {
	widget->setProperty(kBleedProperty, true);
}

} // namespace AyuDesign
