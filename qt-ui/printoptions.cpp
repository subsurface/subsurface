#include "printoptions.h"
#include "../display.h"

PrintOptions::PrintOptions(QWidget *parent, struct options *printOpt)
{
	hasSetupSlots = false;
	ui.setupUi(this);
	if (parent)
		setParent(parent);
	if (!printOpt)
		return;
	setup(printOpt);
}

void PrintOptions::setup(struct options *printOpt)
{
	printOptions = printOpt;
	// layout height sliders
	initSliderWithLabel(ui.sliderPHeight, ui.valuePHeight, printOptions->profile_height);
	initSliderWithLabel(ui.sliderOHeight, ui.valueOHeight, printOptions->notes_height);
	initSliderWithLabel(ui.sliderNHeight, ui.valueNHeight, printOptions->tanks_height);
	// print type radio buttons
	switch (printOptions->type) {
	case options::PRETTY:
		ui.radioSixDives->setChecked(true);
		break;
	case options::TWOPERPAGE:
		ui.radioTwoDives->setChecked(true);
		break;
	case options::TABLE:
		ui.radioTablePrint->setChecked(true);
		break;
	}
	// general print option checkboxes
	if (printOptions->color_selected)
		ui.printInColor->setChecked(true);
	if (printOptions->print_selected)
		ui.printSelected->setChecked(true);
	// ordering
	if (printOptions->notes_up)
		ui.notesOnTop->setChecked(true);
	else
		ui.profileOnTop->setChecked(true);

	// connect slots only once
	if (hasSetupSlots)
		return;
	connect(ui.sliderPHeight, SIGNAL(sliderMoved(int)), this, SLOT(sliderPHeightMoved(int)));
	connect(ui.sliderOHeight, SIGNAL(sliderMoved(int)), this, SLOT(sliderOHeightMoved(int)));
	connect(ui.sliderNHeight, SIGNAL(sliderMoved(int)), this, SLOT(sliderNHeightMoved(int)));

	connect(ui.radioSixDives, SIGNAL(clicked(bool)), this, SLOT(radioSixDivesClicked(bool)));
	connect(ui.radioTwoDives, SIGNAL(clicked(bool)), this, SLOT(radioTwoDivesClicked(bool)));
	connect(ui.radioTablePrint, SIGNAL(clicked(bool)), this, SLOT(radioTablePrintClicked(bool)));

	connect(ui.printInColor, SIGNAL(clicked(bool)), this, SLOT(printInColorClicked(bool)));
	connect(ui.printSelected, SIGNAL(clicked(bool)), this, SLOT(printSelectedClicked(bool)));

	connect(ui.notesOnTop, SIGNAL(clicked(bool)), this, SLOT(notesOnTopClicked(bool)));
	connect(ui.profileOnTop, SIGNAL(clicked(bool)), this, SLOT(profileOnTopClicked(bool)));
	hasSetupSlots = true;
}

// layout height sliders
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
	ui.valuePHeight->setText(formatSliderValueText(value));
	printOptions->profile_height = value;
}

void PrintOptions::sliderOHeightMoved(int value)
{
	ui.valueOHeight->setText(formatSliderValueText(value));
	printOptions->notes_height = value;
}

void PrintOptions::sliderNHeightMoved(int value)
{
	ui.valueNHeight->setText(formatSliderValueText(value));
	printOptions->tanks_height = value;
}

// print type radio buttons
void PrintOptions::radioSixDivesClicked(bool check)
{
	printOptions->type = options::PRETTY;
}

void PrintOptions::radioTwoDivesClicked(bool check)
{
	printOptions->type = options::TWOPERPAGE;
}

void PrintOptions::radioTablePrintClicked(bool check)
{
	printOptions->type = options::TABLE;
}

// general print option checkboxes
void PrintOptions::printInColorClicked(bool check)
{
	printOptions->color_selected = (int)check;
}

void PrintOptions::printSelectedClicked(bool check)
{
	printOptions->print_selected = (int)check;
}

// ordering
void PrintOptions::notesOnTopClicked(bool check)
{
	printOptions->notes_up = true;
}

void PrintOptions::profileOnTopClicked(bool check)
{
	printOptions->notes_up = false;
}
