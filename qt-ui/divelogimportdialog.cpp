#include <QtDebug>
#include <QFileDialog>
#include "divelogimportdialog.h"
#include "mainwindow.h"
#include "ui_divelogimportdialog.h"

const DiveLogImportDialog::CSVAppConfig DiveLogImportDialog::CSVApps[CSVAPPS] = {
		{"", },
		{"APD Log Viewer", 1, 2, 16, 7, 18, 19, "Tab"},
		{"XP5", 1, 2, 10, -1, -1, -1, "Tab"},
		{"SensusCSV", 10, 11, -1, -1, -1, -1, ","},
		{NULL,}
};

DiveLogImportDialog::DiveLogImportDialog(QStringList *fn, QWidget *parent) :
	QDialog(parent),
	selector(true),
	ui(new Ui::DiveLogImportDialog)
{
	ui->setupUi(this);
	fileNames = *fn;

	/* Add indexes of XSLTs requiring special handling to the list */
	specialCSV << 3;

	for (int i = 0; !CSVApps[i].name.isNull(); ++i)
		ui->knownImports->addItem(CSVApps[i].name);

	ui->CSVSeparator->addItem("Tab");
	ui->CSVSeparator->addItem(",");
	ui->CSVSeparator->addItem(";");
	ui->knownImports->setCurrentIndex(1);

	connect(ui->CSVDepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->CSVTime, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->CSVTemperature, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->temperatureCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVpo2, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->po2CheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVcns, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->cnsCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVstopdepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->stopdepthCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVSeparator, SIGNAL(currentIndexChanged(int)), this, SLOT(unknownImports()));
}

DiveLogImportDialog::~DiveLogImportDialog()
{
	delete ui;
}

#define VALUE_IF_CHECKED(x) (ui->x->isEnabled() ? ui->x->value() - 1: -1)
void DiveLogImportDialog::on_buttonBox_accepted()
{
	char *error = NULL;
	for (int i = 0; i < fileNames.size(); ++i) {
		parse_csv_file(fileNames[i].toUtf8().data(), ui->CSVTime->value() - 1,
				ui->CSVDepth->value() - 1, VALUE_IF_CHECKED(CSVTemperature),
				VALUE_IF_CHECKED(CSVpo2),
				VALUE_IF_CHECKED(CSVcns),
				VALUE_IF_CHECKED(CSVstopdepth),
				ui->CSVSeparator->currentIndex(),
				specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
				&error);
		if (error != NULL) {
			mainWindow()->showError(error);
			free(error);
			error = NULL;
		}
	}
	process_dives(true, false);

	mainWindow()->refreshDisplay();
}

#define SET_VALUE_AND_CHECKBOX(CSV, BOX, VAL) ({\
		ui->CSV->blockSignals(true);\
		ui->CSV->setValue(VAL);\
		ui->CSV->setEnabled(VAL >= 0);\
		ui->BOX->setChecked(VAL >= 0);\
		ui->CSV->blockSignals(false);\
		})
void DiveLogImportDialog::on_knownImports_currentIndexChanged(int index)
{
	if (specialCSV.contains(index)) {
		ui->groupBox_3->setEnabled(false);
	} else {
		ui->groupBox_3->setEnabled(true);
	}
	if (index == 0)
		return;

	ui->CSVTime->blockSignals(true);
	ui->CSVDepth->blockSignals(true);
	ui->CSVTime->setValue(CSVApps[index].time);
	ui->CSVDepth->setValue(CSVApps[index].depth);
	ui->CSVTime->blockSignals(false);
	ui->CSVDepth->blockSignals(false);
	SET_VALUE_AND_CHECKBOX(CSVTemperature, temperatureCheckBox, CSVApps[index].temperature);
	SET_VALUE_AND_CHECKBOX(CSVpo2, po2CheckBox, CSVApps[index].po2);
	SET_VALUE_AND_CHECKBOX(CSVcns, cnsCheckBox, CSVApps[index].cns);
	SET_VALUE_AND_CHECKBOX(CSVstopdepth, stopdepthCheckBox, CSVApps[index].stopdepth);
	ui->CSVSeparator->blockSignals(true);
	int separator_index = ui->CSVSeparator->findText(CSVApps[index].separator);
	if (separator_index != -1)
		ui->CSVSeparator->setCurrentIndex(separator_index);
	ui->CSVSeparator->blockSignals(false);
}

void DiveLogImportDialog::unknownImports()
{
	if (!specialCSV.contains(ui->knownImports->currentIndex()))
		ui->knownImports->setCurrentIndex(0);
}
