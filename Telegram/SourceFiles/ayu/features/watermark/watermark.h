// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class HistoryItem;
struct TextWithTags;

namespace Ui {
class PopupMenu;
} // namespace Ui

namespace AyuFeatures::Watermark {

void Apply(TextWithTags &text, PeerId recipient);
[[nodiscard]] std::optional<PeerId> Find(const QString &text);

void AddMenuAction(
	not_null<Ui::PopupMenu*> menu,
	not_null<HistoryItem*> item);

} // namespace AyuFeatures::Watermark
