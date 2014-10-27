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
#include "diveshareexportdialog.h"
#include "ui_divelogexportdialog.h"
#include "subsurfacewebservices.h"
#include "worldmap-save.h"
#include "save-html.h"
#include "helpers.h"
#include "statistics.h"

#define GET_UNIT(name, field, f, t)                   \
	v = settings.value(QString(name));            \
	if (v.isValid())                              \
		field = (v.toInt() == 0) ? (t) : (f); \
	else                                          \
		field = default_prefs.units.field

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
	if (settings.contains("subsurfaceNumbers")) {
		ui->exportSubsurfaceNumber->setChecked(settings.value("subsurfaceNumbers").toBool());
	}
	if (settings.contains("yearlyStatistics")) {
		ui->exportStatistics->setChecked(settings.value("yearlyStatistics").toBool());
	}
	if (settings.contains("listOnly")) {
		ui->exportListOnly->setChecked(settings.value("listOnly").toBool());
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
		ui->description->setText(tr("Comma separated values that include the most relevant information of the dive profile."));
	} else if (ui->exportDivelogs->isChecked()) {
		ui->description->setText(tr("Send the dive data to divelogs.de website."));
	} else if (ui->exportDiveshare->isChecked()) {
		ui->description->setText(tr("Send the dive data to dive-share.appspot.com website"));
	} else if (ui->exportWorldMap->isChecked()) {
		ui->description->setText(tr("HTML export of the dive locations, visualized on a world map."));
	} else if (ui->exportSubsurfaceXML->isChecked()) {
		ui->description->setText(tr("Subsurface native XML format."));
	} else if (ui->exportImageDepths->isChecked()) {
		ui->description->setText(tr("Write depths of images to file."));
	}
}

void DiveLogExportDialog::copy_and_overwrite(const QString &fileName, const QString &newName)
{
	QFile file(newName);
	if (file.exists())
		file.remove();
	QFile::copy(fileName, newName);
}

void DiveLogExportDialog::exportHtmlInit(const QString &filename)
{
	QFile file(filename);
	QFileInfo info(file);
	QDir mainDir = info.absoluteDir();
	mainDir.mkdir(file.fileName() + "_files");
	QString exportFiles = file.fileName() + "_files";

	QString json_dive_data = exportFiles + QDir::separator() + "file.js";
	QString json_settings = exportFiles + QDir::separator() + "settings.js";
	QString translation = exportFiles + QDir::separator() + "translation.js";
	QString stat_file = exportFiles + QDir::separator() + "stat.js";
	QString photos_directory = exportFiles + QDir::separator() + "photos" + QDir::separator();
	mainDir.mkdir(photos_directory);
	exportFiles += "/";

	exportHTMLsettings(json_settings);
	exportHTMLstatistics(stat_file);
	export_translation(translation.toUtf8().data());

	export_HTML(json_dive_data.toUtf8().data(), photos_directory.toUtf8().data(), ui->exportSelectedDives->isChecked(), ui->exportListOnly->isChecked());

	QString searchPath = getSubsurfaceDataPath("theme");
	if (searchPath.isEmpty())
		return;

	searchPath += QDir::separator();

	copy_and_overwrite(searchPath + "dive_export.html", filename);
	copy_and_overwrite(searchPath + "list_lib.js", exportFiles + "list_lib.js");
	copy_and_overwrite(searchPath + "poster.png", exportFiles + "poster.png");
	copy_and_overwrite(searchPath + "jqplot.highlighter.min.js", exportFiles + "jqplot.highlighter.min.js");
	copy_and_overwrite(searchPath + "jquery.jqplot.min.js", exportFiles + "jquery.jqplot.min.js");
	copy_and_overwrite(searchPath + "jqplot.canvasAxisTickRenderer.min.js", exportFiles + "jqplot.canvasAxisTickRenderer.min.js");
	copy_and_overwrite(searchPath + "jqplot.canvasTextRenderer.min.js", exportFiles + "jqplot.canvasTextRenderer.min.js");
	copy_and_overwrite(searchPath + "jquery.min.js", exportFiles + "jquery.min.js");
	copy_and_overwrite(searchPath + "jquery.jqplot.css", exportFiles + "jquery.jqplot.css");
	copy_and_overwrite(searchPath + (ui->themeSelection->currentText() == "Light" ? "light.css" : "sand.css"),
			   exportFiles + "theme.css");
}

void DiveLogExportDialog::exportHTMLsettings(const QString &filename)
{
	QSettings settings;
	settings.beginGroup("HTML");
	settings.setValue("fontSelection", ui->fontSelection->currentIndex());
	settings.setValue("fontSizeSelection", ui->fontSizeSelection->currentIndex());
	settings.setValue("themeSelection", ui->themeSelection->currentIndex());
	settings.setValue("subsurfaceNumbers", ui->exportSubsurfaceNumber->isChecked());
	settings.setValue("yearlyStatistics", ui->exportStatistics->isChecked());
	settings.setValue("listOnly", ui->exportListOnly->isChecked());
	settings.endGroup();

	QString fontSize = ui->fontSizeSelection->currentText();
	QString fontFamily = ui->fontSelection->itemData(ui->fontSelection->currentIndex()).toString();
	QFile file(filename);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);
	out << "settings = {\"fontSize\":\"" << fontSize << "\",\"fontFamily\":\"" << fontFamily << "\",\"listOnly\":\""
	    << ui->exportListOnly->isChecked() << "\",\"subsurfaceNumbers\":\"" << ui->exportSubsurfaceNumber->isChecked() << "\",";
	//save units preferences
	settings.beginGroup("Units");
	if (settings.value("unit_system").toString() == "metric") {
		out << "\"unit_system\":\"Meteric\"";
	} else if (settings.value("unit_system").toString() == "imperial") {
		out << "\"unit_system\":\"Imperial\"";
	} else {
		QVariant v;
		QString length, pressure, volume, temperature, weight;
		GET_UNIT("length", length, "FEET", "METER");
		GET_UNIT("pressure", pressure, "PSI", "BAR");
		GET_UNIT("volume", volume, "CUFT", "LITER");
		GET_UNIT("temperature", temperature, "FAHRENHEIT", "CELSIUS");
		GET_UNIT("weight", weight, "LBS", "KG");
		out << "\"unit_system\":\"Personalize\",";
		out << "\"units\":{\"depth\":\"" << length << "\",\"pressure\":\"" << pressure << "\",\"volume\":\"" << volume << "\",\"temperature\":\"" << temperature << "\",\"weight\":\"" << weight << "\"}";
	}
	out << "}";
	settings.endGroup();
	file.close();
}

void DiveLogExportDialog::exportHTMLstatistics(const QString &filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);

	stats_t total_stats;

	total_stats.selection_size = 0;
	total_stats.total_time.seconds = 0;

	int i = 0;
	out << "divestat=[";
	if (ui->exportStatistics->isChecked()) {
		while (stats_yearly != NULL && stats_yearly[i].period) {
			out << "{";
			out << "\"YEAR\":\"" << stats_yearly[i].period << "\",";
			out << "\"DIVES\":\"" << stats_yearly[i].selection_size << "\",";
			out << "\"TOTAL_TIME\":\"" << get_time_string(stats_yearly[i].total_time.seconds, 0) << "\",";
			out << "\"AVERAGE_TIME\":\"" << get_minutes(stats_yearly[i].total_time.seconds / stats_yearly[i].selection_size) << "\",";
			out << "\"SHORTEST_TIME\":\"" << get_minutes(stats_yearly[i].shortest_time.seconds) << "\",";
			out << "\"LONGEST_TIME\":\"" << get_minutes(stats_yearly[i].longest_time.seconds) << "\",";
			out << "\"AVG_DEPTH\":\"" << get_depth_string(stats_yearly[i].avg_depth) << "\",";
			out << "\"MIN_DEPTH\":\"" << get_depth_string(stats_yearly[i].min_depth) << "\",";
			out << "\"MAX_DEPTH\":\"" << get_depth_string(stats_yearly[i].max_depth) << "\",";
			out << "\"AVG_SAC\":\"" << get_volume_string(stats_yearly[i].avg_sac) << "\",";
			out << "\"MIN_SAC\":\"" << get_volume_string(stats_yearly[i].min_sac) << "\",";
			out << "\"MAX_SAC\":\"" << get_volume_string(stats_yearly[i].max_sac) << "\",";
			out << "\"AVG_TEMP\":\"" << QString::number(stats_yearly[i].combined_temp / stats_yearly[i].combined_count, 'f', 1) << "\",";
			out << "\"MIN_TEMP\":\"" << get_temp_units(stats_yearly[i].min_temp, NULL) << "\",";
			out << "\"MAX_TEMP\":\"" << get_temp_units(stats_yearly[i].max_temp, NULL) << "\",";
			out << "},";
			total_stats.selection_size += stats_yearly[i].selection_size;
			total_stats.total_time.seconds += stats_yearly[i].total_time.seconds;
			i++;
		}
		exportHTMLstatisticsTotal(out, &total_stats);
	}
	out << "]";
	file.close();
}

void exportHTMLstatisticsTotal(QTextStream &out, stats_t *total_stats)
{
	out << "{";
	out << "\"YEAR\":\"Total\",";
	out << "\"DIVES\":\"" << total_stats->selection_size << "\",";
	out << "\"TOTAL_TIME\":\"" << get_time_string(total_stats->total_time.seconds, 0) << "\",";
	out << "\"AVERAGE_TIME\":\"--\",";
	out << "\"SHORTEST_TIME\":\"--\",";
	out << "\"LONGEST_TIME\":\"--\",";
	out << "\"AVG_DEPTH\":\"--\",";
	out << "\"MIN_DEPTH\":\"--\",";
	out << "\"MAX_DEPTH\":\"--\",";
	out << "\"AVG_SAC\":\"--\",";
	out << "\"MIN_SAC\":\"--\",";
	out << "\"MAX_SAC\":\"--\",";
	out << "\"AVG_TEMP\":\"--\",";
	out << "\"MIN_TEMP\":\"--\",";
	out << "\"MAX_TEMP\":\"--\",";
	out << "},";
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
			filename = QFileDialog::getSaveFileName(this, tr("Export UDDF file as"), lastDir,
								tr("UDDF files (*.uddf *.UDDF)"));
		} else if (ui->exportCSV->isChecked()) {
			stylesheet = "xml2csv.xslt";
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

void DiveLogExportDialog::export_depths(const char *filename, const bool selected_only)
{
	FILE *f;
	struct dive *dive;
	depth_t depth;
	int i;
	const char **unit = NULL;

	struct membuffer buf = { 0 };

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
			put_format(&buf, "%s\t%.1f%s\n", picture->filename, get_depth_units(depth.mm, NULL, unit), *unit);
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
