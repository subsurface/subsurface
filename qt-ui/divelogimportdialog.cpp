#include <QtDebug>
#include <QFileDialog>
#include <QShortcut>
#include "divelogimportdialog.h"
#include "mainwindow.h"
#include "ui_divelogimportdialog.h"
#include <QAbstractListModel>
#include <QAbstractTableModel>

const DiveLogImportDialog::CSVAppConfig DiveLogImportDialog::CSVApps[CSVAPPS] = {
	// time, depth, temperature, po2, cns, ndl, tts, stopdepth, pressure
	{ "", },
	{ "APD Log Viewer", 1, 2, 16, 7, 18, -1, -1, 19, -1, "Tab" },
	{ "XP5", 1, 2, 10, -1, -1, -1, -1, -1, -1, "Tab" },
	{ "SensusCSV", 10, 11, -1, -1, -1, -1, -1, -1, -1, "," },
	{ "Seabear CSV", 1, 2, 6, -1, -1, 3, 4, 5, 7, ";" },
	{ "SubsurfaceCSV", -1, -1, -1, -1, -1, -1, -1, -1, -1, "," },
	{ NULL, }
};

ColumnNameProvider::ColumnNameProvider(QObject *parent)
{

}

bool ColumnNameProvider::insertRows(int row, int count, const QModelIndex &parent)
{

}

bool ColumnNameProvider::removeRows(int row, int count, const QModelIndex &parent)
{

}

bool ColumnNameProvider::setData(const QModelIndex &index, const QVariant &value, int role)
{

}

QVariant ColumnNameProvider::data(const QModelIndex &index, int role) const
{

}

int ColumnNameProvider::rowCount(const QModelIndex &parent) const
{

}

DiveLogImportDialog::DiveLogImportDialog(QStringList *fn, QWidget *parent) : QDialog(parent),
	selector(true),
	ui(new Ui::DiveLogImportDialog)
{
	ui->setupUi(this);
	fileNames = *fn;
	column = 0;

	/* Add indexes of XSLTs requiring special handling to the list */
	specialCSV << 3;
	specialCSV << 5;

	for (int i = 0; !CSVApps[i].name.isNull(); ++i)
		ui->knownImports->addItem(CSVApps[i].name);

	ui->CSVSeparator->addItems( QStringList() << tr("Separator") << tr("Tab") << ";" << ",");
	ui->knownImports->setCurrentIndex(1);

	/* manually import CSV file */
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
	/*
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

				// Seabear CSV stores NDL and TTS in Minutes, not seconds
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
					  ui->DateFormat->currentIndex(),
					  ui->DurationFormat->currentIndex(),
					  VALUE_IF_CHECKED(DiveNumber),
					  VALUE_IF_CHECKED(Date), VALUE_IF_CHECKED(Time),
					  VALUE_IF_CHECKED(Duration), VALUE_IF_CHECKED(Location),
					  VALUE_IF_CHECKED(Gps), VALUE_IF_CHECKED(MaxDepth),
					  VALUE_IF_CHECKED(MeanDepth), VALUE_IF_CHECKED(Buddy),
					  VALUE_IF_CHECKED(Notes), VALUE_IF_CHECKED(Weight),
					  VALUE_IF_CHECKED(Tags),
					  VALUE_IF_CHECKED(CylinderSize), VALUE_IF_CHECKED(StartPressure),
					  VALUE_IF_CHECKED(EndPressure), VALUE_IF_CHECKED(O2),
					  VALUE_IF_CHECKED(He), VALUE_IF_CHECKED(AirTemp),
					  VALUE_IF_CHECKED(WaterTemp));
		}
	}
*/
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
	if (index == 0)
		return;

}

void DiveLogImportDialog::unknownImports()
{
	if (!specialCSV.contains(ui->knownImports->currentIndex()))
		ui->knownImports->setCurrentIndex(0);
}
