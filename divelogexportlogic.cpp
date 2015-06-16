#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QTextStream>
#include "divelogexportlogic.h"
#include "helpers.h"
#include "units.h"
#include "statistics.h"
#include "save-html.h"

void file_copy_and_overwrite(const QString &fileName, const QString &newName)
{
	QFile file(newName);
	if (file.exists())
		file.remove();
	QFile::copy(fileName, newName);
}

void exportHTMLsettings(const QString &filename, struct htmlExportSetting &hes)
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
		QVariant v;
		QString length, pressure, volume, temperature, weight;
		length = prefs.units.length == units::METERS ? "METER" : "FEET";
		pressure = prefs.units.pressure == units::BAR ? "BAR" : "PSI";
		volume = prefs.units.volume == units::LITER ? "LITER" : "CUFT";
		temperature = prefs.units.temperature == units::CELSIUS ? "CELSIUS" : "FAHRENHEIT";
		weight = prefs.units.weight == units::KG ? "KG" : "LBS";
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


static void exportHTMLstatistics(const QString filename, struct htmlExportSetting &hes)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);

	stats_t total_stats;

	total_stats.selection_size = 0;
	total_stats.total_time.seconds = 0;

	int i = 0;
	out << "divestat=[";
	if (hes.yearlyStatistics) {
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
			if ( stats_yearly[i].combined_count )
				out << "\"AVG_TEMP\":\"" << QString::number(stats_yearly[i].combined_temp / stats_yearly[i].combined_count, 'f', 1) << "\",";
			else
				out << "\"AVG_TEMP\":\"0.0\",";
			out << "\"MIN_TEMP\":\"" << ( stats_yearly[i].min_temp == 0 ? 0 : get_temp_units(stats_yearly[i].min_temp, NULL)) << "\",";
			out << "\"MAX_TEMP\":\"" << ( stats_yearly[i].max_temp == 0 ? 0 : get_temp_units(stats_yearly[i].max_temp, NULL)) << "\",";
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

void exportHtmlInitLogic(const QString &filename, struct htmlExportSetting &hes)
{
	QString photosDirectory;
	QFile file(filename);
	QFileInfo info(file);
	QDir mainDir = info.absoluteDir();
	mainDir.mkdir(file.fileName() + "_files");
	QString exportFiles = file.fileName() + "_files";

	QString json_dive_data = exportFiles + QDir::separator() + "file.js";
	QString json_settings = exportFiles + QDir::separator() + "settings.js";
	QString translation = exportFiles + QDir::separator() + "translation.js";
	QString stat_file = exportFiles + QDir::separator() + "stat.js";
	exportFiles += "/";

	if (hes.exportPhotos) {
		photosDirectory = exportFiles + QDir::separator() + "photos" + QDir::separator();
		mainDir.mkdir(photosDirectory);
	}


	exportHTMLsettings(json_settings, hes);
	exportHTMLstatistics(stat_file, hes);
	export_translation(translation.toUtf8().data());

	export_HTML(qPrintable(json_dive_data), qPrintable(photosDirectory), hes.selectedOnly, hes.listOnly);

	QString searchPath = getSubsurfaceDataPath("theme");
	if (searchPath.isEmpty())
		return;

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
