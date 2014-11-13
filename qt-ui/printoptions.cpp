#include "printoptions.h"
#include "../display.h"

PrintOptions::PrintOptions(QWidget *parent, struct print_options *printOpt)
{
	hasSetupSlots = false;
	ui.setupUi(this);
	if (parent)
		setParent(parent);
	if (!printOpt)
		return;
	setup(printOpt);
}

void PrintOptions::setup(struct print_options *printOpt)
{
	printOptions = printOpt;
	// print type radio buttons
	switch (printOptions->type) {
	case print_options::PRETTY:
		ui.radioSixDives->setChecked(true);
		break;
	case print_options::TWOPERPAGE:
		ui.radioTwoDives->setChecked(true);
		break;
	case print_options::ONEPERPAGE:
		ui.radioOneDive->setChecked(true);
		break;
	case print_options::TABLE:
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

	connect(ui.radioSixDives, SIGNAL(clicked(bool)), this, SLOT(radioSixDivesClicked(bool)));
	connect(ui.radioTwoDives, SIGNAL(clicked(bool)), this, SLOT(radioTwoDivesClicked(bool)));
	connect(ui.radioOneDive, SIGNAL(clicked(bool)), this, SLOT(radioOneDiveClicked(bool)));
	connect(ui.radioTablePrint, SIGNAL(clicked(bool)), this, SLOT(radioTablePrintClicked(bool)));

	connect(ui.printInColor, SIGNAL(clicked(bool)), this, SLOT(printInColorClicked(bool)));
	connect(ui.printSelected, SIGNAL(clicked(bool)), this, SLOT(printSelectedClicked(bool)));

	connect(ui.notesOnTop, SIGNAL(clicked(bool)), this, SLOT(notesOnTopClicked(bool)));
	connect(ui.profileOnTop, SIGNAL(clicked(bool)), this, SLOT(profileOnTopClicked(bool)));
	hasSetupSlots = true;
}

// print type radio buttons
void PrintOptions::radioSixDivesClicked(bool check)
{
	printOptions->type = print_options::PRETTY;
}

void PrintOptions::radioTwoDivesClicked(bool check)
{
	printOptions->type = print_options::TWOPERPAGE;
}

void PrintOptions::radioOneDiveClicked(bool check)
{
	printOptions->type = print_options::ONEPERPAGE;
}

void PrintOptions::radioTablePrintClicked(bool check)
{
	printOptions->type = print_options::TABLE;
}

// general print option checkboxes
void PrintOptions::printInColorClicked(bool check)
{
	printOptions->color_selected = check;
}

void PrintOptions::printSelectedClicked(bool check)
{
	printOptions->print_selected = check;
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
