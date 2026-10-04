// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace AyuDesign {

[[nodiscard]] bool HasMissingWebSections(not_null<Main::Session*> session);
void AddWebSections(not_null<Window::SessionController*> controller);

} // namespace AyuDesign
