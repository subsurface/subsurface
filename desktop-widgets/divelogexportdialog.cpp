// SPDX-License-Identifier: GPL-2.0
#include <QFileDialog>
#include <QShortcut>
#include <QSettings>
#include <QtConcurrent>
#include <string.h> // Allows string comparisons and substitutions in TeX export

#include "desktop-widgets/divelogexportdialog.h"
#include "core/divelogexportlogic.h"
#include "desktop-widgets/diveshareexportdialog.h"
#include "ui_divelogexportdialog.h"
#include "desktop-widgets/subsurfacewebservices.h"
#include "core/worldmap-save.h"
#include "core/save-html.h"
#include "desktop-widgets/mainwindow.h"
#include "profile-widget/profilewidget2.h"
#include "core/dive.h"  // Allows access to helper functions in TeX export.

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
		ui->description->setText(tr("Comma separated values describing the dive profile."));
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
	} else if (ui->exportImageDepths->isChecked()) {
		ui->description->setText(tr("Write depths of images to file."));
	} else if (ui->exportTeX->isChecked()) {
		ui->description->setText(tr("Write dive as TeX macros to file."));
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

void DiveLogExportDialog::on_exportGroup_buttonClicked(QAbstractButton *button)
{
	Q_UNUSED(button)
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
			filename = QFileDialog::getSaveFileName(this, tr("Export UDDF file as"), lastDir,
								tr("UDDF files (*.uddf *.UDDF)"));
		} else if (ui->exportCSV->isChecked()) {
			stylesheet = "xml2csv.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export CSV file as"), lastDir,
								tr("CSV files (*.csv *.CSV)"));
		} else if (ui->exportCSVDetails->isChecked()) {
			stylesheet = "xml2manualcsv.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export CSV file as"), lastDir,
								tr("CSV files (*.csv *.CSV)"));
		} else if (ui->exportDivelogs->isChecked()) {
			DivelogsDeWebServices::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
		} else if (ui->exportDiveshare->isChecked()) {
			DiveShareExportDialog::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
		} else if (ui->exportWorldMap->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export world map"), lastDir,
								tr("HTML files (*.html)"));
			if (!filename.isNull() && !filename.isEmpty())
				export_worldmap_HTML(filename.toUtf8().data(), ui->exportSelected->isChecked());
		} else if (ui->exportSubsurfaceXML->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export Subsurface XML"), lastDir,
								tr("XML files (*.xml *.ssrf)"));
			if (!filename.isNull() && !filename.isEmpty()) {
				if (!filename.contains('.'))
					filename.append(".ssrf");
				QByteArray bt = QFile::encodeName(filename);
				save_dives_logic(bt.data(), ui->exportSelected->isChecked());
			}
		} else if (ui->exportImageDepths->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Save image depths"), lastDir);
			if (!filename.isNull() && !filename.isEmpty())
				export_depths(filename.toUtf8().data(), ui->exportSelected->isChecked());
		} else if (ui->exportTeX->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export to TeX file"), lastDir, tr("TeX files (*.tex)"));
			if (!filename.isNull() && !filename.isEmpty())
				export_TeX(filename.toUtf8().data(), ui->exportSelected->isChecked());
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
		if (!stylesheet.isEmpty()) {
			future = QtConcurrent::run(export_dives_xslt, filename.toUtf8(), ui->exportSelected->isChecked(), ui->CSVUnits_2->currentIndex(), stylesheet.toUtf8());
			MainWindow::instance()->getNotificationWidget()->showNotification(tr("Please wait, exporting..."), KMessageWidget::Information);
			MainWindow::instance()->getNotificationWidget()->setFuture(future);
		}
	}
}

void DiveLogExportDialog::export_depths(const char *filename, const bool selected_only)
{
	FILE *f;
	struct dive *dive;
	depth_t depth;
	int i;
	const char *unit = NULL;

	struct membuffer buf = {};

	for_each_dive (i, dive) {
		if (selected_only && !dive->selected)
			continue;

		FOR_EACH_PICTURE (dive) {
			int n = dive->dc.samples;
			struct sample *s = dive->dc.sample;
			depth.mm = 0;
			while (--n >= 0 && (int32_t)s->time.seconds <= picture->offset.seconds) {
				depth.mm = s->depth.mm;
				s++;
			}
			put_format(&buf, "%s\t%.1f", picture->filename, get_depth_units(depth.mm, NULL, &unit));
			put_format(&buf, "%s\n", unit);
		}
	}

	f = subsurface_fopen(filename, "w+");
	if (!f) {
		report_error(tr("Can't open file %s").toUtf8().data(), filename);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
	free_buffer(&buf);
}

void DiveLogExportDialog::export_TeX(const char *filename, const bool selected_only)
{
	FILE *f;
	struct dive *dive;
	struct units *units = get_units();
	const char *unit;
	int i;
	int tmp;
	bool need_pagebreak = false;

	struct membuffer buf = {};

	put_format(&buf, "\\input subsurfacetemplate\n");
	put_format(&buf, "%% This is a plain TeX file. Compile with pdftex, not pdflatex!\n");
	put_format(&buf, "%% You will also need a subsurfacetemplate.tex in the current directory.\n");
	put_format(&buf, "%% You can download an example from http://www.atdotde.de/~robert/subsurfacetemplate\n%%\n");
	put_format(&buf, "%%\n");
	put_format(&buf, "%% Notes: TeX/LaTex will not render the degree symbol correctly by default. In LaTeX, you may\n");
	put_format(&buf, "%% add the following line to the end of the preamble of your template to ensure correct output:\n");
	put_format(&buf, "%% \\usepackage[utf8]{inputenc}\n");
	put_format(&buf, "%% \\usepackage{gensymb}\n");
	put_format(&buf, "%% \\DeclareUnicodeCharacter{00B0}{\degree}\n"); //replaces ° with \degree
	put_format(&buf, "%%\n");

	/* Define text fields with the units used for export.  These values are set in the Subsurface Preferences
	 * and the text fields created here are included in the data fields below.
	 */
	put_format(&buf, "\n%% These fields contain the units used in other fields below. They may be\n");
	put_format(&buf, "%% referenced as needed in TeX templates.\n");
	put_format(&buf, "%% \n");
	put_format(&buf, "%% By default, Subsurface exports units of volume as \"ℓ\" and \"cuft\", which do\n");
	put_format(&buf, "%% not render well in TeX/LaTeX.  The code below substitutes \"L\" and \"ft$^{3}$\",\n");
	put_format(&buf, "%% respectively.  If you wish to display the original values, you may edit this\n");
	put_format(&buf, "%% list and all calls to those units will be updated in your document.\n");

	tmp = get_depth_units(1, NULL, &unit);
	put_format(&buf, "\\def\\depthunit{%s}\n", unit);
	tmp = get_weight_units(1, NULL, &unit);
	put_format(&buf, "\\def\\weightunit{%s}\n", unit);
	tmp = get_pressure_units(1, &unit);
	put_format(&buf, "\\def\\pressureunit{%s}\n", unit);
	tmp = get_temp_units(1, &unit);
	put_format(&buf, "\\def\\temperatureunit{%s}\n", unit);
	tmp = get_volume_units(1, NULL, &unit);
	if (strcmp(unit, "ℓ") == 0)
	{
		put_format(&buf, "\\def\\volumeunit{L}\n");
	}
	else if (strcmp(unit, "cuft") == 0)
	{
		put_format(&buf, "\\def\\volumeunit{ft$^{3}$}\n");
	}
	else
	{
		put_format(&buf, "\\def\\volumeunit{%s}\n", unit);
	}
	tmp = get_vertical_speed_units(1, NULL, &unit);
	put_format(&buf, "\\def\\verticalspeedunit{%s}\n", unit);

	put_format(&buf, "\n%%%%%%%%%% Begin Dive Data: %%%%%%%%%%\n");

	for_each_dive (i, dive) {
		if (selected_only && !dive->selected)
			continue;

		QString filename = "profile%1.png";
		ProfileWidget2 *profile = MainWindow::instance()->graphics();
		profile->plotDive(dive, true);
		profile->setToolTipVisibile(false);
		QPixmap pix = QPixmap::grabWidget(profile);
		profile->setToolTipVisibile(true);
		pix.save(filename.arg(dive->number));


		struct tm tm;
		utc_mkdate(dive->when, &tm);

		dive_site *site = get_dive_site_by_uuid(dive->dive_site_uuid);
		QRegExp ct("countrytag: (\\w+)");
		QString country;
		if (ct.indexIn(site->notes) >= 0)
			country = ct.cap(1);
		else
			country = "";

		pressure_t delta_p = {.mbar = 0};

		QString star = "*";
		QString viz = star.repeated(dive->visibility);
		QString rating = star.repeated(dive->rating);

		int i;
		int qty_cyl;
		int qty_weight;
		float total_weight;

		if (need_pagebreak)
			put_format(&buf, "\\vfill\\eject\n");
		need_pagebreak = true;
		put_format(&buf, "\n%% Time, Date, and location:\n");
		put_format(&buf, "\\def\\date{%04u-%02u-%02u}\n",
		      tm.tm_year, tm.tm_mon+1, tm.tm_mday);
		put_format(&buf, "\\def\\number{%d}\n", dive->number);
		put_format(&buf, "\\def\\sitename{%s}\n", site ? site->name : "");
		site ? put_format(&buf, "\\def\\gpslat{%f}\n", site->latitude.udeg / 1000000.0) : put_format(&buf, "\\def\\gpslat{}\n");
		site ? put_format(&buf, "\\def\\gpslon{%f}\n", site->longitude.udeg / 1000000.0) : put_format(&buf, "\\def\\gpslon{}\n");
		put_format(&buf, "\\def\\gpslat{%f}\n", site ? site->latitude.udeg / 1000000.0 : 0);
		put_format(&buf, "\\def\\gpslon{%f}\n", site ? site->longitude.udeg / 1000000.0 : 0);
		put_format(&buf, "\\def\\computer{%s}\n", dive->dc.model);
		put_format(&buf, "\\def\\country{%s}\n", country.toUtf8().data());
		put_format(&buf, "\\def\\time{%u:%02u}\n", FRACTION(dive->duration.seconds, 60));

		// Code has generally been reworked to use helper functions to access data, and to print a blank field "{}" if data is not plausible (i.e. 0 deg K)
		put_format(&buf, "\n%% Dive Profile Details:\n");
		get_temp_units(dive->maxtemp.mkelvin, &unit) > 0 ? put_format(&buf, "\\def\\maxtemp{%.1f\\temperatureunit}\n", get_temp_units(dive->maxtemp.mkelvin, &unit)) : put_format(&buf, "\\def\\maxtemp{}\n");
		get_temp_units(dive->mintemp.mkelvin, &unit) > 0 ? put_format(&buf, "\\def\\mintemp{%.1f\\temperatureunit}\n", get_temp_units(dive->mintemp.mkelvin, &unit)) : put_format(&buf, "\\def\\mintemp{}\n");
		get_temp_units(dive->watertemp.mkelvin, &unit) > 0 ? put_format(&buf, "\\def\\watertemp{%.1f\\temperatureunit}\n", get_temp_units(dive->watertemp.mkelvin, &unit)) : put_format(&buf, "\\def\\watertemp{}\n");
		get_temp_units(dive->airtemp.mkelvin, &unit) > 0 ? put_format(&buf, "\\def\\airtemp{%.1f\\temperatureunit}\n", get_temp_units(dive->airtemp.mkelvin, &unit)) : put_format(&buf, "\\def\\airtemp{}\n");
		get_depth_units(dive->maxdepth.mm, NULL, &unit) > 0 ? put_format(&buf, "\\def\\maximumdepth{%.1f \\depthunit}\n", get_depth_units(dive->maxdepth.mm, NULL, &unit)) : put_format(&buf, "\\def\\maximumdepth{}\n");
		get_depth_units(dive->meandepth.mm, NULL, &unit) > 0 ? put_format(&buf, "\\def\\meandepth{%.1f \\depthunit}\n", get_depth_units(dive->meandepth.mm, NULL, &unit)) : put_format(&buf, "\\def\\meandepth{}\n");

		//Code block for misc dive parameters
		put_format(&buf, "\\def\\type{%s}\n", dive->tag_list ? dive->tag_list->tag->name : ""); // this currently prints only a single tag.
		put_format(&buf, "\\def\\maxtemp{%.2f}\n", (dive->maxtemp.mkelvin) ? (dive->maxtemp.mkelvin / 1000.0) - 273.15 : 0);
		put_format(&buf, "\\def\\mintemp{%.2f}\n", (dive->mintemp.mkelvin) ? (dive->mintemp.mkelvin / 1000.0) - 273.15 : 0);
		put_format(&buf, "\\def\\watertemp{%.2f}\n", (dive->watertemp.mkelvin) ? (dive->watertemp.mkelvin / 1000.0) - 273.15 : 0);
		put_format(&buf, "\\def\\airtemp{%.2f}\n", (dive->airtemp.mkelvin) ? (dive->airtemp.mkelvin / 1000.0) - 273.15 : 0);
		put_format(&buf, "\\def\\maxdepth{%u.%01u m}\n", FRACTION(dive->maxdepth.mm / 100, 10));
		put_format(&buf, "\\def\\avedepth{%u.%01u m}\n", FRACTION(dive->meandepth.mm / 100, 10));

		//Code block prints start/end press. for all cylinders used, number of cyl. used, SAC, and total delta_p
		qty_cyl = 0;
		for (i = 0; i < MAX_CYLINDERS; i++)
			if (is_cylinder_used(dive, i)){
				put_format(&buf, "\\def\\cyl%ddescription{%s}\n", i + 1, dive->cylinder[i].type.description);
				if (dive->cylinder[i].gasmix.o2.permille > 0){ //This code assumes that all gas must have oxygen, so a zero value indicated that the gas in question is AIR
					put_format(&buf, "\\def\\cyl%dmixO2{%u\%}\n", i + 1, FRACTION(dive->cylinder[i].gasmix.o2.permille, 10));
					put_format(&buf, "\\def\\cyl%dmixN2{%u\%}\n", i + 1, FRACTION(1000 - dive->cylinder[i].gasmix.o2.permille - dive->cylinder[i].gasmix.he.permille, 10));
					put_format(&buf, "\\def\\cyl%dmixHe{%u\%}\n", i + 1, FRACTION(dive->cylinder[i].gasmix.he.permille, 10));
				} else {
					put_format(&buf, "\\def\\cyl%dmixO2{21\%}\n", i + 1);
					put_format(&buf, "\\def\\cyl%dmixN2{79\%}\n", i + 1);
					put_format(&buf, "\\def\\cyl%dmixHe{0\%}\n", i + 1);
				}
				delta_p.mbar += dive->cylinder[i].start.mbar - dive->cylinder[i].end.mbar;
				put_format(&buf, "\\def\\cyl%dstartpress{%u.%01u bar}\n", i + 1, FRACTION(dive->cylinder[i].start.mbar / 100, 10));
				put_format(&buf, "\\def\\cyl%dendpress{%u.%01u bar}\n", i + 1, FRACTION(dive->cylinder[i].end.mbar / 100, 10));
				qty_cyl += 1;
			}
		put_format(&buf, "\\def\\qtycyl{%d}\n", qty_cyl);
		put_format(&buf, "\\def\\gasuse{%u.%01u bar}\n", FRACTION(delta_p.mbar / 100, 10));
		put_format(&buf, "\\def\\sac{%u.%01u l/min}\n", FRACTION(dive->sac/100,10));

		//Code block prints all weights used.
		qty_weight = 0;
		total_weight = 0;
		for (i = 0; i < MAX_WEIGHTSYSTEMS; i++)
			if (dive->weightsystem[i].weight.grams != NULL){
				put_format(&buf, "\\def\\weight%dtype{%s}\n", i + 1, dive->weightsystem[i].description);
				put_format(&buf, "\\def\\weight%damt{%u.%01u kg}\n", i + 1, FRACTION(dive->weightsystem[i].weight.grams, 1000));
				qty_weight += 1;
				total_weight += dive->weightsystem[i].weight.grams;
			}

		put_format(&buf, "\\def\\qtyweights{%d}\n", qty_weight);
		put_format(&buf, "\\def\\totalweight{%u.%01u kg}\n", FRACTION(total_weight, 1000));

		put_format(&buf, "\\def\\type{%s}\n", dive->tag_list ? dive->tag_list->tag->name : "");
		put_format(&buf, "\\def\\viz{%s}\n", viz.toUtf8().data());
		put_format(&buf, "\\def\\rating{%s}\n", rating.toUtf8().data());
		put_format(&buf, "\\def\\plot{\\includegraphics[width=9cm,height=4cm]{profile%d}}\n", dive->number);
		put_format(&buf, "\\def\\comment{%s}\n", dive->notes ? dive->notes : "");
		put_format(&buf, "\\def\\buddy{%s}\n", dive->buddy ? dive->buddy : "");
		put_format(&buf, "\\def\\divemaster{%s}\n", dive->divemaster ? dive->divemaster : "");
		put_format(&buf, "\\def\\suit{%s}\n", dive->suit ? dive->suit : "");

		//Code block prints start/end press. for all cylinders defined in dive, number of cyl. used, SAC, and total delta_p
		put_format(&buf, "\n%% Gas use information:\n");
		qty_cyl = 0;
		for (i = 0; i < MAX_CYLINDERS; i++){

			/* Previously, this IF statement used "is_cylinder_used(...)" to print only the cylinders where pressure
			 * identified by Subsurface as being used.  Now it prints all cylinders that are associated with the
			 * dive. This way, bailout cylinders that are not needed are still shown, rather than disappearing,*/
			if (dive->cylinder[i].type.description != NULL){
				put_format(&buf, "\\def\\cyl%ddescription{%s}\n", i + 1, dive->cylinder[i].type.description);
				put_format(&buf, "\\def\\cyl%dmixO2{%.1f\\%}\n", i + 1, get_o2(&dive->cylinder[i].gasmix)/10.0);
				put_format(&buf, "\\def\\cyl%dmixHe{%.1f\\%}\n", i + 1, get_he(&dive->cylinder[i].gasmix)/10.0);
				put_format(&buf, "\\def\\cyl%dmixN2{%.1f\\%}\n", i + 1, (100.0 - (get_o2(&dive->cylinder[i].gasmix)/10.0) - (get_he(&dive->cylinder[i].gasmix)/10.0)));
				delta_p.mbar += dive->cylinder[i].start.mbar - dive->cylinder[i].end.mbar;
				put_format(&buf, "\\def\\cyl%dstartpress{%.1f \\pressureunit}\n", i + 1, get_pressure_units(dive->cylinder[i].start.mbar, &unit)/1.0);
				put_format(&buf, "\\def\\cyl%dendpress{%.1f \\pressureunit}\n", i + 1, get_pressure_units(dive->cylinder[i].end.mbar, &unit)/1.0);
				qty_cyl += 1;
			}
		}
		put_format(&buf, "\\def\\qtycyl{%d}\n", qty_cyl);
		put_format(&buf, "\\def\\gasuse{%.1f \\pressureunit}\n", get_pressure_units(delta_p.mbar, &unit)/1.0);
		put_format(&buf, "\\def\\sac{%.2f \\volumeunit/min}\n", get_volume_units(dive->sac, NULL, &unit));

		//Code block prints all weights listed in dive.
		put_format(&buf, "\n%% Weighting information:\n");
		qty_weight = 0;
		total_weight = 0;
		for (i = 0; i < MAX_WEIGHTSYSTEMS; i++){
			if (dive->weightsystem[i].weight.grams != NULL){
				put_format(&buf, "\\def\\weight%dtype{%s}\n", i + 1, dive->weightsystem[i].description);
				put_format(&buf, "\\def\\weight%damt{%.3f \\weightunit}\n", i + 1, get_weight_units(dive->weightsystem[i].weight.grams, NULL, &unit));
				qty_weight += 1;
				total_weight += get_weight_units(dive->weightsystem[i].weight.grams, NULL, &unit);
			}
		}
		put_format(&buf, "\\def\\qtyweights{%d}\n", qty_weight);
		put_format(&buf, "\\def\\totalweight{%.2f \\weightunit}\n", total_weight);
		unit = "";

		// Deprecated fields
		put_format(&buf, "\n%% Deprecated Fields - kept to maintain compatibility with legacy templates:\n");
		put_format(&buf, "%% The following fields are deprecated and may not return any value!\n");
		put_format(&buf, "%% They have been let here to preserve functionality in legacy templates.");
		put_format(&buf, "%% Do not use these fields in future templates.\n");
		put_format(&buf, "\\def\\spot{} %% Deprecated - does not return a value\n"); // Deprecated - remains for use in legacy templates
		put_format(&buf, "\\def\\entrance{} %% Deprecated - does not return a value\n"); // Deprecated - remains for use in legacy templates
		put_format(&buf, "\\def\\place{%s} %% Deprecated - use \\sitename instead\n", site ? site->name : ""); // Deprecated - remains for use in legacy templates
		get_depth_units(dive->maxdepth.mm, NULL, &unit) > 0 ? put_format(&buf, "\\def\\depth{%.1f \\depthunit}%% Deprecated - use \\maximumdepth instead\n", get_depth_units(dive->maxdepth.mm, NULL, &unit)) : put_format(&buf, "\\def\\depth{} %% Deprecated - use \\maximumdepth instead\n");  // Deprecated - remains for use in legacy templates

		put_format(&buf, "\\page\n");
	}

	put_format(&buf, "\\bye\n");

	f = subsurface_fopen(filename, "w+");
	if (!f) {
		report_error(tr("Can't open file %s").toUtf8().data(), filename);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
	free_buffer(&buf);
}
