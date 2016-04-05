#include "printoptions.h"
#include "templateedit.h"
#include "core/helpers.h"

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

	setupTemplates();

	// general print option checkboxes
	ui.printInColor->setChecked(printOptions->color_selected);
	ui.printSelected->setChecked(printOptions->print_selected);

	// connect slots only once
	if (hasSetupSlots)
		return;

	connect(ui.printInColor, SIGNAL(clicked(bool)), this, SLOT(printInColorClicked(bool)));
	connect(ui.printSelected, SIGNAL(clicked(bool)), this, SLOT(printSelectedClicked(bool)));

	hasSetupSlots = true;
}

void PrintOptions::setupTemplates()
{
	QStringList currList = printOptions->type == print_options::DIVELIST ?
		grantlee_templates : grantlee_statistics_templates;

	qSort(currList);
	int current_index = 0;
	ui.printTemplate->clear();
	Q_FOREACH(const QString& theme, currList) {
		if (theme == printOptions->p_template){
			current_index = currList.indexOf(theme);
		}
		ui.printTemplate->addItem(theme.split('.')[0], theme);
	}
	ui.printTemplate->setCurrentIndex(current_index);
}

// print type radio buttons
void PrintOptions::on_radioDiveListPrint_toggled(bool check)
{
	if (check) {
		printOptions->type = print_options::DIVELIST;

		// print options
		ui.printSelected->setEnabled(true);

		// print template
		ui.deleteButton->setEnabled(true);
		ui.exportButton->setEnabled(true);
		ui.importButton->setEnabled(true);

		setupTemplates();
	}
}

void PrintOptions::on_radioStatisticsPrint_toggled(bool check)
{
	if (check) {
		printOptions->type = print_options::STATISTICS;

		// print options
		ui.printSelected->setEnabled(false);

		// print template
		ui.deleteButton->setEnabled(false);
		ui.exportButton->setEnabled(false);
		ui.importButton->setEnabled(false);

		setupTemplates();
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
	QString filename = QFileDialog::getOpenFileName(this, tr("Import template file"), "",
							tr("HTML files (*.html)"));
	if (filename.isEmpty())
		return;
	QFileInfo fileInfo(filename);
	QFile::copy(filename, getPrintingTemplatePathUser() + QDir::separator() + fileInfo.fileName());
	printOptions->p_template = fileInfo.fileName();
	find_all_templates();
	setup();
}

void PrintOptions::on_exportButton_clicked()
{
	QString filename = QFileDialog::getSaveFileName(this, tr("Export template files as"), "",
							tr("HTML files (*.html)"));
	if (filename.isEmpty())
		return;
	QFile::copy(getPrintingTemplatePathUser() + QDir::separator() + getSelectedTemplate(), filename);
}

void PrintOptions::on_deleteButton_clicked()
{
	QString templateName = getSelectedTemplate();
	QMessageBox msgBox;
	msgBox.setText(tr("This action cannot be undone!"));
	msgBox.setInformativeText(tr("Delete template: %1?").arg(templateName));
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	if (msgBox.exec() == QMessageBox::Ok) {
		QFile f(getPrintingTemplatePathUser() + QDir::separator() + templateName);
		f.remove();
		find_all_templates();
		setup();
	}
}

QString PrintOptions::getSelectedTemplate()
{
	return ui.printTemplate->currentData().toString();
}
