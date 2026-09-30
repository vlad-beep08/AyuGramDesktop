// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class History;

namespace Window {
class SessionController;
} // namespace Window

namespace AyuFeatures::HiddenChats {

[[nodiscard]] bool IsHidden(not_null<const History*> history);
[[nodiscard]] bool IsConcealed(not_null<const History*> history);
[[nodiscard]] bool IsLocked();

void Toggle(
	not_null<Window::SessionController*> controller,
	not_null<History*> history);
void RequestUnlock(not_null<Window::SessionController*> controller);
void ShowPinBox(not_null<Window::SessionController*> controller);
void Lock();
void Panic();

} // namespace AyuFeatures::HiddenChats
