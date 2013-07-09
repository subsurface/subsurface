#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include "../display.h"

namespace Ui {
	class PrintOptions;
};

// should be based on a custom QPrintDialog class
class PrintOptions : public QWidget {
Q_OBJECT

public:
	explicit PrintOptions(QWidget *parent = 0, struct options *printOpt = 0);

private:
	Ui::PrintOptions *ui;
	void setLabelFromSlider(QSlider *slider, QLabel *label);
	void initSliderWithLabel(QSlider *slider, QLabel *label, int value);
	QString formatSliderValueText(int value);
	struct options *printOptions;

private slots:
	void sliderPHeightMoved(int value);
	void sliderOHeightMoved(int value);
	void sliderNHeightMoved(int value);
};

#endif
