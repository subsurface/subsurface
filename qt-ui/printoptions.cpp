#include "printoptions.h"
#include <QDebug>

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
	case print_options::DIVELIST:
		ui.radioDiveListPrint->setChecked(true);
		break;
	case print_options::TABLE:
		ui.radioTablePrint->setChecked(true);
		break;
	case print_options::STATISTICS:
		ui.radioStatisticsPrint->setChecked(true);
		break;
	}
	switch (printOptions->p_template) {
	case print_options::ONE_DIVE:
		ui.printTemplate->setCurrentIndex(0);
		break;
	case print_options::TWO_DIVE:
		ui.printTemplate->setCurrentIndex(1);
		break;
	}

	// general print option checkboxes
	if (printOptions->color_selected)
		ui.printInColor->setChecked(true);
	if (printOptions->print_selected)
		ui.printSelected->setChecked(true);

	// connect slots only once
	if (hasSetupSlots)
		return;

	connect(ui.printInColor, SIGNAL(clicked(bool)), this, SLOT(printInColorClicked(bool)));
	connect(ui.printSelected, SIGNAL(clicked(bool)), this, SLOT(printSelectedClicked(bool)));

	hasSetupSlots = true;
}

// print type radio buttons
void PrintOptions::on_radioDiveListPrint_clicked(bool check)
{
	if (check) {
		printOptions->type = print_options::DIVELIST;
	}
}

void PrintOptions::on_radioTablePrint_clicked(bool check)
{
	if (check) {
		printOptions->type = print_options::TABLE;
	}
}

void PrintOptions::on_radioStatisticsPrint_clicked(bool check)
{
	if (check) {
		printOptions->type = print_options::STATISTICS;
	}
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


void PrintOptions::on_printTemplate_currentIndexChanged(int index)
{
    switch(index){
	case 0:
		printOptions->p_template = print_options::ONE_DIVE;
	break;
	case 1:
		printOptions->p_template = print_options::TWO_DIVE;
	break;
    }
}
