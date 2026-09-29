// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/watermark/watermark.h"

#include "lang_auto.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "ui/text/text_entity.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"

namespace AyuFeatures::Watermark {
namespace {

constexpr auto kZeroBit = char16_t(0x200C);
constexpr auto kOneBit = char16_t(0x200D);
constexpr auto kMagic = uchar(0xA7);
constexpr auto kChecksumSalt = uchar(0x5A);
constexpr auto kIdBytes = 8;
constexpr auto kPayloadBytes = kIdBytes + 2;
constexpr auto kBitsPerByte = 8;
constexpr auto kMarkLength = kPayloadBytes * kBitsPerByte;

[[nodiscard]] bool IsMarkChar(QChar ch) {
	return (ch.unicode() == kZeroBit) || (ch.unicode() == kOneBit);
}

[[nodiscard]] std::array<uchar, kPayloadBytes> Payload(PeerId peer) {
	auto result = std::array<uchar, kPayloadBytes>();
	result[0] = kMagic;
	auto checksum = kChecksumSalt;
	for (auto i = 0; i != kIdBytes; ++i) {
		const auto shift = (kIdBytes - 1 - i) * kBitsPerByte;
		const auto byte = uchar((peer.value >> shift) & 0xFF);
		result[i + 1] = byte;
		checksum ^= byte;
	}
	result[kPayloadBytes - 1] = checksum;
	return result;
}

[[nodiscard]] QString Encode(const std::array<uchar, kPayloadBytes> &bytes) {
	auto result = QString();
	result.reserve(kMarkLength);
	for (const auto byte : bytes) {
		for (auto bit = kBitsPerByte - 1; bit >= 0; --bit) {
			result.append(QChar(((byte >> bit) & 1) ? kOneBit : kZeroBit));
		}
	}
	return result;
}

[[nodiscard]] std::optional<PeerId> Decode(QStringView bits) {
	auto bytes = std::array<uchar, kPayloadBytes>();
	for (auto i = 0; i != kMarkLength; ++i) {
		auto &byte = bytes[i / kBitsPerByte];
		byte = uchar((byte << 1) | ((bits[i].unicode() == kOneBit) ? 1 : 0));
	}
	if (bytes[0] != kMagic) {
		return std::nullopt;
	}
	auto checksum = kChecksumSalt;
	auto value = uint64();
	for (auto i = 0; i != kIdBytes; ++i) {
		checksum ^= bytes[i + 1];
		value = (value << kBitsPerByte) | bytes[i + 1];
	}
	if (checksum != bytes[kPayloadBytes - 1] || !value) {
		return std::nullopt;
	}
	return PeerId(PeerIdHelper(value));
}

[[nodiscard]] int InsertPosition(const QString &text) {
	const auto space = text.indexOf(QChar(' '));
	return (space >= 0) ? int(space + 1) : int(text.size());
}

} // namespace

void Apply(TextWithTags &text, PeerId recipient) {
	if (text.text.trimmed().isEmpty() || Find(text.text)) {
		return;
	}
	const auto mark = Encode(Payload(recipient));
	const auto position = InsertPosition(text.text);
	const auto length = int(mark.size());
	text.text.insert(position, mark);
	for (auto &tag : text.tags) {
		if (tag.offset >= position) {
			tag.offset += length;
		} else if (tag.offset + tag.length > position) {
			tag.length += length;
		}
	}
}

std::optional<PeerId> Find(const QString &text) {
	const auto size = int(text.size());
	for (auto i = 0; i < size;) {
		if (!IsMarkChar(text[i])) {
			++i;
			continue;
		}
		auto end = i;
		while (end < size && IsMarkChar(text[end])) {
			++end;
		}
		for (auto start = i; start + kMarkLength <= end; ++start) {
			const auto bits = QStringView(text).mid(start, kMarkLength);
			if (const auto result = Decode(bits)) {
				return result;
			}
		}
		i = end;
	}
	return std::nullopt;
}

void AddMenuAction(
		not_null<Ui::PopupMenu*> menu,
		not_null<HistoryItem*> item) {
	const auto found = Find(item->originalText().text);
	if (!found) {
		return;
	}
	const auto session = &item->history()->session();
	const auto peerId = *found;
	const auto peer = session->data().peerLoaded(peerId);
	const auto name = peer
		? peer->name()
		: tr::ayu_WatermarkUnknownRecipient(tr::now);
	menu->addAction(
		tr::ayu_WatermarkSentTo(tr::now, lt_name, name),
		[=] {
			const auto window = session->tryResolveWindow();
			if (!window) {
				return;
			} else if (session->data().peerLoaded(peerId)) {
				window->showPeerHistory(peerId);
			} else {
				window->showToast(tr::ayu_WatermarkUnknownRecipient(tr::now));
			}
		},
		&st::menuIconLock);
}

} // namespace AyuFeatures::Watermark
