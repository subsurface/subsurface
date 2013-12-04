#include <QtDebug>
#include <QFileDialog>
#include "csvimportdialog.h"
#include "mainwindow.h"
#include "ui_csvimportdialog.h"

const CSVImportDialog::CSVAppConfig CSVImportDialog::CSVApps[CSVAPPS] = {
		{"", },
		{"APD Log Viewer", 0, 1, 15, 6, 17, 18, "Tab"},
		{"XP5", 0, 1, 9, -1, -1, -1, "Tab"},
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
	ui->CSVSeparator->addItem(",");
	ui->knownImports->setCurrentIndex(1);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(ui->CSVDepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->CSVTime, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->CSVTemperature, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->temperatureCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
	connect(ui->CSVpo2, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->po2CheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
	connect(ui->CSVcns, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->cnsCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
	connect(ui->CSVstopdepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports(int)));
	connect(ui->stopdepthCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports(bool)));
}

CSVImportDialog::~CSVImportDialog()
{
	delete ui;
}

#define VALUE_IF_CHECKED(x) (ui->x->isEnabled() ? ui->x->value() : -1)
void CSVImportDialog::on_buttonBox_accepted()
{
	char *error = NULL;

	parse_csv_file(ui->CSVFile->text().toUtf8().data(), ui->CSVTime->value(),
			ui->CSVDepth->value(), VALUE_IF_CHECKED(CSVTemperature),
			VALUE_IF_CHECKED(CSVpo2),
			VALUE_IF_CHECKED(CSVcns),
			VALUE_IF_CHECKED(CSVstopdepth),
			ui->CSVSeparator->currentIndex(),
			&error);
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

#define SET_VALUE_AND_CHECKBOX(CSV, BOX, VAL) ({\
		ui->CSV->blockSignals(true);\
		ui->CSV->setValue(VAL);\
		ui->CSV->setEnabled(VAL >= 0);\
		ui->BOX->setChecked(VAL >= 0);\
		ui->CSV->blockSignals(false);\
		})
void CSVImportDialog::on_knownImports_currentIndexChanged(int index)
{
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
}

void CSVImportDialog::unknownImports(bool arg1)
{
	unknownImports();
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
