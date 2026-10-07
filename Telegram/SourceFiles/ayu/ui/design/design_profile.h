#pragma once

namespace Ui {
class VerticalLayout;
} // namespace Ui

namespace Window {
class SessionController;
} // namespace Window

namespace AyuDesign {

void AddWebProfileCover(
	not_null<Ui::VerticalLayout*> container,
	not_null<Window::SessionController*> window,
	not_null<PeerData*> peer,
	rpl::producer<int> onlineCount);

} // namespace AyuDesign
