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

namespace AyuFeatures::DraftsHistory {

void Remember(not_null<History*> history);
void Forget(not_null<History*> history, const QString &sentText);

[[nodiscard]] bool HasEntries(not_null<History*> history);
void ShowBox(
	not_null<Window::SessionController*> controller,
	not_null<History*> history);

} // namespace AyuFeatures::DraftsHistory
