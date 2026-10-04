// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ui/effects/animations.h"
#include "ui/widgets/buttons.h"

namespace AyuFeatures::QuickPhrase {

[[nodiscard]] QString Text();

class Button final : public Ui::IconButton {
public:
	explicit Button(QWidget *parent);

protected:
	void paintEvent(QPaintEvent *e) override;
	void onStateChanged(State was, StateChangeSource source) override;

private:
	const QString _label;
	QFont _font;
	Ui::Animations::Simple _bounce;

};

} // namespace AyuFeatures::QuickPhrase
