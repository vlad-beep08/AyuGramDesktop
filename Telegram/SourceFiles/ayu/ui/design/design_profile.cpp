#include "ayu/ui/design/design_profile.h"

#include "api/api_peer_colors.h"
#include "apiwrap.h"
#include "ayu/ui/design/design_cards.h"
#include "ayu/ui/design/design_system.h"
#include "data/data_changes.h"
#include "data/data_document.h"
#include "data/data_emoji_statuses.h"
#include "data/data_file_origin.h"
#include "data/data_peer.h"
#include "data/data_peer_colors.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "data/data_session.h"
#include "data/stickers/data_custom_emoji.h"
#include "info/profile/info_profile_badge.h"
#include "info/profile/info_profile_status_label.h"
#include "info/profile/info_profile_values.h"
#include "main/main_session.h"
#include "ui/image/image.h"
#include "ui/image/image_prepare.h"
#include "ui/painter.h"
#include "ui/text/text_custom_emoji.h"
#include "ui/top_background_gradient.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_ayu_styles.h"
#include "styles/style_basic.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_info.h"

#include <QtGui/QCursor>

#include <array>

namespace AyuDesign {
namespace {

constexpr auto kPi = 3.14159265358979323846;
constexpr auto kFallbackSize = 320;
constexpr auto kTextLeft = 8;
constexpr auto kTextBottom = 16;
constexpr auto kNameSkip = 2;
constexpr auto kBadgeSkip = 4;
constexpr auto kShadeFrom = 0.5;
constexpr auto kShadeAlpha = 170;
constexpr auto kColoredHeight = 236;
constexpr auto kAvatarSize = 110;
constexpr auto kAvatarTop = 22;
constexpr auto kAvatarNameSkip = 12;
constexpr auto kPatternRings = 3;
constexpr auto kPatternFirstGap = 26;
constexpr auto kPatternRingGap = 34;
constexpr auto kPatternFirstCount = 6;
constexpr auto kPatternCountStep = 3;
constexpr auto kPatternTileAlpha = 0.8;
constexpr auto kPatternVertical = 0.82;
constexpr auto kPatternOpacity = std::array<float64, 3>{ 0.55, 0.38, 0.22 };
constexpr auto kPatternScale = std::array<float64, 3>{ 1., 0.86, 0.72 };

class CoverButton final : public Ui::AbstractButton {
public:
	using Ui::AbstractButton::AbstractButton;

	void setHeightCallback(Fn<int(int)> callback) {
		_height = std::move(callback);
	}

protected:
	int resizeGetHeight(int newWidth) override {
		return _height ? _height(newWidth) : newWidth;
	}

private:
	Fn<int(int)> _height;

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
	QImage avatar;
	int avatarFor = 0;
	qint64 avatarFrom = 0;
	QImage gradient;
	QSize gradientFor;
	DocumentId patternId = 0;
	std::unique_ptr<Ui::Text::CustomEmoji> pattern;
	QImage patternTile;
	bool expanded = false;
	std::unique_ptr<Info::Profile::Badge> badge;
	std::unique_ptr<Info::Profile::Badge> verified;
	std::unique_ptr<Info::Profile::StatusLabel> status;
};

[[nodiscard]] std::optional<Data::ColorProfileSet> ColorProfile(
		not_null<PeerData*> peer) {
	return peer->session().api().peerColors().colorProfileFor(peer);
}

[[nodiscard]] bool HasProfileColor(not_null<PeerData*> peer) {
	if (peer->emojiStatusId().collectible) {
		return true;
	}
	const auto profile = ColorProfile(peer);
	return profile && !profile->bg.empty();
}

[[nodiscard]] bool ColoredMode(
		not_null<CoverState*> state,
		not_null<PeerData*> peer) {
	return !state->expanded && HasProfileColor(peer);
}

[[nodiscard]] int CoverHeight(
		not_null<CoverState*> state,
		not_null<PeerData*> peer,
		int width) {
	return ColoredMode(state, peer)
		? style::ConvertScale(kColoredHeight)
		: (width + 2 * WebCardMargin());
}

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

[[nodiscard]] QImage CropScaled(const QImage &image, QSize target) {
	auto scaled = image.scaled(
		target,
		Qt::KeepAspectRatioByExpanding,
		Qt::SmoothTransformation);
	return scaled.copy(
		(scaled.width() - target.width()) / 2,
		(scaled.height() - target.height()) / 2,
		target.width(),
		target.height());
}

[[nodiscard]] QRect AvatarRect(QRect rect) {
	const auto size = style::ConvertScale(kAvatarSize);
	return QRect(
		rect.x() + (rect.width() - size) / 2,
		rect.y() + style::ConvertScale(kAvatarTop),
		size,
		size);
}

void RefreshPattern(
		not_null<CoverState*> state,
		not_null<PeerData*> peer,
		Fn<void()> repaint) {
	const auto collectible = peer->emojiStatusId().collectible;
	const auto id = (collectible && collectible->patternDocumentId)
		? collectible->patternDocumentId
		: peer->profileBackgroundEmojiId();
	if (state->patternId == id) {
		return;
	}
	state->patternId = id;
	state->patternTile = QImage();
	state->pattern = id
		? peer->owner().customEmojiManager().create(
			peer->owner().document(id),
			std::move(repaint),
			Data::CustomEmojiSizeTag::Normal)
		: nullptr;
}

void PaintPattern(
		QPainter &p,
		QRect avatar,
		not_null<CoverState*> state,
		not_null<PeerData*> peer) {
	const auto pattern = state->pattern.get();
	if (!pattern || !pattern->ready()) {
		return;
	}
	const auto ratio = style::DevicePixelRatio();
	const auto size = st::emojiSize;
	if (state->patternTile.isNull()) {
		auto tile = QImage(
			QSize(size, size) * ratio,
			QImage::Format_ARGB32_Premultiplied);
		tile.setDevicePixelRatio(ratio);
		tile.fill(Qt::transparent);
		auto q = QPainter(&tile);
		auto hq = PainterHighQualityEnabler(q);
		pattern->paint(q, { .textColor = Qt::white });
		q.setCompositionMode(QPainter::CompositionMode_SourceIn);
		const auto collectible = peer->emojiStatusId().collectible;
		const auto color = (collectible && collectible->patternColor.isValid())
			? collectible->patternColor
			: QColor(0, 0, 0, int(kPatternTileAlpha * 255));
		q.fillRect(QRect(0, 0, size, size), color);
		q.end();
		state->patternTile = std::move(tile);
	}
	const auto center = QPointF(avatar.center()) + QPointF(0.5, 0.5);
	const auto base = avatar.width() / 2.;
	auto hq = PainterHighQualityEnabler(p);
	const auto opacity = p.opacity();
	for (auto ring = 0; ring != kPatternRings; ++ring) {
		const auto radius = base
			+ style::ConvertScale(kPatternFirstGap)
			+ ring * style::ConvertScale(kPatternRingGap);
		const auto count = kPatternFirstCount + ring * kPatternCountStep;
		const auto shift = (ring % 2) ? (kPi / count) : 0.;
		const auto scale = kPatternScale[ring];
		const auto side = size * scale;
		p.setOpacity(opacity * kPatternOpacity[ring]);
		for (auto i = 0; i != count; ++i) {
			const auto angle = shift + 2. * kPi * i / count;
			const auto point = center + QPointF(
				radius * std::cos(angle),
				radius * std::sin(angle) * kPatternVertical);
			p.drawImage(
				QRectF(
					point.x() - side / 2.,
					point.y() - side / 2.,
					side,
					side),
				state->patternTile);
		}
	}
	p.setOpacity(opacity);
}

void PaintColored(
		QPainter &p,
		QRect rect,
		not_null<CoverState*> state,
		not_null<PeerData*> peer) {
	const auto avatar = AvatarRect(rect);
	if (state->gradient.isNull() || state->gradientFor != rect.size()) {
		state->gradientFor = rect.size();
		state->gradient = Ui::CreateTopBgGradient(
			rect.size(),
			peer,
			QPoint(0, avatar.center().y() - rect.center().y()));
	}
	if (!state->gradient.isNull()) {
		p.drawImage(rect.topLeft(), state->gradient);
	} else if (const auto profile = ColorProfile(peer)
			; profile && !profile->bg.empty()) {
		p.fillRect(rect, profile->bg.front());
	} else {
		p.fillRect(rect, st::windowBgOver);
	}
	PaintPattern(p, avatar, state, peer);

	const auto image = CurrentImage(state, peer);
	if (image.isNull()) {
		return;
	}
	const auto ratio = style::DevicePixelRatio();
	if (state->avatarFor != avatar.width()
		|| state->avatarFrom != image.cacheKey()) {
		state->avatarFor = avatar.width();
		state->avatarFrom = image.cacheKey();
		state->avatar = Images::Circle(
			CropScaled(image, avatar.size() * ratio));
		state->avatar.setDevicePixelRatio(ratio);
	}
	p.drawImage(avatar.topLeft(), state->avatar);
}

void PaintPhoto(
		QPainter &p,
		QRect rect,
		not_null<CoverState*> state,
		not_null<PeerData*> peer) {
	const auto image = CurrentImage(state, peer);
	if (image.isNull()) {
		p.fillRect(rect, st::windowBgOver);
	} else {
		const auto ratio = style::DevicePixelRatio();
		if (state->scaledFor != rect.size()
			|| state->scaledFrom != image.cacheKey()) {
			state->scaledFor = rect.size();
			state->scaledFrom = image.cacheKey();
			state->scaled = CropScaled(image, rect.size() * ratio);
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
	button->setHeightCallback([=](int width) {
		return CoverHeight(state, peer, width);
	});

	MarkWebCardBleed(button, [=](QPainter &p, QRect rect) {
		if (ColoredMode(state, peer)) {
			PaintColored(p, rect, state, peer);
		} else {
			PaintPhoto(p, rect, state, peer);
		}
	});

	const auto repaint = [=] {
		RefreshWebCardBleed(button);
	};
	using Flag = Data::PeerUpdate::Flag;
	peer->session().changes().peerFlagsValue(
		peer,
		Flag::Photo | Flag::FullInfo
	) | rpl::on_next([=] {
		RefreshPhoto(state, peer);
		repaint();
	}, button->lifetime());
	peer->session().changes().peerFlagsValue(
		peer,
		Flag::ColorProfile | Flag::EmojiStatus | Flag::BackgroundEmoji
	) | rpl::on_next([=] {
		state->gradient = QImage();
		RefreshPattern(state, peer, repaint);
		button->resizeToWidth(button->width());
		repaint();
	}, button->lifetime());
	peer->session().downloaderTaskFinished(
	) | rpl::on_next([=] {
		const auto key = CurrentImage(state, peer).cacheKey();
		if (key != state->scaledFrom && key != state->avatarFrom) {
			repaint();
		}
	}, button->lifetime());

	button->setClickedCallback([=] {
		if (ColoredMode(state, peer)) {
			const auto margin = WebCardMargin();
			const auto band = QRect(
				-margin,
				0,
				button->width() + 2 * margin,
				button->height());
			const auto position = button->mapFromGlobal(QCursor::pos());
			if (AvatarRect(band).contains(position)) {
				state->expanded = true;
				button->resizeToWidth(button->width());
				repaint();
			}
			return;
		}
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
		status->resizeToNaturalWidth(std::max(width - 2 * left, 1));
		auto nameLeft = left;
		auto nameTop = 0;
		if (ColoredMode(state, peer)) {
			const auto band = QRect(0, 0, width, button->height());
			nameTop = AvatarRect(band).y()
				+ AvatarRect(band).height()
				+ style::ConvertScale(kAvatarNameSkip);
			nameLeft = (width - name->width() - badges) / 2;
			status->moveToLeft(
				(width - status->width()) / 2,
				nameTop + name->height() + style::ConvertScale(kNameSkip));
		} else {
			const auto statusTop = button->height()
				- style::ConvertScale(kTextBottom)
				- status->height();
			status->moveToLeft(left, statusTop);
			nameTop = statusTop
				- style::ConvertScale(kNameSkip)
				- name->height();
		}
		name->moveToLeft(nameLeft, nameTop);
		auto badgeLeft = nameLeft + name->width() + skip;
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
		status->sizeValue() | rpl::to_empty,
		state->badge->updated(),
		state->verified->updated()
	) | rpl::on_next(layout, button->lifetime());
}

} // namespace AyuDesign
