// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

class HistoryItem;

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace AyuFeatures::Bookmarks {

struct Entry {
	PeerId peerId = 0;
	MsgId msgId = 0;
	TimeId added = 0;
	QString preview;
};

[[nodiscard]] bool Has(not_null<HistoryItem*> item);
bool Toggle(not_null<HistoryItem*> item);

[[nodiscard]] std::vector<Entry> List(
	not_null<Main::Session*> session,
	PeerId peerId = 0);
void Clear(not_null<Main::Session*> session, PeerId peerId = 0);

void ShowBox(
	not_null<Window::SessionController*> controller,
	PeerId peerId = 0);

} // namespace AyuFeatures::Bookmarks
