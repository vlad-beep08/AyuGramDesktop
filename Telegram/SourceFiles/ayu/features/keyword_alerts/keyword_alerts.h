// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class HistoryItem;

namespace Window {
class SessionController;
} // namespace Window

namespace AyuFeatures::KeywordAlerts {

[[nodiscard]] bool Matches(not_null<HistoryItem*> item);
void ShowEditBox(not_null<Window::SessionController*> controller);

} // namespace AyuFeatures::KeywordAlerts
