#include <QFileDialog>
#include <QString>
#include <QShortcut>
#include <QAbstractButton>
#include <QTextStream>
#include <QSettings>
#include <QDir>
#include <QDebug>

#include "mainwindow.h"
#include "divelogexportdialog.h"
#include "ui_divelogexportdialog.h"
#include "subsurfacewebservices.h"
#include "worldmap-save.h"
#include "save-html.h"
#include "helpers.h"

DiveLogExportDialog::DiveLogExportDialog(QWidget *parent) : QDialog(parent),
	ui(new Ui::DiveLogExportDialog)
{
	ui->setupUi(this);
	showExplanation();
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));

	/* the names are not the actual values exported to the json files,The font-family property should hold several
	font names as a "fallback" system, to ensure maximum compatibility between browsers/operating systems */
	ui->fontSelection->addItem("Arial", "Arial, Helvetica, sans-serif");
	ui->fontSelection->addItem("Impact", "Impact, Charcoal, sans-serif");
	ui->fontSelection->addItem("Georgia", "Georgia, serif");
	ui->fontSelection->addItem("Courier", "Courier, monospace");
	ui->fontSelection->addItem("Verdana", "Verdana, Geneva, sans-serif");

	QSettings settings;
	settings.beginGroup("HTML");
	if (settings.contains("fontSelection")) {
		ui->fontSelection->setCurrentIndex(settings.value("fontSelection").toInt());
	}
	if (settings.contains("fontSizeSelection")) {
		ui->fontSizeSelection->setCurrentIndex(settings.value("fontSizeSelection").toInt());
	}
	if (settings.contains("themeSelection")) {
		ui->themeSelection->setCurrentIndex(settings.value("themeSelection").toInt());
	}
	settings.endGroup();
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

void DiveLogExportDialog::exportHtmlInit(const QString &filename)
{
	QFile file(filename);
	QFileInfo info(file);
	QDir mainDir = info.absoluteDir();
	mainDir.mkdir(file.fileName() + "_files");
	QString exportFiles = file.fileName() + "_files/";

	QString json_dive_data = exportFiles + QDir::separator() + "file.json";
	QString json_settings = exportFiles + QDir::separator() + "settings.json";

	exportHTMLsettings(json_settings);
	export_HTML(json_dive_data.toUtf8().data(), ui->exportSelectedDives->isChecked(), ui->exportListOnly->isChecked());

	QString searchPath = getSubsurfaceDataPath("theme");
	if (searchPath.isEmpty())
		return;

	searchPath += QDir::separator();

	QFile::copy(searchPath + "dive_export.html", filename);
	QFile::copy(searchPath + "list_lib.js", exportFiles + "list_lib.js");
	QFile::copy(searchPath + "poster.png", exportFiles + "poster.png");

	// Remove theme file if already exist in the export dir
	QFile theme(exportFiles + "theme.css");
	if (theme.exists())
		theme.remove();

	QFile::copy(searchPath + (ui->themeSelection->currentText() == "Light" ? "light.css" : "sand.css"),
		    exportFiles + "theme.css");
}

void DiveLogExportDialog::exportHTMLsettings(const QString &filename)
{
	QSettings settings;
	settings.beginGroup("HTML");
	settings.setValue("fontSelection", ui->fontSelection->currentIndex());
	settings.setValue("fontSizeSelection", ui->fontSizeSelection->currentIndex());
	settings.setValue("themeSelection", ui->themeSelection->currentIndex());
	settings.endGroup();

	QString fontSize = ui->fontSizeSelection->currentText();
	QString fontFamily = ui->fontSelection->itemData(ui->fontSelection->currentIndex()).toString();
	QFile file(filename);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);
	out << "settings = {\"fontSize\":\"" << fontSize << "\",\"fontFamily\":\"" << fontFamily << "\",\"listOnly\":\""<<
	ui->exportListOnly->isChecked() << "\",}";
	file.close();
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

	switch (ui->tabWidget->currentIndex()) {
	case 0:
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
		break;
	case 1:
		filename = QFileDialog::getSaveFileName(this, tr("Export HTML files as"), lastDir,
							tr("HTML files (*.html)"));
		if (!filename.isNull() && !filename.isEmpty())
			exportHtmlInit(filename);
		break;
	}

	if (!filename.isNull() && !filename.isEmpty()) {
		// remember the last export path
		QFileInfo fileInfo(filename);
		settings.beginGroup("FileDialog");
		settings.setValue("LastDir", fileInfo.dir().path());
		settings.endGroup();
		// the non XSLT exports are called directly above, the XSLT based ons are called here
		if (!stylesheet.isEmpty())
			export_dives_xslt(filename.toUtf8(), ui->exportSelected->isChecked(), stylesheet.toUtf8());
	}
}
