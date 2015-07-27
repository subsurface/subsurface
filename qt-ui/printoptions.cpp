#include "printoptions.h"
#include "templateedit.h"
#include "helpers.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

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
	case print_options::STATISTICS:
		ui.radioStatisticsPrint->setChecked(true);
		break;
	}

	// insert existing templates in the UI and select the current template
	qSort(grantlee_templates);
	int current_index = 0, index = 0;
	for (QList<QString>::iterator i = grantlee_templates.begin(); i != grantlee_templates.end(); ++i) {
		if ((*i).compare(printOptions->p_template) == 0) {
			current_index = index;
			break;
		}
		index++;
	}
	ui.printTemplate->clear();
	for (QList<QString>::iterator i = grantlee_templates.begin(); i != grantlee_templates.end(); ++i) {
		ui.printTemplate->addItem((*i).split('.')[0], QVariant::fromValue(*i));
	}
	ui.printTemplate->setCurrentIndex(current_index);

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

void PrintOptions::on_importButton_clicked()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Import Template file"), "",
							tr("HTML files (*.html)"));
	QFileInfo fileInfo(filename);
	QFile::copy(filename, getSubsurfaceDataPath("printing_templates") + QDir::separator() + fileInfo.fileName());
	find_all_templates();
	setup();
}

void PrintOptions::on_exportButton_clicked()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Export Template files as"), "",
							tr("HTML files (*.html)"));
	QFile::copy(getSubsurfaceDataPath("printing_templates") + QDir::separator() + getSelectedTemplate(), filename);
}

void PrintOptions::on_deleteButton_clicked()
{
	QString templateName = getSelectedTemplate();
	QMessageBox msgBox;
	msgBox.setText("This action cannot be undone!");
	msgBox.setInformativeText("Delete '" + templateName + "' template?");
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	if (msgBox.exec() == QMessageBox::Ok) {
		QFile f(getSubsurfaceDataPath("printing_templates") + QDir::separator() + templateName);
		f.remove();
		find_all_templates();
		setup();
	}
}

QString PrintOptions::getSelectedTemplate()
{
	return ui.printTemplate->currentData().toString();
}
