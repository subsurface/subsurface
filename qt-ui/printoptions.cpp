#include "printoptions.h"
#include "templateedit.h"
#include <QDebug>

PrintOptions::PrintOptions(QWidget *parent, struct print_options *printOpt, struct template_options *templateOpt)
{
	hasSetupSlots = false;
	ui.setupUi(this);
	if (parent)
		setParent(parent);
	if (!printOpt || !templateOpt)
		return;
	templateOptions = templateOpt;
	printOptions = printOpt;
	setup();
}

void PrintOptions::setup()
{
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

	// insert existing templates in the UI
	ui.printTemplate->clear();
	qSort(grantlee_templates);
	for (QList<QString>::iterator i = grantlee_templates.begin(); i != grantlee_templates.end(); ++i) {
		ui.printTemplate->addItem((*i).split('.')[0], QVariant::fromValue(*i));
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
	printOptions->p_template = ui.printTemplate->itemData(index).toString();
}

void PrintOptions::on_editButton_clicked()
{
	TemplateEdit te(this, printOptions, templateOptions);
	te.exec();
	setup();
}
