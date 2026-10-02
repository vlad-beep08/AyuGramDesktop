// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace Ui {
class RpWidget;
} // namespace Ui

namespace AyuDesign {

void SetupWebCards(
	not_null<Ui::RpWidget*> wrap,
	not_null<Ui::RpWidget*> content);
void MarkWebCardBleed(
	not_null<QWidget*> widget,
	Fn<void(QPainter&, QRect)> paint = nullptr);
void RefreshWebCardBleed(not_null<QWidget*> widget);

} // namespace AyuDesign
