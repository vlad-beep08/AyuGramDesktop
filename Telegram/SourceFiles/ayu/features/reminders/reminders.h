// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class HistoryItem;

namespace Ui {
class PopupMenu;
} // namespace Ui

namespace AyuFeatures::Reminders {

void Start();

void AddMenuAction(
	not_null<Ui::PopupMenu*> menu,
	not_null<HistoryItem*> item);

} // namespace AyuFeatures::Reminders
