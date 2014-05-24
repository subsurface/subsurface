#include <QFileDialog>
#include <QString>
#include <QShortcut>
#include <QAbstractButton>
#include <QDebug>
#include <QButtonGroup>

#include "mainwindow.h"
#include "divelogexportdialog.h"
#include "ui_divelogexportdialog.h"
#include "subsurfacewebservices.h"
#include "worldmap-save.h"

DiveLogExportDialog::DiveLogExportDialog(QWidget *parent) : QDialog(parent),
	ui(new Ui::DiveLogExportDialog)
{
	ui->setupUi(this);
	showExplanation();
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
}

DiveLogExportDialog::~DiveLogExportDialog()
{
	delete ui;
}

void DiveLogExportDialog::showExplanation()
{
	if (ui->exportUDDF->isChecked()) {
		ui->description->setText("Generic format that is used for data exchange between a variety of diving related programs.");
	} else if (ui->exportCSV->isChecked()) {
		ui->description->setText("Comma separated values that include the most relevant information of the dive profile.");
	} else if (ui->exportDivelogs->isChecked()) {
		ui->description->setText("Send the dive data to Divelogs.de website.");
	} else if (ui->exportWorldMap->isChecked()) {
		ui->description->setText("HTML export of the dive locations, visualized on a world map.");
	}
}

void DiveLogExportDialog::on_exportGroup_buttonClicked(QAbstractButton *button)
{
	showExplanation();
}

void DiveLogExportDialog::on_buttonBox_accepted()
{
	QFileInfo fi(system_default_filename());
	QString filename;
	QString stylesheet;

	if (ui->exportUDDF->isChecked()) {
		stylesheet = "uddf-export.xslt";
		filename = QFileDialog::getSaveFileName(this, tr("Export UDDF File as"), fi.absolutePath(),
							tr("UDDF files (*.uddf *.UDDF)"));
	} else if (ui->exportCSV->isChecked()) {
		stylesheet = "xml2csv.xslt";
		filename = QFileDialog::getSaveFileName(this, tr("Export CSV File as"), fi.absolutePath(),
							tr("CSV files (*.csv *.CSV)"));
	} else if (ui->exportDivelogs->isChecked()) {
		DivelogsDeWebServices::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
		return;
	} else if (ui->exportWorldMap->isChecked()) {
		filename = QFileDialog::getSaveFileName(this, tr("Export World Map"), fi.absolutePath(),
							tr("HTML files (*.html)"));
		if (!filename.isNull() && !filename.isEmpty())
			export_worldmap_HTML(filename.toUtf8().data(), ui->exportSelected->isChecked());
		return;
	}

	if (!filename.isNull() && !filename.isEmpty())
		export_dives_xslt(filename.toUtf8(), ui->exportSelected->isChecked(), stylesheet.toStdString().c_str());
}
