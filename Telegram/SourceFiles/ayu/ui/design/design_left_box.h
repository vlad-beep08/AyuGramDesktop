// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ui/layers/box_content.h"
#include "ui/rp_widget.h"

namespace Info {
class TopBar;
} // namespace Info

namespace Window {
class SessionController;
} // namespace Window

namespace AyuDesign {

class LeftBoxHost final
	: public Ui::RpWidget
	, public Ui::BoxContentDelegate {
public:
	LeftBoxHost(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		object_ptr<Ui::BoxContent> content,
		Fn<void()> close);
	~LeftBoxHost();

	using Ui::RpWidget::show;

	void setInnerFocus();

	void setLayerType(bool layerType) override;
	void setStyle(const style::Box &st) override;
	const style::Box &style() override;
	void setTitle(
		rpl::producer<TextWithEntities> title,
		Ui::Text::MarkedContext context = {}) override;
	void setAdditionalTitle(rpl::producer<QString> additional) override;
	void setCloseByOutsideClick(bool close) override;
	rpl::producer<int> layerHeightMaxValue() override;
	rpl::producer<int> contentHeightMaxValue() override;
	void setCustomCornersFilling(RectParts corners) override;
	void clearButtons() override;
	void addButton(object_ptr<Ui::AbstractButton> button) override;
	void addLeftButton(object_ptr<Ui::AbstractButton> button) override;
	void addTopButton(object_ptr<Ui::AbstractButton> button) override;
	void showLoading(bool show) override;
	void updateButtonsPositions() override;
	void showBox(
		object_ptr<Ui::BoxContent> box,
		Ui::LayerOptions options,
		anim::type animated) override;
	void setDimensions(
		int newWidth,
		int maxHeight,
		bool forceCenterPosition = false) override;
	void setNoContentMargin(bool noContentMargin) override;
	bool isBoxShown() const override;
	void closeBox() override;
	void hideLayer() override;
	void triggerButton(int index) override;
	Ui::ShowFactory showFactory() override;
	QPointer<QWidget> outerContainer() override;

protected:
	void resizeEvent(QResizeEvent *e) override;
	void paintEvent(QPaintEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;

private:
	void updateControlsGeometry();
	[[nodiscard]] int buttonsHeight() const;

	const not_null<Window::SessionController*> _controller;
	const Fn<void()> _close;
	object_ptr<Info::TopBar> _topBar;
	object_ptr<Ui::BoxContent> _content;
	std::vector<object_ptr<Ui::AbstractButton>> _buttons;
	object_ptr<Ui::AbstractButton> _leftButton = { nullptr };
	const style::Box *_st = nullptr;
	rpl::variable<int> _contentHeight = 0;
	bool _closing = false;

};

} // namespace AyuDesign
