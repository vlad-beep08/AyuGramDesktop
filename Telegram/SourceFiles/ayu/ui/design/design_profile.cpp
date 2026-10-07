#include "ayu/ui/design/design_profile.h"

#include "ayu/ui/design/design_cards.h"
#include "ayu/ui/design/design_system.h"
#include "data/data_changes.h"
#include "data/data_file_origin.h"
#include "data/data_peer.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "info/profile/info_profile_badge.h"
#include "info/profile/info_profile_status_label.h"
#include "info/profile/info_profile_values.h"
#include "main/main_session.h"
#include "ui/image/image.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_ayu_styles.h"
#include "styles/style_info.h"

namespace AyuDesign {
namespace {

constexpr auto kFallbackSize = 320;
constexpr auto kTextLeft = 8;
constexpr auto kTextBottom = 16;
constexpr auto kNameSkip = 2;
constexpr auto kBadgeSkip = 4;
constexpr auto kShadeFrom = 0.5;
constexpr auto kShadeAlpha = 170;

class CoverButton final : public Ui::AbstractButton {
public:
	using Ui::AbstractButton::AbstractButton;

protected:
	int resizeGetHeight(int newWidth) override {
		return newWidth + 2 * WebCardMargin();
	}

};

struct CoverState {
	PhotoData *photo = nullptr;
	std::shared_ptr<Data::PhotoMedia> media;
	Ui::PeerUserpicView view;
	QImage fallback;
	InMemoryKey fallbackKey;
	QImage scaled;
	QSize scaledFor;
	qint64 scaledFrom = 0;
	std::unique_ptr<Info::Profile::Badge> badge;
	std::unique_ptr<Info::Profile::Badge> verified;
	std::unique_ptr<Info::Profile::StatusLabel> status;
};

[[nodiscard]] QImage CurrentImage(
		not_null<CoverState*> state,
		not_null<PeerData*> peer) {
	if (const auto media = state->media.get()) {
		for (const auto size : {
				Data::PhotoSize::Large,
				Data::PhotoSize::Thumbnail,
				Data::PhotoSize::Small }) {
			if (const auto image = media->image(size)) {
				return image->original();
			}
		}
	}
	const auto key = peer->userpicUniqueKey(state->view);
	if (state->fallback.isNull() || state->fallbackKey != key) {
		state->fallbackKey = key;
		state->fallback = PeerData::GenerateUserpicImage(
			peer,
			state->view,
			style::ConvertScale(kFallbackSize) * style::DevicePixelRatio(),
			0);
	}
	return state->fallback;
}

void PaintCover(
		QPainter &p,
		QRect rect,
		not_null<CoverState*> state,
		not_null<PeerData*> peer) {
	const auto image = CurrentImage(state, peer);
	if (image.isNull()) {
		p.fillRect(rect, st::windowBgOver);
	} else {
		const auto ratio = style::DevicePixelRatio();
		const auto target = rect.size() * ratio;
		if (state->scaledFor != rect.size()
			|| state->scaledFrom != image.cacheKey()) {
			state->scaledFor = rect.size();
			state->scaledFrom = image.cacheKey();
			auto scaled = image.scaled(
				target,
				Qt::KeepAspectRatioByExpanding,
				Qt::SmoothTransformation);
			state->scaled = scaled.copy(
				(scaled.width() - target.width()) / 2,
				(scaled.height() - target.height()) / 2,
				target.width(),
				target.height());
			state->scaled.setDevicePixelRatio(ratio);
		}
		p.drawImage(rect.topLeft(), state->scaled);
	}
	auto shade = QLinearGradient(
		QPointF(rect.x(), rect.y() + rect.height() * kShadeFrom),
		QPointF(rect.x(), rect.y() + rect.height()));
	shade.setColorAt(0., QColor(0, 0, 0, 0));
	shade.setColorAt(1., QColor(0, 0, 0, kShadeAlpha));
	p.fillRect(rect, shade);
}

void RefreshPhoto(
		not_null<CoverState*> state,
		not_null<PeerData*> peer) {
	const auto id = peer->userpicPhotoId();
	if (!id) {
		state->photo = nullptr;
		state->media = nullptr;
	} else {
		const auto photo = peer->owner().photo(id);
		if (state->photo != photo || !state->media) {
			state->photo = photo;
			state->media = photo->createMediaView();
		}
		const auto origin = peer->isUser()
			? Data::FileOrigin(
				Data::FileOriginUserPhoto(peerToUser(peer->id), photo->id))
			: Data::FileOrigin(Data::FileOriginPeerPhoto(peer->id));
		state->media->wanted(Data::PhotoSize::Large, origin);
	}
	peer->loadUserpic();
}

} // namespace

void AddWebProfileCover(
		not_null<Ui::VerticalLayout*> container,
		not_null<Window::SessionController*> window,
		not_null<PeerData*> peer,
		rpl::producer<int> onlineCount) {
	const auto button = container->add(object_ptr<CoverButton>(container));
	const auto state = button->lifetime().make_state<CoverState>();
	state->view = peer->createUserpicView();

	MarkWebCardBleed(button, [=](QPainter &p, QRect rect) {
		PaintCover(p, rect, state, peer);
	});

	const auto refresh = [=] {
		RefreshPhoto(state, peer);
		RefreshWebCardBleed(button);
	};
	using Flag = Data::PeerUpdate::Flag;
	peer->session().changes().peerFlagsValue(
		peer,
		Flag::Photo | Flag::FullInfo
	) | rpl::on_next(refresh, button->lifetime());
	peer->session().downloaderTaskFinished(
	) | rpl::on_next([=] {
		const auto key = CurrentImage(state, peer).cacheKey();
		if (key != state->scaledFrom) {
			RefreshWebCardBleed(button);
		}
	}, button->lifetime());

	button->setClickedCallback([=] {
		if (const auto id = peer->userpicPhotoId()) {
			const auto photo = peer->owner().photo(id);
			if (photo->date()) {
				window->openPhoto(photo, peer);
			}
		}
	});

	const auto white = QColor(255, 255, 255);
	const auto name = Ui::CreateChild<Ui::FlatLabel>(
		button,
		Info::Profile::NameValue(peer),
		st::ayuWebProfileName);
	name->setTextColorOverride(white);
	name->setAttribute(Qt::WA_TransparentForMouseEvents);

	const auto status = Ui::CreateChild<Ui::FlatLabel>(
		button,
		QString(),
		st::ayuWebProfileStatus);
	status->setTextColorOverride(white);
	status->setAttribute(Qt::WA_TransparentForMouseEvents);
	state->status = std::make_unique<Info::Profile::StatusLabel>(
		status,
		peer);
	state->status->setColorized(false);
	std::move(
		onlineCount
	) | rpl::on_next([=](int count) {
		state->status->setOnlineCount(count);
	}, button->lifetime());
	peer->session().changes().peerFlagsValue(
		peer,
		Flag::OnlineStatus | Flag::Members | Flag::FullInfo
	) | rpl::on_next([=] {
		state->status->refresh();
	}, button->lifetime());

	const auto paused = [=] {
		return window->isGifPausedAtLeastFor(
			Window::GifPauseReason::Layer);
	};
	state->badge = std::make_unique<Info::Profile::Badge>(
		button,
		st::infoPeerBadge,
		&peer->session(),
		Info::Profile::BadgeContentForPeer(peer),
		nullptr,
		paused);
	state->verified = std::make_unique<Info::Profile::Badge>(
		button,
		st::infoPeerBadge,
		&peer->session(),
		Info::Profile::VerifiedContentForPeer(peer),
		nullptr,
		paused);

	const auto layout = [=] {
		const auto width = button->width();
		const auto left = style::ConvertScale(kTextLeft);
		const auto skip = style::ConvertScale(kBadgeSkip);
		auto badges = 0;
		for (const auto badge : { state->badge.get(), state->verified.get() }) {
			if (const auto widget = badge->widget()) {
				badges += widget->width() + skip;
			}
		}
		name->resizeToNaturalWidth(std::max(width - 2 * left - badges, 1));
		status->resizeToWidth(std::max(width - 2 * left, 1));
		const auto statusTop = button->height()
			- style::ConvertScale(kTextBottom)
			- status->height();
		status->moveToLeft(left, statusTop);
		const auto nameTop = statusTop
			- style::ConvertScale(kNameSkip)
			- name->height();
		name->moveToLeft(left, nameTop);
		auto badgeLeft = left + name->width() + skip;
		for (const auto badge : { state->badge.get(), state->verified.get() }) {
			badge->move(badgeLeft, nameTop, nameTop + name->height());
			if (const auto widget = badge->widget()) {
				badgeLeft += widget->width() + skip;
			}
		}
	};
	rpl::merge(
		button->sizeValue() | rpl::to_empty,
		name->widthValue() | rpl::to_empty,
		status->heightValue() | rpl::to_empty,
		state->badge->updated(),
		state->verified->updated()
	) | rpl::on_next(layout, button->lifetime());
}

} // namespace AyuDesign
