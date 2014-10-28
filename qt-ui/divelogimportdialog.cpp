#include <QtDebug>
#include <QFileDialog>
#include <QShortcut>
#include "divelogimportdialog.h"
#include "mainwindow.h"
#include "ui_divelogimportdialog.h"

const DiveLogImportDialog::CSVAppConfig DiveLogImportDialog::CSVApps[CSVAPPS] = {
	// time, depth, temperature, po2, cns, ndl, tts, stopdepth, pressure
	{ "", },
	{ "APD Log Viewer", 1, 2, 16, 7, 18, -1, -1, 19, -1, "Tab" },
	{ "XP5", 1, 2, 10, -1, -1, -1, -1, -1, -1, "Tab" },
	{ "SensusCSV", 10, 11, -1, -1, -1, -1, -1, -1, -1, "," },
	{ "Seabear CSV", 1, 2, 6, -1, -1, 3, 4, 5, 7, ";" },
	{ NULL, }
};

DiveLogImportDialog::DiveLogImportDialog(QStringList *fn, QWidget *parent) : QDialog(parent),
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
	ui->ManualSeparator->addItem("Tab");
	ui->ManualSeparator->addItem(",");
	ui->ManualSeparator->addItem(";");
	ui->knownImports->setCurrentIndex(1);

	connect(ui->CSVDepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->CSVTime, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->CSVTemperature, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->temperatureCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVpo2, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->po2CheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVcns, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->cnsCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVndl, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->ndlCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVtts, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->ttsCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVstopdepth, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->stopdepthCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	connect(ui->CSVpressure, SIGNAL(valueChanged(int)), this, SLOT(unknownImports()));
	connect(ui->pressureCheckBox, SIGNAL(clicked(bool)), this, SLOT(unknownImports()));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

DiveLogImportDialog::~DiveLogImportDialog()
{
	delete ui;
}

#define VALUE_IF_CHECKED(x) (ui->x->isEnabled() ? ui->x->value() - 1 : -1)
void DiveLogImportDialog::on_buttonBox_accepted()
{
	if (ui->tabWidget->currentIndex() == 0) {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (ui->knownImports->currentText() == QString("Seabear CSV")) {
				parse_seabear_csv_file(fileNames[i].toUtf8().data(), ui->CSVTime->value() - 1,
				       ui->CSVDepth->value() - 1, VALUE_IF_CHECKED(CSVTemperature),
				       VALUE_IF_CHECKED(CSVpo2),
				       VALUE_IF_CHECKED(CSVcns),
				       VALUE_IF_CHECKED(CSVndl),
				       VALUE_IF_CHECKED(CSVtts),
				       VALUE_IF_CHECKED(CSVstopdepth),
				       VALUE_IF_CHECKED(CSVpressure),
				       ui->CSVSeparator->currentIndex(),
				       specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
				       ui->CSVUnits->currentIndex());

				/* Seabear CSV stores NDL and TTS in Minutes, not seconds */
				struct dive *dive = dive_table.dives[dive_table.nr - 1];
				for(int s_nr = 0 ; s_nr <= dive->dc.samples ; s_nr++) {
					struct sample *sample = dive->dc.sample + s_nr;
					sample->ndl.seconds *= 60;
					sample->tts.seconds *= 60;
				}
			} else
				parse_csv_file(fileNames[i].toUtf8().data(), ui->CSVTime->value() - 1,
						ui->CSVDepth->value() - 1, VALUE_IF_CHECKED(CSVTemperature),
						VALUE_IF_CHECKED(CSVpo2),
						VALUE_IF_CHECKED(CSVcns),
						VALUE_IF_CHECKED(CSVndl),
						VALUE_IF_CHECKED(CSVtts),
						VALUE_IF_CHECKED(CSVstopdepth),
						VALUE_IF_CHECKED(CSVpressure),
						ui->CSVSeparator->currentIndex(),
						specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
						ui->CSVUnits->currentIndex());
		}
	} else {
		for (int i = 0; i < fileNames.size(); ++i) {
			parse_manual_file(fileNames[i].toUtf8().data(),
					  ui->ManualSeparator->currentIndex(),
					  ui->Units->currentIndex(),
					  VALUE_IF_CHECKED(DiveNumber),
					  VALUE_IF_CHECKED(Date), VALUE_IF_CHECKED(Time),
					  VALUE_IF_CHECKED(Duration), VALUE_IF_CHECKED(Location),
					  VALUE_IF_CHECKED(Gps), VALUE_IF_CHECKED(MaxDepth),
					  VALUE_IF_CHECKED(MeanDepth), VALUE_IF_CHECKED(Buddy),
					  VALUE_IF_CHECKED(Notes), VALUE_IF_CHECKED(Weight),
					  VALUE_IF_CHECKED(Tags));
		}
	}

	process_dives(true, false);

	MainWindow::instance()->refreshDisplay();
}

#define SET_VALUE_AND_CHECKBOX(CSV, BOX, VAL) ({\
		ui->CSV->blockSignals(true);\
		ui->CSV->setValue(VAL);\
		ui->CSV->setEnabled(VAL >= 0);\
		ui->BOX->setChecked(VAL >= 0);\
		ui->CSV->blockSignals(false); })
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
	SET_VALUE_AND_CHECKBOX(CSVndl, ndlCheckBox, CSVApps[index].ndl);
	SET_VALUE_AND_CHECKBOX(CSVtts, ttsCheckBox, CSVApps[index].tts);
	SET_VALUE_AND_CHECKBOX(CSVstopdepth, stopdepthCheckBox, CSVApps[index].stopdepth);
	SET_VALUE_AND_CHECKBOX(CSVpressure, pressureCheckBox, CSVApps[index].pressure);
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
