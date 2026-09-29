// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/strip_metadata/strip_metadata.h"

#include "ayu/ayu_settings.h"

#include <QtCore/QFile>

namespace AyuFeatures::StripMetadata {
namespace {

constexpr auto kMaxFileSize = qint64(64 * 1024 * 1024);

constexpr auto kJpegMarkerPrefix = uchar(0xFF);
constexpr auto kJpegStartOfImage = uchar(0xD8);
constexpr auto kJpegEndOfImage = uchar(0xD9);
constexpr auto kJpegStartOfScan = uchar(0xDA);
constexpr auto kJpegRestartFirst = uchar(0xD0);
constexpr auto kJpegRestartLast = uchar(0xD7);
constexpr auto kJpegTemporary = uchar(0x01);
constexpr auto kJpegApp0 = uchar(0xE0);
constexpr auto kJpegApp1 = uchar(0xE1);
constexpr auto kJpegApp2 = uchar(0xE2);
constexpr auto kJpegApp14 = uchar(0xEE);
constexpr auto kJpegAppLast = uchar(0xEF);
constexpr auto kJpegComment = uchar(0xFE);

constexpr auto kExifHeaderSize = 6;
constexpr auto kTiffHeaderSize = 8;
constexpr auto kIfdEntrySize = 12;
constexpr auto kExifOrientationTag = 0x0112;
constexpr auto kExifTypeShort = 3;
constexpr auto kOrientationMin = 1;
constexpr auto kOrientationMax = 8;

constexpr auto kPngSignatureSize = 8;
constexpr auto kPngChunkOverhead = 12;

[[nodiscard]] int Byte(const QByteArray &bytes, qsizetype index) {
	return int(uchar(bytes[index]));
}

[[nodiscard]] int ReadBig16(const QByteArray &bytes, qsizetype index) {
	return (Byte(bytes, index) << 8) | Byte(bytes, index + 1);
}

[[nodiscard]] qint64 ReadBig32(const QByteArray &bytes, qsizetype index) {
	return (qint64(Byte(bytes, index)) << 24)
		| (qint64(Byte(bytes, index + 1)) << 16)
		| (qint64(Byte(bytes, index + 2)) << 8)
		| qint64(Byte(bytes, index + 3));
}

void AppendBig16(QByteArray &to, int value) {
	to.append(char((value >> 8) & 0xFF));
	to.append(char(value & 0xFF));
}

void AppendBig32(QByteArray &to, qint64 value) {
	AppendBig16(to, int((value >> 16) & 0xFFFF));
	AppendBig16(to, int(value & 0xFFFF));
}

[[nodiscard]] QByteArray ExifHeader() {
	return QByteArray("Exif", 4) + QByteArray(2, char(0));
}

[[nodiscard]] int ExifOrientation(const QByteArray &payload) {
	if (!payload.startsWith(ExifHeader())) {
		return 0;
	}
	const auto tiff = payload.mid(kExifHeaderSize);
	if (tiff.size() < kTiffHeaderSize) {
		return 0;
	}
	const auto little = tiff.startsWith("II");
	if (!little && !tiff.startsWith("MM")) {
		return 0;
	}
	const auto read16 = [&](qint64 index) {
		if (index < 0 || index + 2 > tiff.size()) {
			return -1;
		}
		const auto first = Byte(tiff, index);
		const auto second = Byte(tiff, index + 1);
		return little ? (first | (second << 8)) : ((first << 8) | second);
	};
	const auto read32 = [&](qint64 index) -> qint64 {
		const auto first = read16(index);
		const auto second = read16(index + 2);
		if (first < 0 || second < 0) {
			return -1;
		}
		return little
			? (qint64(first) | (qint64(second) << 16))
			: ((qint64(first) << 16) | qint64(second));
	};
	const auto ifd = read32(4);
	if (ifd < kTiffHeaderSize) {
		return 0;
	}
	const auto count = read16(ifd);
	for (auto i = 0; i < count; ++i) {
		const auto entry = ifd + 2 + qint64(i) * kIfdEntrySize;
		if (entry + kIfdEntrySize > tiff.size()) {
			return 0;
		}
		if (read16(entry) == kExifOrientationTag) {
			const auto value = read16(entry + 8);
			return (value >= kOrientationMin && value <= kOrientationMax)
				? value
				: 0;
		}
	}
	return 0;
}

[[nodiscard]] QByteArray OrientationSegment(int orientation) {
	auto payload = ExifHeader();
	payload.append("MM", 2);
	AppendBig16(payload, 42);
	AppendBig32(payload, kTiffHeaderSize);
	AppendBig16(payload, 1);
	AppendBig16(payload, kExifOrientationTag);
	AppendBig16(payload, kExifTypeShort);
	AppendBig32(payload, 1);
	AppendBig16(payload, orientation);
	AppendBig16(payload, 0);
	AppendBig32(payload, 0);

	auto result = QByteArray();
	result.append(char(kJpegMarkerPrefix));
	result.append(char(kJpegApp1));
	AppendBig16(result, int(payload.size()) + 2);
	result.append(payload);
	return result;
}

[[nodiscard]] bool IsDroppedJpegMarker(uchar marker) {
	return (marker == kJpegApp1)
		|| (marker == kJpegComment)
		|| (marker > kJpegApp2
			&& marker <= kJpegAppLast
			&& marker != kJpegApp14);
}

[[nodiscard]] std::optional<QByteArray> StripJpeg(const QByteArray &bytes) {
	const auto size = bytes.size();
	if (size < 4
		|| Byte(bytes, 0) != kJpegMarkerPrefix
		|| Byte(bytes, 1) != kJpegStartOfImage) {
		return std::nullopt;
	}
	auto result = QByteArray();
	result.reserve(size);
	result.append(bytes.constData(), 2);

	auto orientation = 0;
	auto removed = false;
	auto onlyApp0Kept = true;
	auto insertAt = result.size();
	auto position = qsizetype(2);
	while (position < size) {
		if (Byte(bytes, position) != kJpegMarkerPrefix) {
			return std::nullopt;
		}
		const auto markerStart = position;
		while (position < size && Byte(bytes, position) == kJpegMarkerPrefix) {
			++position;
		}
		if (position >= size) {
			return std::nullopt;
		}
		const auto marker = uchar(Byte(bytes, position++));
		if (marker == kJpegEndOfImage || marker == kJpegStartOfScan) {
			result.append(bytes.mid(markerStart));
			break;
		} else if ((marker >= kJpegRestartFirst && marker <= kJpegRestartLast)
			|| marker == kJpegTemporary) {
			result.append(bytes.mid(markerStart, position - markerStart));
			continue;
		}
		if (position + 2 > size) {
			return std::nullopt;
		}
		const auto length = ReadBig16(bytes, position);
		if (length < 2 || position + length > size) {
			return std::nullopt;
		}
		const auto end = position + length;
		if (marker == kJpegApp1 && !orientation) {
			orientation = ExifOrientation(bytes.mid(position + 2, length - 2));
		}
		if (IsDroppedJpegMarker(marker)) {
			removed = true;
		} else {
			result.append(bytes.mid(markerStart, end - markerStart));
			if (marker != kJpegApp0) {
				onlyApp0Kept = false;
			} else if (onlyApp0Kept) {
				insertAt = result.size();
			}
		}
		position = end;
	}
	if (!removed) {
		return std::nullopt;
	}
	if (orientation > kOrientationMin) {
		result.insert(insertAt, OrientationSegment(orientation));
	}
	return result;
}

[[nodiscard]] QByteArray PngSignature() {
	return QByteArray("\x89PNG\r\n\x1A\n", kPngSignatureSize);
}

[[nodiscard]] bool IsDroppedPngChunk(const QByteArray &type) {
	return (type == "tEXt")
		|| (type == "zTXt")
		|| (type == "iTXt")
		|| (type == "eXIf")
		|| (type == "tIME");
}

[[nodiscard]] std::optional<QByteArray> StripPng(const QByteArray &bytes) {
	const auto size = bytes.size();
	if (!bytes.startsWith(PngSignature())) {
		return std::nullopt;
	}
	auto result = PngSignature();
	result.reserve(size);
	auto removed = false;
	auto position = qsizetype(kPngSignatureSize);
	while (position < size) {
		if (position + kPngChunkOverhead > size) {
			return std::nullopt;
		}
		const auto length = ReadBig32(bytes, position);
		const auto chunkSize = length + kPngChunkOverhead;
		if (position + chunkSize > size) {
			return std::nullopt;
		}
		const auto type = bytes.mid(position + 4, 4);
		if (IsDroppedPngChunk(type)) {
			removed = true;
		} else {
			result.append(bytes.mid(position, chunkSize));
		}
		position += chunkSize;
		if (type == "IEND") {
			break;
		}
	}
	if (!removed) {
		return std::nullopt;
	}
	return result;
}

} // namespace

bool Enabled() {
	return AyuSettings::getInstance().stripImageMetadata();
}

std::optional<QByteArray> Strip(const QByteArray &bytes) {
	if (auto jpeg = StripJpeg(bytes)) {
		return jpeg;
	}
	return StripPng(bytes);
}

std::optional<QByteArray> StripFile(
		const QString &path,
		const QByteArray &content,
		const QString &mime,
		qint64 size) {
	if (mime != u"image/jpeg"_q && mime != u"image/png"_q) {
		return std::nullopt;
	} else if (!content.isEmpty()) {
		return Strip(content);
	} else if (path.isEmpty() || size <= 0 || size > kMaxFileSize) {
		return std::nullopt;
	}
	auto file = QFile(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return std::nullopt;
	}
	return Strip(file.readAll());
}

} // namespace AyuFeatures::StripMetadata
