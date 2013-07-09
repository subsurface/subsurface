#include "printoptions.h"
#include "ui_printoptions.h"

PrintOptions::PrintOptions(QWidget *parent, struct options *printOpt)
: ui( new Ui::PrintOptions())
{
	if (parent)
		setParent(parent);
	if (!printOpt)
		return;
	printOptions = printOpt;
	ui->setupUi(this);
	// make height sliders update the labels next to them
	connect(ui->sliderPHeight, SIGNAL(sliderMoved(int)), this, SLOT(sliderPHeightMoved(int)));
	connect(ui->sliderOHeight, SIGNAL(sliderMoved(int)), this, SLOT(sliderOHeightMoved(int)));
	connect(ui->sliderNHeight, SIGNAL(sliderMoved(int)), this, SLOT(sliderNHeightMoved(int)));
	initSliderWithLabel(ui->sliderPHeight, ui->valuePHeight, printOptions->profile_height);
	initSliderWithLabel(ui->sliderOHeight, ui->valueOHeight, printOptions->notes_height);
	initSliderWithLabel(ui->sliderNHeight, ui->valueNHeight, printOptions->tanks_height);
}

void PrintOptions::initSliderWithLabel(QSlider *slider, QLabel *label, int value)
{
	slider->setValue(value);
	label->setText(formatSliderValueText(value));
}

QString PrintOptions::formatSliderValueText(int value)
{
	QString str = QString("%1%").arg(QString::number(value));
	return str;
}

void PrintOptions::sliderPHeightMoved(int value)
{
	ui->valuePHeight->setText(formatSliderValueText(value));
	printOptions->profile_height = value;
}

void PrintOptions::sliderOHeightMoved(int value)
{
	ui->valueOHeight->setText(formatSliderValueText(value));
	printOptions->notes_height = value;
}

void PrintOptions::sliderNHeightMoved(int value)
{
	ui->valueNHeight->setText(formatSliderValueText(value));
	printOptions->tanks_height = value;
}
