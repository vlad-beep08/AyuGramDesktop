// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/design/design_left_box.h"

#include "chat_helpers/compose/compose_show.h"
#include "info/info_top_bar.h"
#include "info/info_wrap_widget.h"
#include "storage/storage_shared_media.h"
#include "ui/widgets/buttons.h"
#include "window/window_session_controller.h"
#include "styles/style_info.h"
#include "styles/style_layers.h"

namespace AyuDesign {

LeftBoxHost::LeftBoxHost(
	QWidget *parent,
	not_null<Window::SessionController*> controller,
	object_ptr<Ui::BoxContent> content,
	Fn<void()> close)
: RpWidget(parent)
, _controller(controller)
, _close(std::move(close))
, _topBar(
	this,
	controller,
	st::infoTopBar,
	Info::SelectedItems(Storage::SharedMediaType::kCount))
, _content(std::move(content)) {
	setAttribute(Qt::WA_OpaquePaintEvent);
	_topBar->enableBackButton();
	_topBar->backRequest(
	) | rpl::on_next([=] {
		closeBox();
	}, lifetime());
	_content->setParent(this);
	_content->show();
	_content->setDelegate(this);
	updateControlsGeometry();
	crl::on_main(this, [=] {
		_content->showFinished();
	});
}

LeftBoxHost::~LeftBoxHost() = default;

void LeftBoxHost::setInnerFocus() {
	_content->setInnerFocus();
}

void LeftBoxHost::setLayerType(bool layerType) {
}

void LeftBoxHost::setStyle(const style::Box &st) {
	_st = &st;
	updateControlsGeometry();
	update();
}

const style::Box &LeftBoxHost::style() {
	return _st ? *_st : st::defaultBox;
}

void LeftBoxHost::setTitle(
		rpl::producer<TextWithEntities> title,
		Ui::Text::MarkedContext context) {
	if (!title) {
		return;
	}
	_topBar->setTitle({
		.title = std::move(
			title
		) | rpl::map([](const TextWithEntities &value) {
			return value.text;
		}),
	});
}

void LeftBoxHost::setAdditionalTitle(rpl::producer<QString> additional) {
}

void LeftBoxHost::setCloseByOutsideClick(bool close) {
}

rpl::producer<int> LeftBoxHost::layerHeightMaxValue() {
	return heightValue();
}

rpl::producer<int> LeftBoxHost::contentHeightMaxValue() {
	return _contentHeight.value();
}

void LeftBoxHost::setCustomCornersFilling(RectParts corners) {
}

void LeftBoxHost::clearButtons() {
	for (auto &button : base::take(_buttons)) {
		button.destroy();
	}
	_leftButton.destroy();
	updateControlsGeometry();
}

void LeftBoxHost::addButton(object_ptr<Ui::AbstractButton> button) {
	_buttons.push_back(std::move(button));
	const auto raw = _buttons.back().data();
	raw->setParent(this);
	raw->show();
	raw->widthValue(
	) | rpl::on_next([=] {
		updateControlsGeometry();
	}, raw->lifetime());
	updateControlsGeometry();
}

void LeftBoxHost::addLeftButton(object_ptr<Ui::AbstractButton> button) {
	_leftButton = std::move(button);
	const auto raw = _leftButton.data();
	raw->setParent(this);
	raw->show();
	raw->widthValue(
	) | rpl::on_next([=] {
		updateControlsGeometry();
	}, raw->lifetime());
	updateControlsGeometry();
}

void LeftBoxHost::addTopButton(object_ptr<Ui::AbstractButton> button) {
	_topBar->addButton(
		base::unique_qptr<Ui::AbstractButton>(button.release()));
	updateControlsGeometry();
}

void LeftBoxHost::showLoading(bool show) {
}

void LeftBoxHost::updateButtonsPositions() {
	updateControlsGeometry();
}

void LeftBoxHost::showBox(
		object_ptr<Ui::BoxContent> box,
		Ui::LayerOptions options,
		anim::type animated) {
	_controller->show(std::move(box), options, animated);
}

void LeftBoxHost::setDimensions(
		int newWidth,
		int maxHeight,
		bool forceCenterPosition) {
}

void LeftBoxHost::setNoContentMargin(bool noContentMargin) {
}

bool LeftBoxHost::isBoxShown() const {
	return !_closing;
}

void LeftBoxHost::closeBox() {
	if (_closing) {
		return;
	}
	_closing = true;
	_content->notifyBoxClosing();
	crl::on_main(this, [=] {
		_close();
	});
}

void LeftBoxHost::hideLayer() {
	_controller->hideLayer();
	closeBox();
}

void LeftBoxHost::triggerButton(int index) {
	if (index >= 0 && index < int(_buttons.size())) {
		_buttons[index]->clicked(Qt::KeyboardModifiers(), Qt::LeftButton);
	}
}

Ui::ShowFactory LeftBoxHost::showFactory() {
	const auto controller = _controller;
	return [=]() -> Ui::ShowPtr {
		return controller->uiShow();
	};
}

QPointer<QWidget> LeftBoxHost::outerContainer() {
	return parentWidget();
}

void LeftBoxHost::resizeEvent(QResizeEvent *e) {
	updateControlsGeometry();
}

void LeftBoxHost::paintEvent(QPaintEvent *e) {
	QPainter(this).fillRect(e->rect(), style().bg);
}

void LeftBoxHost::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Escape && _content->closeByEscape()) {
		closeBox();
	} else {
		RpWidget::keyPressEvent(e);
	}
}

int LeftBoxHost::buttonsHeight() const {
	if (_buttons.empty() && !_leftButton) {
		return 0;
	}
	const auto &st = _st ? *_st : st::defaultBox;
	return st.buttonPadding.top()
		+ st.buttonHeight
		+ st.buttonPadding.bottom();
}

void LeftBoxHost::updateControlsGeometry() {
	_topBar->resizeToWidth(width());
	_topBar->moveToLeft(0, 0);
	const auto top = _topBar->height();
	const auto available = std::max(height() - top - buttonsHeight(), 0);
	_content->setGeometry(0, top, width(), available);
	_contentHeight = available;

	const auto &st = style();
	const auto padding = st.buttonPadding;
	const auto buttonTop = height() - padding.bottom() - st.buttonHeight;
	const auto wide = width() - padding.left() - padding.right();
	auto right = padding.right();
	if (_leftButton) {
		_leftButton->moveToLeft(right, buttonTop);
	}
	for (const auto &button : _buttons) {
		if (st.buttonWide && wide > 0 && button->width() != wide) {
			button->resizeToWidth(wide);
		}
		button->moveToRight(right, buttonTop);
		right += button->width() + padding.left();
	}
}

} // namespace AyuDesign
