#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

#include <QWidget>
#include <QSlider>
#include <QLabel>

#include "ui_printoptions.h"

// should be based on a custom QPrintDialog class
class PrintOptions : public QWidget {
Q_OBJECT

public:
	explicit PrintOptions(QWidget *parent = 0, struct options *printOpt = 0);
	void setup(struct options *printOpt);

private:
	Ui::PrintOptions ui;
	void setLabelFromSlider(QSlider *slider, QLabel *label);
	void initSliderWithLabel(QSlider *slider, QLabel *label, int value);
	QString formatSliderValueText(int value);
	struct options *printOptions;
	bool hasSetupSlots;

private slots:
	void sliderPHeightMoved(int value);
	void sliderOHeightMoved(int value);
	void sliderNHeightMoved(int value);
	void radioSixDivesClicked(bool check);
	void radioTwoDivesClicked(bool check);
	void radioTablePrintClicked(bool check);
	void printInColorClicked(bool check);
	void printSelectedClicked(bool check);
	void notesOnTopClicked(bool check);
	void profileOnTopClicked(bool check);
};

#endif
