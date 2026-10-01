// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ui/widgets/buttons.h"

namespace AyuDesign {

class ListRow final : public Ui::RippleButton {
public:
	ListRow(
		QWidget *parent,
		const QString &title,
		const QString &label = QString());

	void setSelected(bool selected);
	[[nodiscard]] bool selected() const;

	int resizeGetHeight(int newWidth) override;

protected:
	void paintEvent(QPaintEvent *e) override;
	void onStateChanged(State was, StateChangeSource source) override;
	QImage prepareRippleMask() const override;
	QPoint prepareRippleStartPosition() const override;

private:
	[[nodiscard]] QRect surfaceRect() const;

	QString _title;
	QString _label;
	bool _selected = false;

};

class ThemeCard final : public Ui::RippleButton {
public:
	ThemeCard(
		QWidget *parent,
		const QString &title,
		std::vector<QColor> swatches);

	void setActive(bool active);

	int resizeGetHeight(int newWidth) override;

protected:
	void paintEvent(QPaintEvent *e) override;
	void onStateChanged(State was, StateChangeSource source) override;
	QImage prepareRippleMask() const override;
	QPoint prepareRippleStartPosition() const override;

private:
	[[nodiscard]] QRect surfaceRect() const;

	QString _title;
	std::vector<QColor> _swatches;
	bool _active = false;

};

} // namespace AyuDesign
