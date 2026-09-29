// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace AyuFeatures::StripMetadata {

[[nodiscard]] bool Enabled();

[[nodiscard]] std::optional<QByteArray> Strip(const QByteArray &bytes);

[[nodiscard]] std::optional<QByteArray> StripFile(
	const QString &path,
	const QByteArray &content,
	const QString &mime,
	qint64 size);

} // namespace AyuFeatures::StripMetadata
