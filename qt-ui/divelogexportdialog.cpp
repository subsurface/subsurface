#include <QFileDialog>
#include <QString>

#include "mainwindow.h"
#include "divelogexportdialog.h"
#include "ui_divelogexportdialog.h"
#include "subsurfacewebservices.h"
#include "worldmap-save.h"

DiveLogExportDialog::DiveLogExportDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DiveLogExportDialog)
{
	ui->setupUi(this);
}

DiveLogExportDialog::~DiveLogExportDialog()
{
	delete ui;
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
