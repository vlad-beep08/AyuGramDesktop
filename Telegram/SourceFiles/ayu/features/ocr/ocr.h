// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class PhotoData;

namespace Ui {
class PopupMenu;
} // namespace Ui

namespace AyuFeatures::Ocr {

void AddMenuAction(
	not_null<Ui::PopupMenu*> menu,
	not_null<PhotoData*> photo);

} // namespace AyuFeatures::Ocr
