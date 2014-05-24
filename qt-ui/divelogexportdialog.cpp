#include <QFileDialog>
#include <QString>
#include <QShortcut>
#include <QAbstractButton>
#include <QDebug>
#include <QSettings>

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
	} else if (ui->exportSubsurfaceXML->isChecked()) {
		ui->description->setText("Subsurface native XML format.");
	}
}

void DiveLogExportDialog::on_exportGroup_buttonClicked(QAbstractButton *button)
{
	showExplanation();
}

void DiveLogExportDialog::on_buttonBox_accepted()
{
	QString filename;
	QString stylesheet;
	QSettings settings;
	QString lastDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastDir")) {
		if (QDir::setCurrent(settings.value("LastDir").toString())) {
			lastDir = settings.value("LastDir").toString();
		}
	}
	settings.endGroup();

	if (ui->exportUDDF->isChecked()) {
		stylesheet = "uddf-export.xslt";
		filename = QFileDialog::getSaveFileName(this, tr("Export UDDF File as"), lastDir,
							tr("UDDF files (*.uddf *.UDDF)"));
	} else if (ui->exportCSV->isChecked()) {
		stylesheet = "xml2csv.xslt";
		filename = QFileDialog::getSaveFileName(this, tr("Export CSV File as"), lastDir,
							tr("CSV files (*.csv *.CSV)"));
	} else if (ui->exportDivelogs->isChecked()) {
		DivelogsDeWebServices::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
	} else if (ui->exportWorldMap->isChecked()) {
		filename = QFileDialog::getSaveFileName(this, tr("Export World Map"), lastDir,
							tr("HTML files (*.html)"));
		if (!filename.isNull() && !filename.isEmpty())
			export_worldmap_HTML(filename.toUtf8().data(), ui->exportSelected->isChecked());
	} else if (ui->exportSubsurfaceXML->isChecked()) {
		filename = QFileDialog::getSaveFileName(this, tr("Export Subsurface XML"), lastDir,
							tr("XML files (*.xml *.ssrf)"));
		if (!filename.isNull() && !filename.isEmpty()) {
			QByteArray bt = QFile::encodeName(filename);
			save_dives_logic(bt.data(), true);
		}
	}
	if (!filename.isNull() && !filename.isEmpty()) {
		// remember the last export path
		QFileInfo fileInfo(filename);
		settings.beginGroup("FileDialog");
		settings.setValue("LastDir", fileInfo.dir().path());
		settings.endGroup();
		// the non XSLT exports are called directly above, the XSLT based ons are called here
		if (!stylesheet.isEmpty())
			export_dives_xslt(filename.toUtf8(), ui->exportSelected->isChecked(), stylesheet.toStdString().c_str());
	}
}
