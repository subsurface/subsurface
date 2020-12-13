// SPDX-License-Identifier: GPL-2.0
#include "printoptions.h"
#include "templateedit.h"
#include "templatelayout.h"
#include "core/qthelper.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

PrintOptions::PrintOptions(QWidget *parent, print_options &printOpt, template_options &templateOpt) :
	printOptions(printOpt),
	templateOptions(templateOpt)
{
	hasSetupSlots = false;
	ui.setupUi(this);
	if (parent)
		setParent(parent);
	setup();
}

void PrintOptions::setup()
{
	// print type radio buttons
	switch (printOptions.type) {
	case print_options::DIVELIST:
		ui.radioDiveListPrint->setChecked(true);
		break;
	case print_options::STATISTICS:
		ui.radioStatisticsPrint->setChecked(true);
		break;
	}

	setupTemplates();

	// general print option checkboxes
	ui.printInColor->setChecked(printOptions.color_selected);
	ui.printSelected->setChecked(printOptions.print_selected);

	// resolution
	ui.resolution->setValue(printOptions.resolution);

	// connect slots only once
	if (hasSetupSlots)
		return;

	connect(ui.printInColor, SIGNAL(clicked(bool)), this, SLOT(printInColorClicked(bool)));
	connect(ui.printSelected, SIGNAL(clicked(bool)), this, SLOT(printSelectedClicked(bool)));
	connect(ui.resolution, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
		printOptions.resolution = value;
	});
	hasSetupSlots = true;
}

void PrintOptions::setupTemplates()
{
	QStringList currList = printOptions.type == print_options::DIVELIST ?
		grantlee_templates : grantlee_statistics_templates;

	// temp. store the template from options, as addItem() updates it via:
	// on_printTemplate_currentIndexChanged()
	QString storedTemplate = printOptions.p_template;
	currList.sort();
	int current_index = 0;
	ui.printTemplate->clear();
	Q_FOREACH(const QString& theme, currList) {
		 // find the stored template in the list
		if (theme == storedTemplate || theme == lastImportExportTemplate)
			current_index = currList.indexOf(theme);
		ui.printTemplate->addItem(theme.split('.')[0], theme);
	}
	ui.printTemplate->setCurrentIndex(current_index);
	lastImportExportTemplate = "";
}

// print type radio buttons
void PrintOptions::on_radioDiveListPrint_toggled(bool check)
{
	if (check) {
		printOptions.type = print_options::DIVELIST;

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
		printOptions.type = print_options::STATISTICS;

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
	printOptions.color_selected = check;
}

void PrintOptions::printSelectedClicked(bool check)
{
	printOptions.print_selected = check;
}


void PrintOptions::on_printTemplate_currentIndexChanged(int index)
{
	printOptions.p_template = ui.printTemplate->itemData(index).toString();
}

void PrintOptions::on_editButton_clicked()
{
	QString templateName = getSelectedTemplate();
	QString prefix = (printOptions.type == print_options::STATISTICS) ? "statistics/" : "";
	QFile f(getPrintingTemplatePathUser() + QDir::separator() + prefix + templateName);
	if (!f.open(QFile::ReadWrite | QFile::Text)) {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Read-only template!"));
		msgBox.setText(tr("The template '%1' is read-only and cannot be edited.\n"
			"Please export this template to a different file.").arg(templateName));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
		return;
	} else {
		f.close();
	}
	TemplateEdit te(this, printOptions, templateOptions);
	te.exec();
	setup();
}

void PrintOptions::on_importButton_clicked()
{
	QString pathUser = getPrintingTemplatePathUser();
	QString filename = QFileDialog::getOpenFileName(this, tr("Import template file"), pathUser,
							tr("HTML files") + " (*.html)");
	if (filename.isEmpty())
		return;
	QFileInfo fileInfo(filename);

	const QString dest = pathUser + QDir::separator() + fileInfo.fileName();
	QFile f(dest);
	if (!f.open(QFile::ReadWrite | QFile::Text)) {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Read-only template!"));
		msgBox.setText(tr("The destination template '%1' is read-only and cannot be overwritten.").arg(fileInfo.fileName()));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
		return;
	} else {
		f.close();
		if (filename != dest)
			f.remove();
	}

	QFile::copy(filename, dest);
	printOptions.p_template = fileInfo.fileName();
	lastImportExportTemplate = fileInfo.fileName();
	find_all_templates();
	setup();
}

void PrintOptions::on_exportButton_clicked()
{
	QString pathUser = getPrintingTemplatePathUser();
	QString filename = QFileDialog::getSaveFileName(this, tr("Export template files as"), pathUser,
							tr("HTML files") + " (*.html)");
	if (filename.isEmpty())
		return;
	const QString ext(".html");
	if (filename.endsWith(".htm", Qt::CaseInsensitive))
		filename += "l";
	else if (!filename.endsWith(ext, Qt::CaseInsensitive))
		filename += ext;
	QFileInfo fileInfo(filename);
	const QString dest = pathUser + QDir::separator() + getSelectedTemplate();

	QFile f(filename);
	if (!f.open(QFile::ReadWrite | QFile::Text)) {
		QMessageBox msgBox(this);
		msgBox.setWindowTitle(tr("Read-only template!"));
		msgBox.setText(tr("The destination template '%1' is read-only and cannot be overwritten.").arg(fileInfo.fileName()));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
		return;
	} else {
		f.close();
		if (dest != filename)
			f.remove();
	}

	QFile::copy(dest, filename);
	if (!f.open(QFile::ReadWrite | QFile::Text))
		f.setPermissions(QFileDevice::ReadUser | QFileDevice::ReadOwner | QFileDevice::WriteUser | QFileDevice::WriteOwner);
	else
		f.close();
	lastImportExportTemplate = fileInfo.fileName();
	find_all_templates();
	setup();
}

void PrintOptions::on_deleteButton_clicked()
{
	QString templateName = getSelectedTemplate();
	QMessageBox msgBox(this);
	msgBox.setWindowTitle(tr("This action cannot be undone!"));
	msgBox.setText(tr("Delete template '%1'?").arg(templateName));
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	if (msgBox.exec() == QMessageBox::Ok) {
		QFile f(getPrintingTemplatePathUser() + QDir::separator() + templateName);
		if (!f.open(QFile::ReadWrite | QFile::Text)) {
			msgBox.setWindowTitle(tr("Read-only template!"));
			msgBox.setText(tr("The template '%1' is read-only and cannot be deleted.").arg(templateName));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.exec();
			return;
		} else {
			f.close();
		}
		f.remove();
		find_all_templates();
		setup();
	}
}

QString PrintOptions::getSelectedTemplate()
{
	return ui.printTemplate->currentData().toString();
}
