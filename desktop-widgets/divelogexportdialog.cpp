// SPDX-License-Identifier: GPL-2.0
#include <QFileDialog>
#include <QShortcut>
#include <QSettings>
#include <string.h> // Allows string comparisons and substitutions in TeX export

#include "ui_divelogexportdialog.h"
#include "core/divelogexportlogic.h"
#include "core/worldmap-save.h"
#include "core/save-html.h"
#include "core/settings/qPrefDisplay.h"
#include "core/save-profiledata.h"
#include "core/divefilter.h"
#include "core/divesite.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/tag.h"
#include "backend-shared/exportfuncs.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelogexportdialog.h"
#include "desktop-widgets/diveshareexportdialog.h"
#include "desktop-widgets/subsurfacewebservices.h"
#include "profile-widget/profilewidget2.h"

// Retrieves the current unit settings defined in the Subsurface preferences.
#define GET_UNIT(name, field, f, t)           \
	v = settings.value(QString(name));        \
	if (v.isValid())                          \
		field = (v.toInt() == 0) ? (t) : (f); \
	else                                      \
		field = default_prefs.units.field

DiveLogExportDialog::DiveLogExportDialog(QWidget *parent) : QDialog(parent),
	ui(new Ui::DiveLogExportDialog)
{
	ui->setupUi(this);
	showExplanation();
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), MainWindow::instance(), SLOT(close()));
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
	if (settings.contains("subsurfaceNumbers")) {
		ui->exportSubsurfaceNumber->setChecked(settings.value("subsurfaceNumbers").toBool());
	}
	if (settings.contains("yearlyStatistics")) {
		ui->exportStatistics->setChecked(settings.value("yearlyStatistics").toBool());
	}
	if (settings.contains("listOnly")) {
		ui->exportListOnly->setChecked(settings.value("listOnly").toBool());
	}
	if (settings.contains("exportPhotos")) {
		ui->exportPhotos->setChecked(settings.value("exportPhotos").toBool());
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
		ui->description->setText(tr("Generic format that is used for data exchange between a variety of diving related programs."));
	} else if (ui->exportCSV->isChecked()) {
		ui->description->setText(tr("Comma separated values describing the dive profile as downloaded from dive computer."));
	} else if (ui->exportCSVDetails->isChecked()) {
		ui->description->setText(tr("Comma separated values of the dive information. This includes most of the dive details but no profile information."));
	} else if (ui->exportDivelogs->isChecked()) {
		ui->description->setText(tr("Send the dive data to divelogs.de website."));
	} else if (ui->exportDiveshare->isChecked()) {
		ui->description->setText(tr("Send the dive data to dive-share.appspot.com website."));
	} else if (ui->exportWorldMap->isChecked()) {
		ui->description->setText(tr("HTML export of the dive locations, visualized on a world map."));
	} else if (ui->exportSubsurfaceXML->isChecked()) {
		ui->description->setText(tr("Subsurface native XML format."));
	} else if (ui->exportSubsurfaceSitesXML->isChecked()) {
		ui->description->setText(tr("Subsurface dive sites native XML format."));
	} else if (ui->exportImageDepths->isChecked()) {
		ui->description->setText(tr("Write depths of images to file."));
	} else if (ui->exportTeX->isChecked()) {
		ui->description->setText(tr("Write dive as TeX macros to file."));
	} else if (ui->exportLaTeX->isChecked()) {
		ui->description->setText(tr("Write dive as LaTeX macros to file."));
	} else if (ui->exportProfile->isChecked()) {
		ui->description->setText(tr("Write the profile image as PNG file."));
	} else if (ui->exportProfileData->isChecked()) {
		ui->description->setText(tr("Write the computed Profile Panel data to a CSV file."));
	}
}

void DiveLogExportDialog::exportHtmlInit(const QString &filename)
{
	struct htmlExportSetting hes;
	hes.themeFile = (ui->themeSelection->currentText() == tr("Light")) ? "light.css" : "sand.css";
	hes.exportPhotos = ui->exportPhotos->isChecked();
	hes.selectedOnly = ui->exportSelectedDives->isChecked();
	hes.listOnly = ui->exportListOnly->isChecked();
	hes.fontFamily = ui->fontSelection->itemData(ui->fontSelection->currentIndex()).toString();
	hes.fontSize = ui->fontSizeSelection->currentText();
	hes.themeSelection = ui->themeSelection->currentIndex();
	hes.subsurfaceNumbers = ui->exportSubsurfaceNumber->isChecked();
	hes.yearlyStatistics = ui->exportStatistics->isChecked();

	exportHtmlInitLogic(filename, hes);
}

void DiveLogExportDialog::on_exportGroup_buttonClicked(QAbstractButton*)
{
	showExplanation();
}

void DiveLogExportDialog::on_buttonBox_accepted()
{
	QString filename;
	QString stylesheet;
	QString lastDir = QDir::homePath();

	if (QDir(qPrefDisplay::lastDir()).exists())
		lastDir = qPrefDisplay::lastDir();

	switch (ui->tabWidget->currentIndex()) {
	case 0:
		if (ui->exportUDDF->isChecked()) {
			stylesheet = "uddf-export.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export UDDF file as"), lastDir,
								tr("UDDF files") + " (*.uddf)");
		} else if (ui->exportCSV->isChecked()) {
			stylesheet = "xml2csv.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export CSV file as"), lastDir,
								tr("CSV files") + " (*.csv)");
		} else if (ui->exportCSVDetails->isChecked()) {
			stylesheet = "xml2manualcsv.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export CSV file as"), lastDir,
								tr("CSV files") + " (*.csv)");
		} else if (ui->exportDivelogs->isChecked()) {
			DivelogsDeWebServices::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
		} else if (ui->exportDiveshare->isChecked()) {
			DiveShareExportDialog::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
		} else if (ui->exportWorldMap->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export world map"), lastDir,
								tr("HTML files") + " (*.html)");
			if (!filename.isNull() && !filename.isEmpty())
				export_worldmap_HTML(qPrintable(filename), ui->exportSelected->isChecked());
		} else if (ui->exportSubsurfaceXML->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export Subsurface XML"), lastDir,
								tr("Subsurface files") + " (*.ssrf *.xml)");
			if (!filename.isNull() && !filename.isEmpty()) {
				if (!filename.contains('.'))
					filename.append(".ssrf");
				QByteArray bt = QFile::encodeName(filename);
				save_dives_logic(bt.data(), ui->exportSelected->isChecked(), ui->anonymize->isChecked());
			}
		} else if (ui->exportSubsurfaceSitesXML->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export Subsurface dive sites XML"), lastDir,
								tr("Subsurface files") + " (*.xml)");
			if (!filename.isNull() && !filename.isEmpty()) {
				if (!filename.contains('.'))
					filename.append(".xml");
				QByteArray bt = QFile::encodeName(filename);
				std::vector<const dive_site *> sites = getDiveSitesToExport(ui->exportSelected->isChecked());
				save_dive_sites_logic(bt.data(), sites.data(), (int)sites.size(), ui->anonymize->isChecked());
			}
		} else if (ui->exportImageDepths->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Save image depths"), lastDir);
			if (!filename.isNull() && !filename.isEmpty())
				export_depths(qPrintable(filename), ui->exportSelected->isChecked());
		} else if (ui->exportTeX->isChecked() || ui->exportLaTeX->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export to TeX file"), lastDir, tr("TeX files") + " (*.tex)");
			if (!filename.isNull() && !filename.isEmpty())
				export_TeX(qPrintable(filename), ui->exportSelected->isChecked(), ui->exportTeX->isChecked());
		} else if (ui->exportProfile->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Save profile image"), lastDir);
			if (!filename.isNull() && !filename.isEmpty())
				exportProfile(qPrintable(filename), ui->exportSelected->isChecked());
		} else if (ui->exportProfileData->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Save profile data"), lastDir);
			if (!filename.isNull() && !filename.isEmpty())
				save_profiledata(qPrintable(filename), ui->exportSelected->isChecked());
		}
		break;
	case 1:
		filename = QFileDialog::getSaveFileName(this, tr("Export HTML files as"), lastDir,
							tr("HTML files") + " (*.html)");
		if (!filename.isNull() && !filename.isEmpty())
			exportHtmlInit(filename);
		break;
	}

	if (!filename.isNull() && !filename.isEmpty()) {
		// remember the last export path
		QFileInfo fileInfo(filename);
		qPrefDisplay::set_lastDir(fileInfo.dir().path());
		// the non XSLT exports are called directly above, the XSLT based ons are called here
		if (!stylesheet.isEmpty()) {
			QFuture<void> future = exportUsingStyleSheet(filename, ui->exportSelected->isChecked(),
					ui->CSVUnits_2->currentIndex(), stylesheet.toUtf8(), ui->anonymize->isChecked());
			MainWindow::instance()->getNotificationWidget()->showNotification(tr("Please wait, exporting..."), KMessageWidget::Information);
			MainWindow::instance()->getNotificationWidget()->setFuture(future);
		}
	}
}

void exportProfile(const struct dive *dive, const QString filename)
{
	ProfileWidget2 *profile = MainWindow::instance()->graphics;
	profile->setToolTipVisibile(false);
	profile->setPrintMode(true);
	double scale = profile->getFontPrintScale();
	profile->setFontPrintScale(4 * scale);
	profile->plotDive(dive, 0, true, false, true);
	QImage image = QImage(profile->size() * 4, QImage::Format_RGB32);
	QPainter paint;
	paint.begin(&image);
	profile->render(&paint);
	image.save(filename);
	profile->setToolTipVisibile(true);
	profile->setFontPrintScale(scale);
	profile->setPrintMode(false);
	profile->plotDive(dive, 0, true); // TODO: Shouldn't this plot the current dive?
}
