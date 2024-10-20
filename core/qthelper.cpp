// SPDX-License-Identifier: GPL-2.0
#include "qthelper.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "core/settings/qPrefLanguage.h"
#include "core/settings/qPrefUpdateManager.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "subsurface-string.h"
#include "gettextfromc.h"
#include "statistics.h"
#include "version.h"
#include "errorhelper.h"
#include "planner.h"
#include "range.h"
#include "subsurface-time.h"
#include "gettextfromc.h"
#include "metadata.h"
#include "exif.h"
#include "file.h"
#include "picture.h"
#include "selection.h"
#include "tag.h"
#include "imagedownloader.h"
#include "xmlparams.h"
#include <QFile>
#include <QRegularExpression>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QNetworkProxy>
#include <QDateTime>
#include <QImageReader>
#include <QtConcurrent>
#include <QFont>
#include <QApplication>
#include <QTextDocument>
#include <cstdarg>
#include <cstdint>
#ifdef Q_OS_UNIX
#include <sys/utsname.h>
#endif

#include <libxslt/documents.h>

std::string existing_filename;
static QLocale loc;

static inline QString degreeSigns()
{
	return QStringLiteral("dD\u00b0");
}

static QString weight_string(weight_t weight)
{
	QString str;
	if (get_units()->weight == units::KG) {
		double kg = (double) weight.grams / 1000.0;
		str = QString("%L1").arg(kg, 0, 'f', kg >= 20.0 ? 0 : 1);
	} else {
		double lbs = grams_to_lbs(weight.grams);
		str = QString("%L1").arg(lbs, 0, 'f', lbs >= 40.0 ? 0 : 1);
	}
	return str;
}

QString distance_string(int distanceInMeters)
{
	QString str;
	if(get_units()->length == units::METERS) {
		if (distanceInMeters >= 1000)
			str = gettextFromC::tr("%1km").arg(distanceInMeters / 1000);
		else
			str = gettextFromC::tr("%1m").arg(distanceInMeters);
	} else {
		double miles = m_to_mile(distanceInMeters);
		if (miles >= 1.0)
			str = gettextFromC::tr("%1mi").arg((int)miles);
		else
			str = gettextFromC::tr("%1yd").arg((int)(miles * 1760));
	}
	return str;
}

QString printGPSCoords(const location_t *location)
{
	int lat = location->lat.udeg;
	int lon = location->lon.udeg;
	unsigned int latdeg, londeg;
	unsigned int latmin, lonmin;
	double latsec, lonsec;
	QString lath, lonh, result;

	if (!has_location(location))
		return QString();

	if (prefs.coordinates_traditional) {
		lath = lat >= 0 ? gettextFromC::tr("N") : gettextFromC::tr("S");
		lonh = lon >= 0 ? gettextFromC::tr("E") : gettextFromC::tr("W");
		lat = abs(lat);
		lon = abs(lon);
		latdeg = lat / 1000000U;
		londeg = lon / 1000000U;
		latmin = (lat % 1000000U) * 60U;
		lonmin = (lon % 1000000U) * 60U;
		latsec = (latmin % 1000000) * 60;
		lonsec = (lonmin % 1000000) * 60;
		result = QString::asprintf("%u°%02d\'%06.3f\"%s %u°%02d\'%06.3f\"%s",
			       latdeg, latmin / 1000000, latsec / 1000000, qPrintable(lath),
			       londeg, lonmin / 1000000, lonsec / 1000000, qPrintable(lonh));
	} else {
		result = QString::asprintf("%f %f", (double) lat / 1000000.0, (double) lon / 1000000.0);
	}
	return result;
}

std::string printGPSCoordsC(const location_t *location)
{
	return printGPSCoords(location).toStdString();
}

/**
* Try to parse in a generic manner a coordinate.
*/
static bool parseCoord(const QString &txt, int &pos, const QString &positives,
		       const QString &negatives, const QString &others,
		       double &value)
{
	static const QString separators = QString(",;");
	bool numberDefined = false, degreesDefined = false,
		minutesDefined = false, secondsDefined = false;
	double number = 0.0;
	int posBeforeNumber = pos;
	int sign = 0;
	value = 0.0;
	while (pos < txt.size()) {
		if (txt[pos].isDigit()) {
			if (numberDefined)
				return false;
			QRegularExpression numberRe("\\A(\\d+(?:[\\.,]\\d+)?).*");
			QRegularExpressionMatch match = numberRe.match(txt.mid(pos));
			if (!match.hasMatch())
				return false;
			number = match.captured(1).toDouble();
			numberDefined = true;
			posBeforeNumber = pos;
			pos += match.captured(1).size() - 1;
		} else if (positives.indexOf(txt[pos]) >= 0) {
			if (sign != 0)
				return false;
			sign = 1;
			if (degreesDefined || numberDefined) {
				//sign after the degrees =>
				//at the end of the coordinate
				++pos;
				break;
			}
		} else if (negatives.indexOf(txt[pos]) >= 0) {
			if (sign != 0) {
				if (others.indexOf(txt[pos]) >= 0)
					//special case for the '-' sign => next coordinate
					break;
				return false;
			}
			sign = -1;
			if (degreesDefined || numberDefined) {
				//sign after the degrees =>
				//at the end of the coordinate
				++pos;
				break;
			}
		} else if (others.indexOf(txt[pos]) >= 0) {
			//we are at the next coordinate.
			break;
		} else if (degreeSigns().indexOf(txt[pos]) >= 0 ||
			   (txt[pos].isSpace() && !degreesDefined && numberDefined)) {
			if (!numberDefined)
				return false;
			if (degreesDefined) {
				//next coordinate => need to put back the number
				pos = posBeforeNumber;
				numberDefined = false;
				break;
			}
			value += number;
			numberDefined = false;
			degreesDefined = true;
		} else if (txt[pos] == '\'' || (txt[pos].isSpace() && !minutesDefined && numberDefined)) {
			if (!numberDefined || minutesDefined)
				return false;
			value += number / 60.0;
			numberDefined = false;
			minutesDefined = true;
		} else if (txt[pos] == '"' || (txt[pos].isSpace() && !secondsDefined && numberDefined)) {
			if (!numberDefined || secondsDefined)
				return false;
			value += number / 3600.0;
			numberDefined = false;
			secondsDefined = true;
		} else if ((numberDefined || minutesDefined || secondsDefined) &&
			   separators.indexOf(txt[pos]) >= 0) {
			// separator; this coordinate is finished
			break;
		} else {
			return false;
		}
		++pos;
	}
	if (!degreesDefined && numberDefined)
		value = number; //just a single number => degrees
	else if (!minutesDefined && numberDefined)
		value += number / 60.0;
	else if (!secondsDefined && numberDefined)
		value += number / 3600.0;
	else if (numberDefined)
		return false;

	// We parsed a valid coordinate; eat any subsequent separators and/or
	// whitespace
	while (pos < txt.size() && (txt[pos].isSpace() || separators.indexOf(txt[pos]) >= 0))
		pos++;

	if (sign == -1) value *= -1.0;
	return true;
}

/**
* Parse special coordinate formats that cannot be handled by parseCoord.
*/
static bool parseSpecialCoords(const QString &txt, double &latitude, double &longitude) {
	QRegularExpression xmlFormat("\\A(-?\\d+(?:\\.\\d+)?),?\\s+(-?\\d+(?:\\.\\d+)?)\\z");
	QRegularExpressionMatch match = xmlFormat.match(txt);
	if (match.hasMatch()) {
		latitude = match.captured(1).toDouble();
		longitude = match.captured(2).toDouble();
		return true;
	}
	return false;
}

bool parseGpsText(const QString &gps_text, double *latitude, double *longitude)
{
	static const QString POS_LAT = QString("+N") + gettextFromC::tr("N");
	static const QString NEG_LAT = QString("-S") + gettextFromC::tr("S");
	static const QString POS_LON = QString("+E") + gettextFromC::tr("E");
	static const QString NEG_LON = QString("-W") + gettextFromC::tr("W");

	// Remove the useless spaces (but keep the ones separating numbers)
	// and normalize different ways of writing separators.
	static const QRegularExpression spaceCleaner("\\s*([" + POS_LAT + NEG_LAT + POS_LON +
		NEG_LON + degreeSigns() + "'\"\\s])\\s*");
	const QString normalized = gps_text.trimmed().toUpper().
		replace(spaceCleaner, "\\1").
		replace(QStringLiteral("′"), "'").
		replace(QStringLiteral("’"), "'").
		replace(QStringLiteral("''"), "\"").
		replace(QStringLiteral("″"), "\"");

	if (normalized.isEmpty()) {
		*latitude = 0.0;
		*longitude = 0.0;
		return true;
	}
	if (parseSpecialCoords(normalized, *latitude, *longitude))
		return true;
	int pos = 0;
	return parseCoord(normalized, pos, POS_LAT, NEG_LAT, POS_LON + NEG_LON, *latitude) &&
		parseCoord(normalized, pos, POS_LON, NEG_LON, "", *longitude) &&
		pos == normalized.size();
}

static xmlDocPtr get_stylesheet_doc(const xmlChar *uri, xmlDictPtr, int, void *, xsltLoadType)
{
	std::string filename = std::string(":/xslt/") + (const char *)uri;
	QFile f(filename.c_str());
	if (!f.open(QIODevice::ReadOnly)) {
		if (verbose > 0) {
			report_info("cannot open stylesheet %s %s", filename.c_str(), qPrintable(f.errorString()));
			return NULL;
		}
	}
	/* Load and parse the data */
	QByteArray source = f.readAll();

	xmlDocPtr doc = xmlParseMemory(source, source.size());
	return doc;
}

xsltStylesheetPtr get_stylesheet(const char *name)
{
	// this needs to be done only once, but doesn't hurt to run every time
	xsltSetLoaderFunc(get_stylesheet_doc);

	// get main document:
	xmlDocPtr doc = get_stylesheet_doc((const xmlChar *)name, NULL, 0, NULL, XSLT_LOAD_START);
	if (!doc)
		return NULL;

	//	xsltSetGenericErrorFunc(stderr, NULL);
	xsltStylesheetPtr xslt = xsltParseStylesheetDoc(doc);
	if (!xslt) {
		xmlFreeDoc(doc);
		return NULL;
	}

	return xslt;
}

std::string move_away(const std::string &old_path)
{
	if (verbose > 1)
		report_info("move away %s", old_path.c_str());
	QDir oldDir(old_path.c_str());
	QDir newDir;
	std::string newPath;
	int i = 0;
	do {
		newPath = old_path + "." + std::to_string(++i);
		newDir.setPath(newPath.c_str());
	} while(newDir.exists());
	if (verbose > 1)
		report_info("renaming toi %s", newPath.c_str());
	if (!oldDir.rename(old_path.c_str(), newPath.c_str())) {
		if (verbose)
			report_info("rename of %s to %s failed", old_path.c_str(), newPath.c_str());
		// this next one we only try on Windows... if we are on a different platform
		// we simply give up and return an empty string
#ifdef WIN32
		if (subsurface_dir_rename(old_path.c_str(), newPath.c_str()) == 0)
#endif
			return std::string();
	}
	return newPath;
}

std::string get_file_name(const std::string &fileName)
{
	QFileInfo fileInfo(fileName.c_str());
	return fileInfo.fileName().toStdString();
}

static bool lessThan(const QPair<QString, int> &a, const QPair<QString, int> &b)
{
	return a.second < b.second;
}

QVector<QPair<QString, int>> selectedDivesGasUsed()
{
	QMap<QString, int> gasUsed;
	for (dive *d: getDiveSelection()) {
		std::vector<volume_t> diveGases = get_gas_used(d);
		for (size_t j = 0; j < d->cylinders.size(); j++) {
			if (diveGases[j].mliter) {
				QString gasName = QString::fromStdString(d->get_cylinder(j)->gasmix.name());
				gasUsed[gasName] += diveGases[j].mliter;
			}
		}
	}
	QVector<QPair<QString, int>> gasUsedOrdered;
	gasUsedOrdered.reserve(gasUsed.size());
	for (auto it = gasUsed.cbegin(); it != gasUsed.cend(); ++it)
		gasUsedOrdered.append(qMakePair(it.key(), it.value()));
	std::sort(gasUsedOrdered.begin(), gasUsedOrdered.end(), lessThan);

	return gasUsedOrdered;
}

QString getUserAgent()
{
	QString arch;
	// fill in the system data - use ':' as separator
	// replace all other ':' with ' ' so that this is easy to parse
#ifdef SUBSURFACE_MOBILE
	QString userAgent = QString("Subsurface-mobile:%1:").arg(subsurface_canonical_version());
#elif SUBSURFACE_DOWNLOADER
	QString userAgent = QString("Subsurface-downloader:%1:").arg(subsurface_canonical_version());
#else
	QString userAgent = QString("Subsurface:%1:").arg(subsurface_canonical_version());
#endif
	QString prettyOsName = QSysInfo::prettyProductName();
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID)
	// QSysInfo::kernelType() returns lowercase ("linux" instead of "Linux")
	struct utsname u;
	if (uname(&u) == 0)
		prettyOsName = QString::fromLatin1(u.sysname) + QLatin1String(" (") + prettyOsName + QLatin1Char(')');
#endif

	userAgent.append(prettyOsName.replace(':', ' ') + ":");
	arch = QSysInfo::buildCpuArchitecture().replace(':', ' ');
	userAgent.append(arch);
	if (arch == "i386")
		userAgent.append("/" + QSysInfo::currentCpuArchitecture());
	userAgent.append(":" + getUiLanguage());
	return userAgent;

}

std::string subsurface_user_agent()
{
	return getUserAgent().toStdString();
}

QString getUiLanguage()
{
	return QString::fromStdString(prefs.locale.lang_locale);
}

static std::vector<std::string> get_languages(const QLocale &loc)
{
	std::vector<std::string> res;
	for (const QString &s: loc.uiLanguages())
		res.push_back(s.toStdString());
	return res;
}

/* TOOD: Move this to SettingsObjectWrapper, and also fix this complexity.
 * gezus.
 */
void initUiLanguage()
{
	QString shortDateFormat;
	QString dateFormat;
	QString timeFormat;

	// set loc as system language or selected language
	if (!qPrefLanguage::use_system_language()) {
		loc = QLocale(qPrefLanguage::lang_locale());
	} else {
		loc = QLocale(QLocale().uiLanguages().first());
	}

	// Find language code with one '-', or use the first entry.
	std::vector<std::string> languages = get_languages(loc);
	std::string uiLang;
	if (!languages.empty()) {
		auto it = std::find_if(languages.begin(), languages.end(), [](const std::string &s)
					{ return std::count(s.begin(), s.end(), '-') == 1; });
		uiLang = it == languages.end() ? languages[0] : *it;
	}
#ifdef SUBSURFACE_MOBILE
	report_info("uiLanguages was %s, picked %s", join(languages, ", ").c_str(), uiLang.c_str());
#endif

	// there's a stupid Qt bug on MacOS where uiLanguages doesn't give us the country info
	if (!contains(uiLang, '-') && uiLang != loc.bcp47Name().toStdString()) {
		QLocale loc2(loc.bcp47Name());
		loc = loc2;
		std::vector<std::string> languages = get_languages(loc2);

		if (!languages.empty()) {
			auto it = std::find_if(languages.begin(), languages.end(), [](const std::string &s)
					{ return contains(s, '-'); });
			uiLang = it == languages.end() ? languages[0] : *it;
		}
#ifdef SUBSURFACE_MOBILE
		report_info("bcp47 based languages was %s, picked %s", join(languages, ", ").c_str(), uiLang.c_str());
#endif
	}

	prefs.locale.lang_locale = uiLang;

	if (!prefs.date_format_override || prefs.date_format.empty()) {
		// derive our standard date format from what the locale gives us
		// the long format uses long weekday and month names, so replace those with the short ones
		// for time we don't want the time zone designator and don't want leading zeroes on the hours
		dateFormat = loc.dateFormat(QLocale::LongFormat);
		dateFormat.replace("dddd,", "ddd").replace("dddd", "ddd").replace("MMMM", "MMM");
		// special hack for Swedish as our switching from long weekday names to short weekday names
		// messes things up there
		dateFormat.replace("'en' 'den' d:'e'", " d");
		prefs.date_format = dateFormat.toStdString();
	}

	if (!prefs.date_format_override || prefs.date_format_short.empty()) {
		// derive our standard date format from what the locale gives us
		shortDateFormat = loc.dateFormat(QLocale::ShortFormat);
		prefs.date_format_short = shortDateFormat.toStdString();
	}

	if (!prefs.time_format_override || prefs.time_format.empty()) {
		timeFormat = loc.timeFormat();
		timeFormat.replace("(t)", "").replace(" t", "").replace("t", "").replace("hh", "h").replace("HH", "H").replace("'kl'.", "");
		timeFormat.replace(".ss", "").replace(":ss", "").replace("ss", "");
		prefs.time_format = timeFormat.toStdString();
	}
}

QLocale getLocale()
{
	return loc;
}

QString get_depth_string(int mm, bool showunit, bool showdecimal)
{
	if (prefs.units.length == units::METERS) {
		double meters = mm / 1000.0;
		return QString("%L1%2").arg(meters, 0, 'f', (showdecimal && meters < 20.0) ? 1 : 0).arg(showunit ? gettextFromC::tr("m") : QString());
	} else {
		double feet = mm_to_feet(mm);
		return QString("%L1%2").arg(feet, 0, 'f', 0).arg(showunit ? gettextFromC::tr("ft") : QString());
	}
}

QString get_depth_string(depth_t depth, bool showunit, bool showdecimal)
{
	return get_depth_string(depth.mm, showunit, showdecimal);
}

QString get_depth_unit(bool metric)
{
	if (metric)
		return gettextFromC::tr("m");
	else
		return gettextFromC::tr("ft");
}

QString get_depth_unit()
{
	return get_depth_unit(prefs.units.length == units::METERS);
}

QString get_weight_string(weight_t weight, bool showunit)
{
	QString str = weight_string(weight);
	return showunit ? str + get_weight_unit() : str;
}

QString get_weight_unit(bool metric)
{
	if (metric)
		return gettextFromC::tr("kg");
	else
		return gettextFromC::tr("lbs");
}

QString get_weight_unit()
{
	return get_weight_unit(prefs.units.weight == units::KG);
}

QString get_temperature_string(temperature_t temp, bool showunit)
{
	if (temp.mkelvin == 0) {
		return ""; //temperature not defined
	} else if (prefs.units.temperature == units::CELSIUS) {
		double celsius = mkelvin_to_C(temp.mkelvin);
		return QString("%L1%2%3").arg(celsius, 0, 'f', 1).arg(showunit ? "°" : "").arg(showunit ? gettextFromC::tr("C") : QString());
	} else {
		double fahrenheit = mkelvin_to_F(temp.mkelvin);
		return QString("%L1%2%3").arg(fahrenheit, 0, 'f', 1).arg(showunit ? "°" : "").arg(showunit ? gettextFromC::tr("F") : QString());
	}
}

QString get_temp_unit(bool metric)
{
	if (metric)
		return QStringLiteral("°C");
	else
		return QStringLiteral("°F");
}

QString get_temp_unit()
{
	return get_temp_unit(prefs.units.temperature == units::CELSIUS);
}

QString get_volume_string(int mliter, bool showunit)
{
	const char *unit;
	int decimals;
	double value = get_volume_units(mliter, &decimals, &unit);
	return QString("%L1%2").arg(value, 0, 'f', decimals).arg(showunit ? unit : "");
}

QString get_volume_string(volume_t volume, bool showunit)
{
	return get_volume_string(volume.mliter, showunit);
}

QString get_volume_unit(bool metric)
{
	if (metric)
		return gettextFromC::tr("ℓ");
	else
		return gettextFromC::tr("cuft");
}

QString get_volume_unit()
{
	return get_volume_unit(prefs.units.volume == units::LITER);
}

QString get_pressure_string(pressure_t pressure, bool showunit)
{
	if (prefs.units.pressure == units::BAR) {
		double bar = pressure.mbar / 1000.0;
		return QString("%L1%2").arg(bar, 0, 'f', 0).arg(showunit ? gettextFromC::tr("bar") : QString());
	} else {
		double psi = mbar_to_PSI(pressure.mbar);
		return QString("%L1%2").arg(psi, 0, 'f', 0).arg(showunit ? gettextFromC::tr("psi") : QString());
	}
}

QString get_salinity_string(int salinity)
{
	return QStringLiteral("%L1%2").arg(salinity / 10.0).arg(gettextFromC::tr("g/ℓ"));
}

// the water types need to match the watertypes enum
static const char *waterTypes[] = {
	QT_TRANSLATE_NOOP("gettextFromC", "Fresh"),
	QT_TRANSLATE_NOOP("gettextFromC", "Brackish"),
	QT_TRANSLATE_NOOP("gettextFromC", "EN13319"),
	QT_TRANSLATE_NOOP("gettextFromC", "Salt"),
	QT_TRANSLATE_NOOP("gettextFromC", "Use DC")
};

QString get_water_type_string(int salinity)
{
	if (salinity < 10050)
		return gettextFromC::tr(waterTypes[FRESHWATER]);
	else if (salinity < 10190)
		return gettextFromC::tr(waterTypes[BRACKISHWATER]);
	else if (salinity < 10210)
		return gettextFromC::tr(waterTypes[EN13319WATER]);
	else
		return gettextFromC::tr(waterTypes[SALTWATER]);
}

QStringList getWaterTypesAsString()
{
	QStringList res;
	res.reserve(std::size(waterTypes));
	for (const char *t: waterTypes)
		res.push_back(gettextFromC::tr(t));
	return res;
}

QString getSubsurfaceDataPath(QString folderToFind)
{
	QString execdir;
	QDir folder;

	// first check if we are running in the build dir, so the path that we
	// are looking for is just a  subdirectory of the execution path;
	// this also works on Windows as there we install the dirs
	// under the application path
	execdir = QCoreApplication::applicationDirPath();
	folder = QDir(execdir.append(QDir::separator()).append(folderToFind));
	if (folder.exists())
		return folder.absolutePath();

	// next check for the Linux typical $(prefix)/share/subsurface
	execdir = QCoreApplication::applicationDirPath();
	if (execdir.contains("bin")) {
		folder = QDir(execdir.replace("bin", "share/subsurface/").append(folderToFind));
		if (folder.exists())
			return folder.absolutePath();
	}
	// then look for the usual locations on a Mac
	execdir = QCoreApplication::applicationDirPath();
	folder = QDir(execdir.append("/../Resources/share/").append(folderToFind));
	if (folder.exists())
		return folder.absolutePath();
	execdir = QCoreApplication::applicationDirPath();
	folder = QDir(execdir.append("/../Resources/").append(folderToFind));
	if (folder.exists())
		return folder.absolutePath();
	return QString();
}

static const char *printing_templates = "printing_templates";

QString getPrintingTemplatePathUser()
{
	// Function-local statics are initialized on first invocation
	static QString path(QString::fromStdString(system_default_directory()) +
			    QDir::separator() +
			    QString(printing_templates));
	return path;
}

QString getPrintingTemplatePathBundle()
{
	// Function-local statics are initialized on first invocation
	static QString path(getSubsurfaceDataPath(printing_templates));
	return path;
}

QDateTime timestampToDateTime(timestamp_t when)
{
	// Subsurface always uses "local time" as in "whatever was the local time at the location"
	// so all time stamps have no time zone information and are in UTC
	return QDateTime::fromMSecsSinceEpoch(1000 * when, Qt::UTC);
}

timestamp_t dateTimeToTimestamp(const QDateTime &t)
{
	return t.toSecsSinceEpoch();
}

QString render_seconds_to_string(int seconds)
{
	if (seconds % 60 == 0)
		return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC).toUTC().toString("h:mm");
	else
		return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC).toUTC().toString("h:mm:ss");
}

int parseDurationToSeconds(const QString &text)
{
	int secs;
	QString numOnly = text;
	QString hours, minutes, seconds;
	numOnly.replace(",", ".").remove(QRegularExpression("[^-0-9.:]"));
	if (numOnly.isEmpty())
		return 0;
	if (numOnly.contains(':')) {
		hours = numOnly.left(numOnly.indexOf(':'));
		minutes = numOnly.right(numOnly.length() - hours.length() - 1);
		if (minutes.contains(':')) {
			numOnly = minutes;
			minutes = numOnly.left(numOnly.indexOf(':'));
			seconds = numOnly.right(numOnly.length() - minutes.length() - 1);
		}
	} else {
		hours = QStringLiteral("0");
		minutes = std::move(numOnly);
	}
	secs = lrint(hours.toDouble() * 3600 + minutes.toDouble() * 60 + seconds.toDouble());
	return secs;
}

int parseLengthToMm(const QString &text)
{
	int mm;
	QString numOnly = text;
	numOnly.replace(",", ".").remove(QRegularExpression("[^-0-9.]"));
	if (numOnly.isEmpty())
		return 0;
	double number = numOnly.toDouble();
	if (text.contains(gettextFromC::tr("m"), Qt::CaseInsensitive)) {
		mm = lrint(number * 1000);
	} else if (text.contains(gettextFromC::tr("ft"), Qt::CaseInsensitive)) {
		mm = feet_to_mm(number);
	} else {
		switch (prefs.units.length) {
		case units::FEET:
			mm = feet_to_mm(number);
			break;
		case units::METERS:
			mm = lrint(number * 1000);
			break;
		default:
			mm = 0;
		}
	}
	return mm;

}

int parseTemperatureToMkelvin(const QString &text)
{
	int mkelvin;
	QString numOnly = text;
	numOnly.replace(",", ".").remove(QRegularExpression("[^-0-9.]"));
	if (numOnly.isEmpty())
		return 0;
	double number = numOnly.toDouble();
	if (text.contains(gettextFromC::tr("C"), Qt::CaseInsensitive)) {
		mkelvin = C_to_mkelvin(number);
	} else if (text.contains(gettextFromC::tr("F"), Qt::CaseInsensitive)) {
		mkelvin = F_to_mkelvin(number);
	} else {
		switch (prefs.units.temperature) {
		case units::CELSIUS:
			mkelvin = C_to_mkelvin(number);
			break;
		case units::FAHRENHEIT:
			mkelvin = F_to_mkelvin(number);
			break;
		default:
			mkelvin = 0;
		}
	}
	return mkelvin;
}

int parsePressureToMbar(const QString &text)
{
	int mbar;
	QString numOnly = text;
	// different locales use different symbols as group separator or decimal separator
	// (I think it's usually '.' and ',' - but maybe there are others?)
	// let's use Qt's help to get the parsing right
	QString validNumberCharacters("0-9");
	validNumberCharacters += loc.decimalPoint();
	validNumberCharacters += loc.groupSeparator();
	numOnly.remove(QRegularExpression(QString("[^%1]").arg(validNumberCharacters)));
	if (numOnly.isEmpty())
		return 0;
	double number = loc.toDouble(numOnly);
	if (text.contains(gettextFromC::tr("bar"), Qt::CaseInsensitive)) {
		mbar = lrint(number * 1000);
	} else if (text.contains(gettextFromC::tr("psi"), Qt::CaseInsensitive)) {
		mbar = psi_to_mbar(number);
	} else {
		switch (prefs.units.pressure) {
		case units::BAR:
			mbar = lrint(number * 1000);
			break;
		case units::PSI:
			mbar = psi_to_mbar(number);
			break;
		default:
			mbar = 0;
		}
	}
	return mbar;
}

int parseGasMixO2(const QString &text)
{
	QString gasString = text;
	int o2, number;
	if (gasString.contains(gettextFromC::tr("AIR"), Qt::CaseInsensitive)) {
		o2 = O2_IN_AIR;
	} else if (gasString.contains(gettextFromC::tr("EAN"), Qt::CaseInsensitive)) {
		gasString.remove(QRegularExpression("[^0-9]"));
		number = gasString.toInt();
		o2 = number * 10;
	} else if (gasString.contains("/")) {
		QStringList gasSplit = gasString.split("/");
		number = gasSplit[0].toInt();
		o2 = number * 10;
	} else {
		number = gasString.toInt();
		o2 = number * 10;
	}
	return o2;
}

int parseGasMixHE(const QString &text)
{
	QString gasString = text;
	int he, number;
	if (gasString.contains("/")) {
		QStringList gasSplit = gasString.split("/");
		number = gasSplit[1].toInt();
		he = number * 10;
	} else {
		he = 0;
	}
	return he;
}

QString get_dive_duration_string(timestamp_t when, QString hoursText, QString minutesText, QString secondsText, QString separator, bool isFreeDive)
{
	int hrs, mins, fullmins, secs;
	mins = (when + 30) / 60;
	fullmins = when / 60;
	secs = when - 60 * fullmins;
	hrs = mins / 60;

	QString displayTime;
	if (prefs.units.duration_units == units::ALWAYS_HOURS || (prefs.units.duration_units == units::MIXED && hrs)) {
		mins -= hrs * 60;
		displayTime = QString("%1%2%3%4%5").arg(hrs).arg(separator == ":" ? "" : hoursText).arg(separator)
			.arg(mins, 2, 10, QChar('0')).arg(separator == ":" ? hoursText : minutesText);
	} else if (isFreeDive && ( prefs.units.duration_units == units::MINUTES_ONLY || minutesText != "" )) {
		// Freedive <1h and we display no hours but only minutes for other dives
		// --> display a short (5min 35sec) freedives e.g. as "5:35"
		// Freedive <1h and we display a unit for minutes
		// --> display a short (5min 35sec) freedives e.g. as "5:35min"
		if (separator == ":") displayTime = QString("%1%2%3%4").arg(fullmins).arg(separator)
			.arg(secs, 2, 10, QChar('0')).arg(minutesText);
		else displayTime = QString("%1%2%3%4%5").arg(fullmins).arg(minutesText).arg(separator)
			.arg(secs).arg(secondsText);
	} else if (isFreeDive) {
		// Mixed display (hh:mm / mm only) and freedive < 1h and we have no unit for minutes
		// --> Prefix duration with "0:" --> "0:05:35"
		if (separator == ":") displayTime = QString("%1%2%3%4%5%6").arg(hrs).arg(separator)
			.arg(fullmins, 2, 10, QChar('0')).arg(separator)
			.arg(secs, 2, 10, QChar('0')).arg(hoursText);
		// Separator != ":" and no units for minutes --> unlikely case - remove?
		else displayTime = QString("%1%2%3%4%5%6%7%8").arg(hrs).arg(hoursText).arg(separator)
			.arg(fullmins).arg(minutesText).arg(separator)
			.arg(secs).arg(secondsText);
	} else {
		displayTime = QString("%1%2").arg(mins).arg(minutesText);
	}
	return displayTime;
}

QString get_dive_surfint_string(timestamp_t when, QString daysText, QString hoursText, QString minutesText, QString separator, int maxdays)
{
	int days, hrs, mins;
	days = when / 3600 / 24;
	hrs = (when - days * 3600 * 24) / 3600;
	mins = (when + 30 - days * 3600 * 24 - hrs * 3600) / 60;

	QString displayInt;
	if (maxdays && days > maxdays) displayInt = gettextFromC::tr("more than %1 days").arg(maxdays);
	else if (days) displayInt = QString("%1%2%3%4%5%6%7%8").arg(days).arg(daysText).arg(separator)
		.arg(hrs).arg(hoursText).arg(separator)
		.arg(mins).arg(minutesText);
	else displayInt = QString("%1%2%3%4%5").arg(hrs).arg(hoursText).arg(separator)
		.arg(mins).arg(minutesText);
	return displayInt;
}

QString get_dive_date_string(timestamp_t when)
{
	QDateTime ts;
	ts.setMSecsSinceEpoch(when * 1000L);
	return loc.toString(ts.toUTC(), QString::fromStdString(prefs.date_format + " " + prefs.time_format));
}

// Get local seconds since Epoch from ISO formatted UTC date time + offset string
time_t get_dive_datetime_from_isostring(const char *when) {
	QDateTime divetime = QDateTime::fromString(when, Qt::ISODate);
	return (time_t)(divetime.toSecsSinceEpoch());
}

QString get_short_dive_date_string(timestamp_t when)
{
	QDateTime ts;
	ts.setMSecsSinceEpoch(when * 1000L);
	return loc.toString(ts.toUTC(), QString::fromStdString(prefs.date_format_short + " " + prefs.time_format));
}

std::string get_dive_date_c_string(timestamp_t when)
{
	return get_short_dive_date_string(when).toStdString();
}

static QString get_dive_only_date_string(timestamp_t when)
{
	QDateTime ts;
	ts.setMSecsSinceEpoch(when * 1000L);
	return loc.toString(ts.toUTC(), QString::fromStdString(prefs.date_format));
}

QString get_first_dive_date_string()
{
	const dive_table &dives = divelog.dives;
	return !dives.empty() ? get_dive_only_date_string(dives[0]->when) : gettextFromC::tr("no dives");
}

QString get_last_dive_date_string()
{
	const dive_table &dives = divelog.dives;
	return !dives.empty() ? get_dive_only_date_string(dives.back()->when) : gettextFromC::tr("no dives");
}

std::string get_current_date()
{
	QDateTime ts(QDateTime::currentDateTime());;
	QString current_date;

	current_date = loc.toString(ts, QString::fromStdString(prefs.date_format_short));

	return current_date.toStdString();
}

static QMutex hashOfMutex;
static QHash<QString, QString> localFilenameOf;

std::string hashfile_name()
{
	return std::string(system_default_directory()) + "/hashes";
}

static QString thumbnailDir()
{
	return QString::fromStdString(system_default_directory() + "/thumbnails/");
}

// Calculate thumbnail filename by hashing name of file.
QString thumbnailFileName(const QString &filename)
{
	if (filename.isEmpty())
		return QString();
	QCryptographicHash hash(QCryptographicHash::Sha1);
	hash.addData(filename.toUtf8());
	return thumbnailDir() + hash.result().toHex();
}

// TODO: This is a temporary helper struct. Remove in due course with convertLocalFilename().
struct HashToFile {
	QByteArray hash;
	QString filename;
	bool operator< (const HashToFile &h) const {
		return hash < h.hash;
	}
};

// During a transition period, convert the hash->localFilename into a canonicalFilename->localFilename.
// TODO: remove this code in due course
static void convertLocalFilename(const QHash<QString, QByteArray> &hashOf, const QHash<QByteArray, QString> &hashToLocal)
{
	// Bail out early if there is nothing to do
	if (hashToLocal.isEmpty())
		return;

	// Create a vector of hash/filename pairs and sort by hash.
	// Elements can then be accessed with binary search.
	QHash<QByteArray, QString> canonicalFilenameByHash;
	QVector<HashToFile> h2f;
	h2f.reserve(hashOf.size());
	for (auto it = hashOf.cbegin(); it != hashOf.cend(); ++it)
		h2f.append({ it.value(), it.key() });
	std::sort(h2f.begin(), h2f.end());

	// Make the canonical-to-local connection
	for (auto it = hashToLocal.cbegin(); it != hashToLocal.cend(); ++it) {
		QByteArray hash = it.key();
		HashToFile dummy { hash, QString() };
		for(auto it2 = std::lower_bound(h2f.begin(), h2f.end(), dummy);
		    it2 != h2f.end() && it2->hash == hash; ++it2) {
			// Note that learnPictureFilename cares about all the special cases,
			// i.e. either filename being empty or both filenames being equal.
			learnPictureFilename(it2->filename, it.value());
		}
		QString canonicalFilename = canonicalFilenameByHash.value(it.key());
	}
}

void read_hashes()
{
	QFile hashfile(QString::fromStdString(hashfile_name()));
	if (hashfile.open(QIODevice::ReadOnly)) {
		QDataStream stream(&hashfile);
		QHash<QByteArray, QString> localFilenameByHash;
		QHash<QString, QByteArray> hashOf;
		stream >> localFilenameByHash;		// For backwards compatibility
		stream >> hashOf;			// For backwards compatibility
		QHash <QString, QImage> thumbnailCache;
		stream >> thumbnailCache;		// For backwards compatibility
		QMutexLocker locker(&hashOfMutex);
		stream >> localFilenameOf;
		locker.unlock();
		hashfile.close();
		convertLocalFilename(hashOf, localFilenameByHash);
	}
	QMutexLocker locker(&hashOfMutex);
	localFilenameOf.remove("");

	// Make sure that the thumbnail directory exists
	QDir().mkpath(thumbnailDir());
}

void write_hashes()
{
	QSaveFile hashfile(QString::fromStdString(hashfile_name()));
	QMutexLocker locker(&hashOfMutex);

	if (hashfile.open(QIODevice::WriteOnly)) {
		QDataStream stream(&hashfile);
		stream << QHash<QByteArray, QString>();	// Empty hash to filename - for backwards compatibility
		stream << QHash<QString, QByteArray>(); // Empty hashes - for backwards compatibility
		stream << QHash<QString,QImage>();	// Empty thumbnailCache - for backwards compatibility
		stream << localFilenameOf;
		hashfile.commit();
	} else {
		qWarning() << "Cannot open hashfile for writing: " << hashfile.fileName();
	}
}

void learnPictureFilename(const QString &originalName, const QString &localName)
{
	if (originalName.isEmpty() || localName.isEmpty())
		return;
	QMutexLocker locker(&hashOfMutex);
	// Only keep track of images where original and local names differ
	if (originalName == localName)
		localFilenameOf.remove(originalName);
	else
		localFilenameOf[originalName] = localName;
}

QString localFilePath(const QString &originalFilename)
{
	QMutexLocker locker(&hashOfMutex);
	return localFilenameOf.value(originalFilename, originalFilename);
}

// TODO: Apparently Qt has no simple way of listing the supported video
// codecs? Do we have to query them by hand using QMediaPlayer::hasSupport()?
const QStringList videoExtensionsList = {
	".avi", ".mp4", ".mov", ".mpeg", ".mpg", ".wmv"
};

// Raw extensions according to https://en.wikipedia.org/wiki/Raw_image_format
static const QStringList rawExtensionsList = {
#ifdef LIBRAW_SUPPORT
	"*.3fr", "*.ari", "*.arw", "*.bay", "*.braw", "*.crw", "*.cr2", "*.cr3", "*.cap",
	"*.data", "*.dcs", "*.dcr", "*.dng", "*.drf", "*.eip", "*.erf", "*.fff", "*.gpr",
	"*.iiq", "*.k25", "*.kdc", "*.mdc", "*.mef", "*.mos", "*.mrw", "*.nef", "*.nrw",
	"*.obm", "*.orf", "*.pef", "*.ptx", "*.pxn", "*.r3d", "*.raf", "*.raw", "*.rwl",
	"*.rw2", "*.rwz", "*.sr2", "*.srf", "*.srw", "*.x3f"
#endif
};

QStringList mediaExtensionFilters()
{
	return imageExtensionFilters() + videoExtensionFilters();
}

QStringList imageExtensionFilters()
{
	QStringList filters;
	for (QString format: QImageReader::supportedImageFormats())
		filters.append("*." + format);
	return filters + rawExtensionsList;
}

QStringList videoExtensionFilters()
{
	QStringList filters;
	for (const QString &format: videoExtensionsList)
		filters.append("*" + format);
	return filters;
}

std::string local_file_path(const struct picture &picture)
{
	return localFilePath(QString::fromStdString(picture.filename)).toStdString();
}

QString get_gas_string(struct gasmix gas)
{
	uint o2 = (get_o2(gas) + 5) / 10, he = (get_he(gas) + 5) / 10;
	QString result = gasmix_is_air(gas) ? gettextFromC::tr("AIR") : he == 0 ? (o2 == 100 ? gettextFromC::tr("OXYGEN") : QString("EAN%1").arg(o2, 2, 10, QChar('0'))) : QString("%1/%2").arg(o2).arg(he);
	return result;
}

QString get_dive_gas(const struct dive *d, int dcNr, int cylinderId)
{
	const cylinder_t *cyl = d->get_cylinder(cylinderId);
	const divecomputer *dc = d->get_dc(dcNr);

	bool showUse = (dc->divemode == CCR) | !is_cylinder_use_appropriate(*dc, *cyl, false);

	QString gasType = get_gas_string(cyl->gasmix);
	QString gasName;
	/* Check if we have the same gasmix two or more times
	 * If yes return more verbose string */
	int same_gas = same_gasmix_cylinder(*cyl, cylinderId, d, true);
	if (same_gas != -1) {
		gasType += QString(" (%1 %2").arg(gettextFromC::tr("cyl.")).arg(cylinderId + 1);
		gasName = QString::fromStdString(d->get_cylinder(cylinderId)->type.description);
	}

	if (showUse) {
		if (gasName.isNull())
			gasType += " (";
		else
			gasType += ", ";
		gasType += gettextFromC::tr(cylinderuse_text[d->get_cylinder(cylinderId)->cylinder_use]);
	}

	if (!gasName.isNull())
		gasType += QString(", %1").arg(gasName);

	if (!gasName.isNull() || showUse)
		gasType += ")";

	return gasType;
}

std::vector<std::pair<int, QString>> get_dive_gas_list(const struct dive *d, int dcNr, bool showOnlyAppropriate)
{
	const divecomputer *dc = d->get_dc(dcNr);
	std::vector<std::pair<int, QString>> list;
	for (unsigned int i = 0; i < d->cylinders.size(); i++) {
		if (showOnlyAppropriate && !is_cylinder_use_appropriate(*dc, *d->get_cylinder(i), false))
			continue;

		list.push_back(std::pair(i, get_dive_gas(d, dcNr, i)));
	}

	return list;
}

QStringList stringToList(const QString &s)
{
	QStringList res = s.split(",", SKIP_EMPTY);
	for (QString &str: res)
		str = str.trimmed();
	return res;
}

weight_t string_to_weight(const char *str)
{
	const char *end;
	double value = permissive_strtod(str, &end);
	QString rest = QString(end).trimmed();
	QString local_kg = gettextFromC::tr("kg");
	QString local_lbs = gettextFromC::tr("lbs");
	weight_t weight;

	if (rest.startsWith("kg") || rest.startsWith(local_kg))
		goto kg;
	// using just "lb" instead of "lbs" is intentional - some people might enter the singular
	if (rest.startsWith("lb") || rest.startsWith(local_lbs))
		goto lbs;
	if (prefs.units.weight == prefs.units.LBS)
		goto lbs;
kg:
	weight.grams = lrint(value * 1000);
	return weight;
lbs:
	weight.grams = lbs_to_grams(value);
	return weight;
}

depth_t string_to_depth(const char *str)
{
	const char *end;
	double value = permissive_strtod(str, &end);
	QString rest = QString(end).trimmed();
	QString local_ft = gettextFromC::tr("ft");
	QString local_m = gettextFromC::tr("m");
	depth_t depth;

	if (value < 0)
		value = 0;
	if (rest.startsWith("m") || rest.startsWith(local_m))
		goto m;
	if (rest.startsWith("ft") || rest.startsWith(local_ft))
		goto ft;
	if (prefs.units.length == prefs.units.FEET)
		goto ft;
m:
	depth.mm = lrint(value * 1000);
	return depth;
ft:
	depth.mm = feet_to_mm(value);
	return depth;
}

pressure_t string_to_pressure(const char *str)
{
	const char *end;
	double value = permissive_strtod(str, &end);
	QString rest = QString(end).trimmed();
	QString local_psi = gettextFromC::tr("psi");
	QString local_bar = gettextFromC::tr("bar");
	pressure_t pressure;

	if (rest.startsWith("bar") || rest.startsWith(local_bar))
		goto bar;
	if (rest.startsWith("psi") || rest.startsWith(local_psi))
		goto psi;
	if (prefs.units.pressure == prefs.units.PSI)
		goto psi;
bar:
	pressure.mbar = lrint(value * 1000);
	return pressure;
psi:
	pressure.mbar = psi_to_mbar(value);
	return pressure;
}

volume_t string_to_volume(const char *str, pressure_t workp)
{
	const char *end;
	double value = permissive_strtod(str, &end);
	QString rest = QString(end).trimmed();
	QString local_l = gettextFromC::tr("l");
	QString local_cuft = gettextFromC::tr("cuft");
	volume_t volume;

	if (rest.startsWith("l") || rest.startsWith("ℓ") || rest.startsWith(local_l))
		goto l;
	if (rest.startsWith("cuft") || rest.startsWith(local_cuft))
		goto cuft;
	/*
	 * If we don't have explicit units, and there is no working
	 * pressure, we're going to assume "liter" even in imperial
	 * measurements.
	 */
	if (!workp.mbar)
		goto l;
	if (prefs.units.volume == prefs.units.LITER)
		goto l;
cuft:
	if (workp.mbar)
		value /= bar_to_atm(workp.mbar / 1000.0);
	value = cuft_to_l(value);
l:
	volume.mliter = lrint(value * 1000);
	return volume;
}

fraction_t string_to_fraction(const char *str)
{
	const char *end;
	double value = permissive_strtod(str, &end);
	fraction_t fraction;

	fraction.permille = lrint(value * 10);
	/*
	 * Don't permit values less than zero or greater than 100%
	 */
	// TODO: use std::clamp() once we have comparison on unit types
	if (fraction.permille < 0)
		fraction = 0_percent;
	else if (fraction.permille > 1000)
		fraction = 100_percent;
	return fraction;
}

// Sadly, the standard C++ library's regexp support is mediocre at best.
static void sanitize_email(std::string &email)
{
	size_t j = 0;
	for (char c: email) {
		if (isalnum(c) || c == '@' || c == '.' ||
		    c == '_' || c == '+' || c == '-')
			email[j++] = c;
	}
	email.resize(j);
}

std::optional<std::string> getCloudURL()
{
	std::string email(prefs.cloud_storage_email);
	sanitize_email(email);
	if (email.empty() || prefs.cloud_storage_password.empty()) {
		report_error("Please configure Cloud storage email and password in the preferences");
		return {};
	}
	prefs.cloud_storage_email_encoded = email;
	std::string filename = std::string(prefs.cloud_base_url) + "git/" + email + "[" + email + "]";
	if (verbose)
		report_info("returning cloud URL %s", filename.c_str());
	return filename;
}

void subsurface_mkdir(const char *dir)
{
	QDir directory;
	if (!directory.mkpath(QString(dir)))
		report_info("failed to create path %s", dir);
}

enum deco_mode decoMode(bool in_planner)
{
	return in_planner ? prefs.planner_deco_mode : prefs.display_deco_mode;
}

void init_proxy()
{
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::ProxyType(prefs.proxy_type));
	proxy.setHostName(QString::fromStdString(prefs.proxy_host));
	proxy.setPort(prefs.proxy_port);
	if (prefs.proxy_auth) {
		proxy.setUser(QString::fromStdString(prefs.proxy_user));
		proxy.setPassword(QString::fromStdString(prefs.proxy_pass));
	}
	QNetworkProxy::setApplicationProxy(proxy);
}

void parse_seabear_header(const char *filename, struct xml_params *params)
{
	QFile f(filename);

	f.open(QFile::ReadOnly);
	QString parseLine = f.readLine();

	/*
	 * Parse dive number from Seabear CSV header
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		if (parseLine.contains("//DIVE NR: ")) {
			xml_params_add(params, "diveNro",
				       qPrintable(parseLine.replace(QString::fromLatin1("//DIVE NR: "), QString::fromLatin1(""))));
			break;
		}
	}
	f.seek(0);

	/*
	 * Parse header - currently only interested in sample
	 * interval and hardware version. If we have old format
	 * the interval value is missing from the header.
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		if (parseLine.contains("//Hardware Version: ")) {
			xml_params_add(params, "hw",
				       qPrintable(parseLine.replace(QString::fromLatin1("//Hardware Version: "),
						  QString::fromLatin1("\"Seabear ")).trimmed().append("\"")));
			break;
		}
	}
	f.seek(0);

	/*
	 * Grab the sample interval
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		if (parseLine.contains("//Log interval: ")) {
			xml_params_add(params, "delta",
				       qPrintable(parseLine.remove(QString::fromLatin1("//Log interval: ")).trimmed().remove(QString::fromLatin1(" s"))));
			break;
		}
	}
	f.seek(0);

	/*
	 * Dive mode, can be: OC, APNEA, BOTTOM TIMER, CCR, CCR SENSORBOARD
	 * Note that we scan over the "Log interval" on purpose
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		QString needle = "//Mode: ";
		if (parseLine.contains(needle)) {
			xml_params_add(params, "diveMode",
				       qPrintable(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\"")));
		}
	}
	f.seek(0);

	/*
	 * Grabbing some fields for the extradata
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		QString needle = "//Firmware Version: ";
		if (parseLine.contains(needle)) {
			xml_params_add(params, "Firmware",
				       qPrintable(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\"")));
		}
	}
	f.seek(0);

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		QString needle = "//Serial number: ";
		if (parseLine.contains(needle)) {
			xml_params_add(params, "Serial",
				       qPrintable(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\"")));
		}
	}
	f.seek(0);

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		QString needle = "//GF: ";
		if (parseLine.contains(needle)) {
			xml_params_add(params, "GF",
				       qPrintable(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\"")));
		}
	}
	f.seek(0);

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
	}

	/*
	 * Parse CSV fields
	 */

	parseLine = f.readLine().trimmed();

	const QStringList currColumns = parseLine.split(';');
	unsigned short index = 0;
	for (const QString &columnText: currColumns) {
		if (columnText == "Time") {
			xml_params_add_int(params, "timeField", index++);
		} else if (columnText == "Depth") {
			xml_params_add_int(params, "depthField", index++);
		} else if (columnText == "Temperature") {
			xml_params_add_int(params, "tempField", index++);
		} else if (columnText == "NDT") {
			xml_params_add_int(params, "ndlField", index++);
		} else if (columnText == "TTS") {
			xml_params_add_int(params, "ttsField", index++);
		} else if (columnText == "pO2_1") {
			xml_params_add_int(params, "o2sensor1Field", index++);
		} else if (columnText == "pO2_2") {
			xml_params_add_int(params, "o2sensor2Field", index++);
		} else if (columnText == "pO2_3") {
			xml_params_add_int(params, "o2sensor3Field", index++);
		} else if (columnText == "Ceiling") {
			/* TODO: Add support for dive computer reported ceiling*/
			xml_params_add_int(params, "ceilingField", index++);
		} else if (columnText == "Tank pressure") {
			xml_params_add_int(params, "pressureField", index++);
		} else {
			// We do not know about this value
			report_info("Seabear import found an un-handled field: %s", qPrintable(columnText));
		}
	}

	/* Separator is ';' and the index for that in DiveLogImportDialog constructor is 2 */
	xml_params_add_int(params, "separatorIndex", 2);

	/* And metric units */
	xml_params_add_int(params, "units", 0);

	f.close();
}

void print_qt_versions()
{
	printf("%s\n", qPrintable(QStringLiteral("built with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion())));
}

QMutex planLock;

void lock_planner()
{
	planLock.lock();
}

void unlock_planner()
{
	planLock.unlock();
}

// function to call to allow the UI to show updates for longer running activities
void (*uiNotificationCallback)(QString msg) = nullptr;

void uiNotification(const QString &msg)
{
	if (uiNotificationCallback != nullptr)
		uiNotificationCallback(msg);
}

// function to call to get changes for a git commit
QString (*changesCallback)() = nullptr;

std::string get_changes_made()
{
	return changesCallback != nullptr ? changesCallback().toStdString()
					  : std::string();
}

// Generate a cylinder-renumber map for use when the n-th cylinder
// of a dive with count cylinders is removed. It fills an int vector
// with 0..n, -1, n..count-1. Each entry in the vector represents
// the new id of the cylinder, whereby <0 means that this particular
// cylinder does not get any new id. This should probably be moved
// to the C-core, but using std::vector is simply more convenient.
// The function assumes that n < count!
std::vector<int> get_cylinder_map_for_remove(int count, int n)
{
	// 1) Fill mapping[0]..mapping[n-1] with 0..n-1
	// 2) Set mapping[n] to -1
	// 3) Fill mapping[n+1]..mapping[count-1] with n..count-2
	std::vector<int> mapping(count);
	std::iota(mapping.begin(), mapping.begin() + n, 0);
	mapping[n] = -1;
	std::iota(mapping.begin() + n + 1, mapping.end(), n);
	return mapping;
}

// Generate a cylinder-renumber map for use when a cylinder is added
// before the n-th cylinder. It fills an int vector with
// with 0..n-1, n+1..count. Each entry in the vector represents
// the new id of the cylinder. This probably should be moved
// to the C-core, but using std::vector is simply more convenient.
// This function assumes that that n <= count!
std::vector<int> get_cylinder_map_for_add(int count, int n)
{
	std::vector<int> mapping(count);
	std::iota(mapping.begin(), mapping.begin() + n, 0);
	std::iota(mapping.begin() + n, mapping.end(), n + 1);
	return mapping;
}

void emit_reset_signal()
{
	emit diveListNotifier.dataReset();
}
