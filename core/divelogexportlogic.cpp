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

	stats_t total_stats;

	stats_summary stats = calculate_stats_summary(hes.selectedOnly);
	total_stats.selection_size = 0;
	total_stats.total_time = 0_sec;

	out << "divestat=[";
	if (hes.yearlyStatistics) {
		for (const auto &s: stats.stats_yearly) {
			out << "{";
			out << "\"YEAR\":\"" << s.period << "\",";
			out << "\"DIVES\":\"" << s.selection_size << "\",";
			out << "\"TOTAL_TIME\":\"" << get_dive_duration_string(s.total_time.seconds,
											gettextFromC::tr("h"), gettextFromC::tr("min"), gettextFromC::tr("sec"), " ") << "\",";
			out << "\"AVERAGE_TIME\":\"" << formatMinutes(s.total_time.seconds / s.selection_size) << "\",";
			out << "\"SHORTEST_TIME\":\"" << formatMinutes(s.shortest_time.seconds) << "\",";
			out << "\"LONGEST_TIME\":\"" << formatMinutes(s.longest_time.seconds) << "\",";
			out << "\"AVG_DEPTH\":\"" << get_depth_string(s.avg_depth) << "\",";
			out << "\"MIN_DEPTH\":\"" << get_depth_string(s.min_depth) << "\",";
			out << "\"MAX_DEPTH\":\"" << get_depth_string(s.max_depth) << "\",";
			out << "\"AVG_SAC\":\"" << get_volume_string(s.avg_sac) << "\",";
			out << "\"MIN_SAC\":\"" << get_volume_string(s.min_sac) << "\",";
			out << "\"MAX_SAC\":\"" << get_volume_string(s.max_sac) << "\",";
			if (s.combined_count) {
				temperature_t avg_temp;
				avg_temp.mkelvin = s.combined_temp.mkelvin / s.combined_count;
				out << "\"AVG_TEMP\":\"" << get_temperature_string(avg_temp) << "\",";
			} else {
				out << "\"AVG_TEMP\":\"0.0\",";
			}
			out << "\"MIN_TEMP\":\"" << (s.min_temp.mkelvin == 0 ? 0 : get_temperature_string(s.min_temp)) << "\",";
			out << "\"MAX_TEMP\":\"" << (s.max_temp.mkelvin == 0 ? 0 : get_temperature_string(s.max_temp)) << "\",";
			out << "},";
			total_stats.selection_size += s.selection_size;
			total_stats.total_time += s.total_time;
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

	QString js_dive_data = exportFiles + "file.js";
	QString json_dive_data = exportFiles + "trips.json";
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

	export_JS(qPrintable(js_dive_data), qPrintable(photosDirectory), hes.selectedOnly, hes.listOnly);
	export_JSON(qPrintable(json_dive_data), hes.selectedOnly, hes.listOnly);

	QString searchPath = getSubsurfaceDataPath("theme");
	if (searchPath.isEmpty()) {
		report_error("%s", qPrintable(gettextFromC::tr("Cannot find a folder called 'theme' in the standard locations")));
		return;
	}

	searchPath += QDir::separator();

	file_copy_and_overwrite(searchPath + "dive_export.html", filename);
	file_copy_and_overwrite(searchPath + "list_lib.js", exportFiles + "list_lib.js");
	file_copy_and_overwrite(searchPath + "poster.png", exportFiles + "poster.png");
	file_copy_and_overwrite(searchPath + "jquery.jqplot.min.js", exportFiles + "jquery.jqplot.min.js");
	file_copy_and_overwrite(searchPath + "jqplot.canvasAxisTickRenderer.js", exportFiles + "jqplot.canvasAxisTickRenderer.js");
	file_copy_and_overwrite(searchPath + "jqplot.canvasTextRenderer.js", exportFiles + "jqplot.canvasTextRenderer.js");
	file_copy_and_overwrite(searchPath + "jqplot.highlighter.js", exportFiles + "jqplot.highlighter.js");
	file_copy_and_overwrite(searchPath + "jquery.min.js", exportFiles + "jquery.min.js");
	file_copy_and_overwrite(searchPath + "jquery.jqplot.min.css", exportFiles + "jquery.jqplot.min.css");
	file_copy_and_overwrite(searchPath + hes.themeFile, exportFiles + "theme.css");
}
