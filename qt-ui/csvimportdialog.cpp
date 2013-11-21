#include <QtDebug>
#include <QFileDialog>
#include "csvimportdialog.h"
#include "mainwindow.h"
#include "ui_csvimportdialog.h"

const CSVImportDialog::CSVAppConfig CSVImportDialog::CSVApps[CSVAPPS] = {
		{"", },
		{"APD Log Viewer", 0, 1, 15, "Tab"},
		{"XP5", 0, 1, 9, "Tab"},
		{NULL,}
};

CSVImportDialog::CSVImportDialog(QWidget *parent) :
	QDialog(parent),
	selector(true),
	ui(new Ui::CSVImportDialog)
{
	ui->setupUi(this);

	for (int i = 0; !CSVApps[i].name.isNull(); ++i)
		ui->knownImports->addItem(CSVApps[i].name);

	ui->CSVSeparator->addItem("Tab");
	ui->knownImports->setCurrentIndex(1);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(ui->CSVDepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->CSVTime, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->CSVTemperature, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
}

CSVImportDialog::~CSVImportDialog()
{
	delete ui;
}

void CSVImportDialog::on_buttonBox_accepted()
{
	char *error = NULL;

	parse_csv_file(ui->CSVFile->text().toUtf8().data(), ui->CSVTime->value(), ui->CSVDepth->value(), ui->CSVTemperature->value(), &error);
	if (error != NULL) {

		mainWindow()->showError(error);
		free(error);
		error = NULL;
	}
	process_dives(TRUE, FALSE);

	mainWindow()->refreshDisplay();
}

void CSVImportDialog::on_CSVFileSelector_clicked()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open CSV Log File"), ".", tr("CSV Files (*.csv)"));
	ui->CSVFile->setText(filename);
	if (filename.isEmpty())
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	else
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void CSVImportDialog::on_knownImports_currentIndexChanged(int index)
{
	if (index == 0)
		return;

	ui->CSVTime->blockSignals(true);
	ui->CSVDepth->blockSignals(true);
	ui->CSVTemperature->blockSignals(true);
	ui->CSVTime->setValue(CSVApps[index].time);
	ui->CSVDepth->setValue(CSVApps[index].depth);
	ui->CSVTemperature->setValue(CSVApps[index].temperature);
	ui->CSVTime->blockSignals(false);
	ui->CSVDepth->blockSignals(false);
	ui->CSVTemperature->blockSignals(false);
}

void CSVImportDialog::unknownImports(int arg1)
{
	unknownImports();
}

void CSVImportDialog::unknownImports()
{
	ui->knownImports->setCurrentIndex(0);
}

void CSVImportDialog::on_CSVFile_textEdited()
{
	if (ui->CSVFile->text().isEmpty())
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	else
		ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
