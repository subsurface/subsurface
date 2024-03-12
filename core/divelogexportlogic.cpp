// SPDX-License-Identifier: GPL-2.0
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include "divelogexportlogic.h"
#include "errorhelper.h"
#include "qthelper.h"
#include "units.h"
#include "statistics.h"
#include "string-format.h"
#include "save-html.h"

static void file_copy_and_overwrite(const QString &fileName, const QString &newName)
{
	QFile file(newName);
	if (file.exists())
		file.remove();
	QFile::copy(fileName, newName);
}

static void exportHTMLsettings(const QString &filename, struct htmlExportSetting &hes)
{
	QString fontSize = hes.fontSize;
	QString fontFamily = hes.fontFamily;
	QFile file(filename);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);
	out << "settings = {\"fontSize\":\"" << fontSize << "\",\"fontFamily\":\"" << fontFamily << "\",\"listOnly\":\""
	    << hes.listOnly << "\",\"subsurfaceNumbers\":\"" << hes.subsurfaceNumbers << "\",";
	//save units preferences
	if (prefs.unit_system == METRIC) {
		out << "\"unit_system\":\"Meteric\"";
	} else if (prefs.unit_system == IMPERIAL) {
		out << "\"unit_system\":\"Imperial\"";
	} else {
		QString length = prefs.units.length == units::METERS ? "METER" : "FEET";
		QString pressure = prefs.units.pressure == units::BAR ? "BAR" : "PSI";
		QString volume = prefs.units.volume == units::LITER ? "LITER" : "CUFT";
		QString temperature = prefs.units.temperature == units::CELSIUS ? "CELSIUS" : "FAHRENHEIT";
		QString weight = prefs.units.weight == units::KG ? "KG" : "LBS";
		out << "\"unit_system\":\"Personalize\",";
		out << "\"units\":{\"depth\":\"" << length << "\",\"pressure\":\"" << pressure << "\",\"volume\":\"" << volume << "\",\"temperature\":\"" << temperature << "\",\"weight\":\"" << weight << "\"}";
	}
	out << "}";
	file.close();
}

static void exportHTMLstatisticsTotal(QTextStream &out, stats_t *total_stats)
{
	out << "{";
	out << "\"YEAR\":\"Total\",";
	out << "\"DIVES\":\"" << total_stats->selection_size << "\",";
	out << "\"TOTAL_TIME\":\"" << get_dive_duration_string(total_stats->total_time.seconds,
									gettextFromC::tr("h"), gettextFromC::tr("min"), gettextFromC::tr("sec"), " ") << "\",";
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


static void exportHTMLstatistics(const QString filename, struct htmlExportSetting &hes)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);
	stats_summary_auto_free stats;

	stats_t total_stats;

	calculate_stats_summary(&stats, hes.selectedOnly);
	total_stats.selection_size = 0;
	total_stats.total_time.seconds = 0;

	int i = 0;
	out << "divestat=[";
	if (hes.yearlyStatistics) {
		while (stats.stats_yearly != NULL && stats.stats_yearly[i].period) {
			out << "{";
			out << "\"YEAR\":\"" << stats.stats_yearly[i].period << "\",";
			out << "\"DIVES\":\"" << stats.stats_yearly[i].selection_size << "\",";
			out << "\"TOTAL_TIME\":\"" << get_dive_duration_string(stats.stats_yearly[i].total_time.seconds,
											gettextFromC::tr("h"), gettextFromC::tr("min"), gettextFromC::tr("sec"), " ") << "\",";
			out << "\"AVERAGE_TIME\":\"" << formatMinutes(stats.stats_yearly[i].total_time.seconds / stats.stats_yearly[i].selection_size) << "\",";
			out << "\"SHORTEST_TIME\":\"" << formatMinutes(stats.stats_yearly[i].shortest_time.seconds) << "\",";
			out << "\"LONGEST_TIME\":\"" << formatMinutes(stats.stats_yearly[i].longest_time.seconds) << "\",";
			out << "\"AVG_DEPTH\":\"" << get_depth_string(stats.stats_yearly[i].avg_depth) << "\",";
			out << "\"MIN_DEPTH\":\"" << get_depth_string(stats.stats_yearly[i].min_depth) << "\",";
			out << "\"MAX_DEPTH\":\"" << get_depth_string(stats.stats_yearly[i].max_depth) << "\",";
			out << "\"AVG_SAC\":\"" << get_volume_string(stats.stats_yearly[i].avg_sac) << "\",";
			out << "\"MIN_SAC\":\"" << get_volume_string(stats.stats_yearly[i].min_sac) << "\",";
			out << "\"MAX_SAC\":\"" << get_volume_string(stats.stats_yearly[i].max_sac) << "\",";
			if (stats.stats_yearly[i].combined_count) {
				temperature_t avg_temp;
				avg_temp.mkelvin = stats.stats_yearly[i].combined_temp.mkelvin / stats.stats_yearly[i].combined_count;
				out << "\"AVG_TEMP\":\"" << get_temperature_string(avg_temp) << "\",";
			} else {
				out << "\"AVG_TEMP\":\"0.0\",";
			}
			out << "\"MIN_TEMP\":\"" << (stats.stats_yearly[i].min_temp.mkelvin == 0 ? 0 : get_temperature_string(stats.stats_yearly[i].min_temp)) << "\",";
			out << "\"MAX_TEMP\":\"" << (stats.stats_yearly[i].max_temp.mkelvin == 0 ? 0 : get_temperature_string(stats.stats_yearly[i].max_temp)) << "\",";
			out << "},";
			total_stats.selection_size += stats.stats_yearly[i].selection_size;
			total_stats.total_time.seconds += stats.stats_yearly[i].total_time.seconds;
			i++;
		}
		exportHTMLstatisticsTotal(out, &total_stats);
	}
	out << "]";
	file.close();

}

void exportHtmlInitLogic(const QString &filename, struct htmlExportSetting &hes)
{
	QString photosDirectory;
	QFile file(filename);
	QFileInfo info(file);
	QDir mainDir = info.absoluteDir();
	QString exportFiles = file.fileName() + "_files" + QDir::separator();
	mainDir.mkdir(exportFiles);

	QString json_dive_data = exportFiles + "file.js";
	QString json_settings = exportFiles + "settings.js";
	QString translation = exportFiles + "translation.js";
	QString stat_file = exportFiles + "stat.js";

	if (hes.exportPhotos) {
		photosDirectory = exportFiles + "photos" + QDir::separator();
		mainDir.mkdir(photosDirectory);
	}

	exportHTMLsettings(json_settings, hes);
	exportHTMLstatistics(stat_file, hes);
	export_translation(qPrintable(translation));

	export_HTML(qPrintable(json_dive_data), qPrintable(photosDirectory), hes.selectedOnly, hes.listOnly);

	QString searchPath = getSubsurfaceDataPath("theme");
	if (searchPath.isEmpty()) {
		report_error("%s", qPrintable(gettextFromC::tr("Cannot find a folder called 'theme' in the standard locations")));
		return;
	}

	searchPath += QDir::separator();

	file_copy_and_overwrite(searchPath + "dive_export.html", filename);
	file_copy_and_overwrite(searchPath + "list_lib.js", exportFiles + "list_lib.js");
	file_copy_and_overwrite(searchPath + "poster.png", exportFiles + "poster.png");
	file_copy_and_overwrite(searchPath + "jqplot.highlighter.min.js", exportFiles + "jqplot.highlighter.min.js");
	file_copy_and_overwrite(searchPath + "jquery.jqplot.min.js", exportFiles + "jquery.jqplot.min.js");
	file_copy_and_overwrite(searchPath + "jqplot.canvasAxisTickRenderer.min.js", exportFiles + "jqplot.canvasAxisTickRenderer.min.js");
	file_copy_and_overwrite(searchPath + "jqplot.canvasTextRenderer.min.js", exportFiles + "jqplot.canvasTextRenderer.min.js");
	file_copy_and_overwrite(searchPath + "jquery.min.js", exportFiles + "jquery.min.js");
	file_copy_and_overwrite(searchPath + "jquery.jqplot.css", exportFiles + "jquery.jqplot.css");
	file_copy_and_overwrite(searchPath + hes.themeFile, exportFiles + "theme.css");
}
