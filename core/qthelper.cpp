// SPDX-License-Identifier: GPL-2.0
#include "qthelper.h"
#include "core/settings/qPrefLanguage.h"
#include "core/settings/qPrefUpdateManager.h"
#include "subsurface-string.h"
#include "subsurface-string.h"
#include "gettextfromc.h"
#include "statistics.h"
#include "membuffer.h"
#include "subsurfacesysinfo.h"
#include "version.h"
#include "divecomputer.h"
#include "errorhelper.h"
#include "planner.h"
#include "time.h"
#include "gettextfromc.h"
#include "applicationstate.h"
#include "metadata.h"
#include <sys/time.h>
#include "exif.h"
#include "file.h"
#include "tag.h"
#include "trip.h"
#include "imagedownloader.h"
#include <QFile>
#include <QRegExp>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QNetworkProxy>
#include <QDateTime>
#include <QImageReader>
#include <QtConcurrent>
#include <QFont>
#include <QApplication>
#include <QTextDocument>
#include <QProgressDialog>	// TODO: remove with convertThumbnails()
#include <cstdarg>
#include <cstdint>

#include <libxslt/documents.h>

const char *existing_filename;
static QLocale loc;

static inline QString degreeSigns()
{
    return QStringLiteral("dD\u00b0");
}

QString weight_string(int weight_in_grams)
{
	QString str;
	if (get_units()->weight == units::KG) {
		double kg = (double) weight_in_grams / 1000.0;
		str = QString("%L1").arg(kg, 0, 'f', kg >= 20.0 ? 0 : 1);
	} else {
		double lbs = grams_to_lbs(weight_in_grams);
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
		result.sprintf("%u°%02d\'%06.3f\"%s %u°%02d\'%06.3f\"%s",
			       latdeg, latmin / 1000000, latsec / 1000000, qPrintable(lath),
			       londeg, lonmin / 1000000, lonsec / 1000000, qPrintable(lonh));
	} else {
		result.sprintf("%f %f", (double) lat / 1000000.0, (double) lon / 1000000.0);
	}
	return result;
}

extern "C" char *printGPSCoordsC(const location_t *location)
{
	return copy_qstring(printGPSCoords(location));
}

/**
* Try to parse in a generic manner a coordinate.
*/
static bool parseCoord(const QString& txt, int& pos, const QString& positives,
		       const QString& negatives, const QString& others,
		       double& value)
{
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
			QRegExp numberRe("(\\d+(?:[\\.,]\\d+)?).*");
			if (!numberRe.exactMatch(txt.mid(pos)))
				return false;
			number = numberRe.cap(1).toDouble();
			numberDefined = true;
			posBeforeNumber = pos;
			pos += numberRe.cap(1).size() - 1;
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
			   (txt[pos] == ',' || txt[pos] == ';')) {
			// next coordinate coming up
			// eat the ',' and any subsequent white space
			while (++pos < txt.size() && txt[pos].isSpace())
				/* nothing */ ;
			break;
		} else {
			return false;
		}
		++pos;
	}
	if (!degreesDefined && numberDefined) {
		value = number; //just a single number => degrees
	} else if (!minutesDefined && numberDefined) {
		value += number / 60.0;
	} else if (!secondsDefined && numberDefined) {
		value += number / 3600.0;
	} else if (numberDefined) {
		return false;
	}
	if (sign == -1) value *= -1.0;
	return true;
}

/**
* Parse special coordinate formats that cannot be handled by parseCoord.
*/
static bool parseSpecialCoords(const QString& txt, double& latitude, double& longitude) {
	QRegExp xmlFormat("(-?\\d+(?:\\.\\d+)?),?\\s+(-?\\d+(?:\\.\\d+)?)");
	if (xmlFormat.exactMatch(txt)) {
		latitude = xmlFormat.cap(1).toDouble();
		longitude = xmlFormat.cap(2).toDouble();
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

	//remove the useless spaces (but keep the ones separating numbers)
	static const QRegExp SPACE_CLEANER("\\s*([" + POS_LAT + NEG_LAT + POS_LON +
		NEG_LON + degreeSigns() + "'\"\\s])\\s*");
	const QString normalized = gps_text.trimmed().toUpper().replace(SPACE_CLEANER, "\\1");

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

#if 0 // we'll need something like this for the dive site management, eventually
bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed_out)
{
	location_t location;
	bool ignore;
	bool *parsed = parsed_out ?: &ignore;
	*parsed = true;

	/* if we have a master and the dive's gps address is different from it,
	 * don't change the dive */
	if (master && !same_location(&master->location, &dive->location))
		return false;

	if (!(*parsed = parseGpsText(gps_text, location)))
		return false;

	/* if dive gps didn't change, nothing changed */
	if (same_location(&dive->location, location))
		return false;
	/* ok, update the dive and mark things changed */
	dive->location = location;
	return true;
}
#endif

QList<int> getDivesInTrip(dive_trip_t *trip)
{
	QList<int> ret;
	int i;
	struct dive *d;
	for_each_dive (i, d) {
		if (d->divetrip == trip) {
			ret.push_back(get_divenr(d));
		}
	}
	return ret;
}

static xmlDocPtr get_stylesheet_doc(const xmlChar *uri, xmlDictPtr, int, void *, xsltLoadType)
{
	QFile f(QLatin1String(":/xslt/") + (const char *)uri);
	if (!f.open(QIODevice::ReadOnly)) {
		if (verbose > 0) {
			qDebug() << "cannot open stylesheet" << QLatin1String(":/xslt/") + (const char *)uri << f.errorString();
			return NULL;
		}
	}
	/* Load and parse the data */
	QByteArray source = f.readAll();

	xmlDocPtr doc = xmlParseMemory(source, source.size());
	return doc;
}

extern "C" xsltStylesheetPtr get_stylesheet(const char *name)
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

extern "C" char *move_away(const char *old_path)
{
	if (verbose > 1)
		qDebug() << "move away" << old_path;
	QDir oldDir(old_path);
	QDir newDir;
	QString newPath;
	int i = 0;
	do {
		newPath = QString(old_path) + QString(".%1").arg(++i);
		newDir.setPath(newPath);
	} while(newDir.exists());
	if (verbose > 1)
		qDebug() << "renaming to" << newPath;
	if (!oldDir.rename(old_path, newPath)) {
		if (verbose)
			qDebug() << "rename of" << old_path << "to" << newPath << "failed";
		// this next one we only try on Windows... if we are on a different platform
		// we simply give up and return an empty string
#ifdef WIN32
		if (subsurface_dir_rename(old_path, qPrintable(newPath)) == 0)
#endif
			return strdup("");
	}
	return copy_qstring(newPath);
}

extern "C" char *get_file_name(const char *fileName)
{
	QFileInfo fileInfo(fileName);
	return copy_qstring(fileInfo.fileName());
}

extern "C" void copy_image_and_overwrite(const char *cfileName, const char *path, const char *cnewName)
{
	QString fileName(cfileName);
	QString newName(path);
	newName += cnewName;
	QFile file(newName);
	if (file.exists())
		file.remove();
	if (!QFile::copy(fileName, newName))
		qDebug() << "copy of" << fileName << "to" << newName << "failed";
}

static bool lessThan(const QPair<QString, int> &a, const QPair<QString, int> &b)
{
	return a.second < b.second;
}

QVector<QPair<QString, int>> selectedDivesGasUsed()
{
	int i, j;
	struct dive *d;
	QMap<QString, int> gasUsed;
	for_each_dive (i, d) {
		if (!d->selected)
			continue;
		volume_t diveGases[MAX_CYLINDERS] = {};
		get_gas_used(d, diveGases);
		for (j = 0; j < MAX_CYLINDERS; j++)
			if (diveGases[j].mliter) {
				QString gasName = gasname(d->cylinder[j].gasmix);
				gasUsed[gasName] += diveGases[j].mliter;
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
	QString userAgent = QString("Subsurface-mobile:%1(%2):").arg(subsurface_mobile_version()).arg(subsurface_canonical_version());
#else
	QString userAgent = QString("Subsurface:%1:").arg(subsurface_canonical_version());
#endif
	userAgent.append(SubsurfaceSysInfo::prettyOsName().replace(':', ' ') + ":");
	arch = SubsurfaceSysInfo::buildCpuArchitecture().replace(':', ' ');
	userAgent.append(arch);
	if (arch == "i386")
		userAgent.append("/" + SubsurfaceSysInfo::currentCpuArchitecture());
	userAgent.append(":" + uiLanguage(NULL));
	return userAgent;

}

extern "C" const char *subsurface_user_agent()
{
	static QString uA = getUserAgent();

	return copy_qstring(uA);
}

/* TOOD: Move this to SettingsObjectWrapper, and also fix this complexity.
 * gezus.
 */
QString uiLanguage(QLocale *callerLoc)
{
	QString shortDateFormat;
	QString dateFormat;
	QString timeFormat;

	// Language settings are already loaded, see qPref::load()
	// so no need to reload them

	// remark this method used "useSystemLanguage", which is not set
	// instead use_system_language is loaded from disk

	// set loc as system language or selected language
	if (!qPrefLanguage::use_system_language()) {
		loc = QLocale(qPrefLanguage::lang_locale());
	} else {
		loc = QLocale(QLocale().uiLanguages().first());
	}

	QStringList languages = loc.uiLanguages();
	QString uiLang;
	if (languages[0].contains('-'))
		uiLang = languages[0];
	else if (languages.count() > 1 && languages[1].contains('-'))
		uiLang = languages[1];
	else if (languages.count() > 2 && languages[2].contains('-'))
		uiLang = languages[2];
	else
		uiLang = languages[0];

	prefs.locale.lang_locale = copy_qstring(uiLang);

	// there's a stupid Qt bug on MacOS where uiLanguages doesn't give us the country info
	if (!uiLang.contains('-') && uiLang != loc.bcp47Name()) {
		QLocale loc2(loc.bcp47Name());
		loc = loc2;
		QStringList languages = loc2.uiLanguages();
		if (languages[0].contains('-'))
			uiLang = languages[0];
		else if (languages.count() > 1 && languages[1].contains('-'))
			uiLang = languages[1];
		else if (languages.count() > 2 && languages[2].contains('-'))
			uiLang = languages[2];
	}
	if (callerLoc)
		*callerLoc = loc;

	if (!prefs.date_format_override || empty_string(prefs.date_format_short) || empty_string(prefs.date_format)) {
		// derive our standard date format from what the locale gives us
		// the short format is fine
		// the long format uses long weekday and month names, so replace those with the short ones
		// for time we don't want the time zone designator and don't want leading zeroes on the hours
		shortDateFormat = loc.dateFormat(QLocale::ShortFormat);
		dateFormat = loc.dateFormat(QLocale::LongFormat);
		dateFormat.replace("dddd,", "ddd").replace("dddd", "ddd").replace("MMMM", "MMM");
		// special hack for Swedish as our switching from long weekday names to short weekday names
		// messes things up there
		dateFormat.replace("'en' 'den' d:'e'", " d");
		if (!prefs.date_format_override || empty_string(prefs.date_format)) {
			free((void *)prefs.date_format);
			prefs.date_format = copy_qstring(dateFormat);
		}
		if (!prefs.date_format_override || empty_string(prefs.date_format_short)) {
			free((void *)prefs.date_format_short);
			prefs.date_format_short = copy_qstring(shortDateFormat);
		}
	}
	if (!prefs.time_format_override || empty_string(prefs.time_format)) {
		timeFormat = loc.timeFormat();
		timeFormat.replace("(t)", "").replace(" t", "").replace("t", "").replace("hh", "h").replace("HH", "H").replace("'kl'.", "");
		timeFormat.replace(".ss", "").replace(":ss", "").replace("ss", "");
		free((void *)prefs.time_format);
		prefs.time_format = copy_qstring(timeFormat);
	}
	return uiLang;
}

QLocale getLocale()
{
	return loc;
}

void set_filename(const char *filename)
{
	free((void *)existing_filename);
	existing_filename = copy_string(filename);
}

QString get_dc_nickname(const char *model, uint32_t deviceid)
{
	const DiveComputerNode *existNode = dcList.getExact(model, deviceid);

	if (existNode && !existNode->nickName.isEmpty())
		return existNode->nickName;
	else
		return model;
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

QString get_depth_unit()
{
	if (prefs.units.length == units::METERS)
		return gettextFromC::tr("m");
	else
		return gettextFromC::tr("ft");
}

QString get_weight_string(weight_t weight, bool showunit)
{
	QString str = weight_string(weight.grams);
	if (get_units()->weight == units::KG) {
		str = QString("%1%2").arg(str, showunit ? gettextFromC::tr("kg") : QString());
	} else {
		str = QString("%1%2").arg(str, showunit ? gettextFromC::tr("lbs") : QString());
	}
	return str;
}

QString get_weight_unit()
{
	if (prefs.units.weight == units::KG)
		return gettextFromC::tr("kg");
	else
		return gettextFromC::tr("lbs");
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

QString get_temp_unit()
{
	if (prefs.units.temperature == units::CELSIUS)
		return QString("°C");
	else
		return QString("°F");
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

QString get_volume_unit()
{
	const char *unit;
	(void) get_volume_units(0, NULL, &unit);
	return QString(unit);
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
	static QString path = QString();
	if (path.isEmpty())
		path = QString(system_default_directory()) + QDir::separator() + QString(printing_templates);
	return path;
}

QString getPrintingTemplatePathBundle()
{
	static QString path = QString();
	if (path.isEmpty())
		path = getSubsurfaceDataPath(printing_templates);
	return path;
}

int gettimezoneoffset(timestamp_t when)
{
	QDateTime dt1, dt2;
	if (when == 0)
		dt1 = QDateTime::currentDateTime();
	else
		dt1 = QDateTime::fromMSecsSinceEpoch(when * 1000);
	dt2 = dt1.toUTC();
	dt1.setTimeSpec(Qt::UTC);
	return dt2.secsTo(dt1);
}

QString render_seconds_to_string(int seconds)
{
	if (seconds % 60 == 0)
		return QDateTime::fromTime_t(seconds).toUTC().toString("h:mm");
	else
		return QDateTime::fromTime_t(seconds).toUTC().toString("h:mm:ss");
}

int parseDurationToSeconds(const QString &text)
{
	int secs;
	QString numOnly = text;
	QString hours, minutes, seconds;
	numOnly.replace(",", ".").remove(QRegExp("[^-0-9.:]"));
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
		hours = "0";
		minutes = numOnly;
	}
	secs = lrint(hours.toDouble() * 3600 + minutes.toDouble() * 60 + seconds.toDouble());
	return secs;
}

int parseLengthToMm(const QString &text)
{
	int mm;
	QString numOnly = text;
	numOnly.replace(",", ".").remove(QRegExp("[^-0-9.]"));
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
	numOnly.replace(",", ".").remove(QRegExp("[^-0-9.]"));
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

int parseWeightToGrams(const QString &text)
{
	int grams;
	QString numOnly = text;
	numOnly.replace(",", ".").remove(QRegExp("[^0-9.]"));
	if (numOnly.isEmpty())
		return 0;
	double number = numOnly.toDouble();
	if (text.contains(gettextFromC::tr("kg"), Qt::CaseInsensitive)) {
		grams = lrint(number * 1000);
	} else if (text.contains(gettextFromC::tr("lbs"), Qt::CaseInsensitive)) {
		grams = lbs_to_grams(number);
	} else {
		switch (prefs.units.weight) {
		case units::KG:
			grams = lrint(number * 1000);
			break;
		case units::LBS:
			grams = lbs_to_grams(number);
			break;
		default:
			grams = 0;
		}
	}
	return grams;
}

int parsePressureToMbar(const QString &text)
{
	int mbar;
	QString numOnly = text;
	numOnly.replace(",", ".").remove(QRegExp("[^0-9.]"));
	if (numOnly.isEmpty())
		return 0;
	double number = numOnly.toDouble();
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
		gasString.remove(QRegExp("[^0-9]"));
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
	return loc.toString(ts.toUTC(), QString(prefs.date_format) + " " + prefs.time_format);
}

QString get_short_dive_date_string(timestamp_t when)
{
	QDateTime ts;
	ts.setMSecsSinceEpoch(when * 1000L);
	return loc.toString(ts.toUTC(), QString(prefs.date_format_short) + " " + prefs.time_format);
}

char *get_dive_date_c_string(timestamp_t when)
{
	QString text = get_dive_date_string(when);
	return copy_qstring(text);
}

extern "C" char *get_current_date()
{
	QDateTime ts(QDateTime::currentDateTime());;
	QString current_date;

	current_date = loc.toString(ts, QString(prefs.date_format_short));

	return copy_qstring(current_date);
}

QString get_trip_date_string(timestamp_t when, int nr, bool getday)
{
	struct tm tm;
	utc_mkdate(when, &tm);
	QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000*when,Qt::UTC);
	localTime.setTimeSpec(Qt::UTC);

	QString suffix = " " + gettextFromC::tr("(%n dive(s))", "", nr);
	if (getday)
		return loc.toString(localTime, prefs.date_format) + suffix;
	else
		return loc.toString(localTime, "MMM yyyy") + suffix;
}

static QMutex hashOfMutex;
static QHash<QString, QString> localFilenameOf;

static const QString hashfile_name()
{
	return QString(system_default_directory()).append("/hashes");
}

static QString thumbnailDir()
{
	return QString(system_default_directory()) + "/thumbnails/";
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

extern "C" char *hashfile_name_string()
{
	return copy_qstring(hashfile_name());
}

// During a transition period, convert old thumbnail-hashes to individual files
// TODO: remove this code in due course
static void convertThumbnails(const QHash <QString, QImage> &thumbnails)
{
	if (thumbnails.empty())
		return;
	// This is a singular occurrence, therefore translating the strings seems not worth it
	QProgressDialog progress("Convert thumbnails...", "Abort", 0, thumbnails.size());
	progress.setWindowModality(Qt::WindowModal);

	int count = 0;
	for (const QString &name: thumbnails.keys()) {
		const QImage thumbnail = thumbnails[name];

		if (thumbnail.isNull())
			continue;

		// This is duplicate code (see qt-models/divepicturemodel.cpp)
		// Not a problem, since this routine will be removed in due course.
		QString filename = thumbnailFileName(name);
		if (filename.isEmpty())
			continue;

		QSaveFile file(filename);
		if (!file.open(QIODevice::WriteOnly))
			return;
		QDataStream stream(&file);

		quint32 type = MEDIATYPE_PICTURE;
		stream << type;
		stream << thumbnail;
		file.commit();

		progress.setValue(++count);
		if (progress.wasCanceled())
			break;
	}
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
	// Elements can than be accessed with binary search.
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
	QFile hashfile(hashfile_name());
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
		convertThumbnails(thumbnailCache);
		convertLocalFilename(hashOf, localFilenameByHash);
	}
	QMutexLocker locker(&hashOfMutex);
	localFilenameOf.remove("");

	// Make sure that the thumbnail directory exists
	QDir().mkpath(thumbnailDir());
}

void write_hashes()
{
	QSaveFile hashfile(hashfile_name());
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

QStringList mediaExtensionFilters()
{
	return imageExtensionFilters() + videoExtensionFilters();
}

QStringList imageExtensionFilters()
{
	QStringList filters;
	foreach (const QString &format, QImageReader::supportedImageFormats())
		filters.append("*." + format);
	return filters;
}

QStringList videoExtensionFilters()
{
	QStringList filters;
	for (const QString &format: videoExtensionsList)
		filters.append("*" + format);
	return filters;
}

extern "C" const char *local_file_path(struct picture *picture)
{
	return copy_qstring(localFilePath(picture->filename));
}

const QString picturedir()
{
	return QString(system_default_directory()).append("/picturedata/");
}

extern "C" char *picturedir_string()
{
	return copy_qstring(picturedir());
}

QString get_gas_string(struct gasmix gas)
{
	uint o2 = (get_o2(gas) + 5) / 10, he = (get_he(gas) + 5) / 10;
	QString result = gasmix_is_air(gas) ? gettextFromC::tr("AIR") : he == 0 ? (o2 == 100 ? gettextFromC::tr("OXYGEN") : QString("EAN%1").arg(o2, 2, 10, QChar('0'))) : QString("%1/%2").arg(o2).arg(he);
	return result;
}

QString get_divepoint_gas_string(struct dive *d, const divedatapoint &p)
{
	int idx = p.cylinderid;
	return get_gas_string(d->cylinder[idx].gasmix);
}

QString get_taglist_string(struct tag_entry *tag_list)
{
	char *buffer = taglist_get_tagstring(tag_list);
	QString ret = QString::fromUtf8(buffer);
	free(buffer);
	return ret;
}

weight_t string_to_weight(const char *str)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
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
	double value = strtod_flags(str, &end, 0);
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
	double value = strtod_flags(str, &end, 0);
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
	double value = strtod_flags(str, &end, 0);
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
	double value = strtod_flags(str, &end, 0);
	fraction_t fraction;

	fraction.permille = lrint(value * 10);
	/*
	 * Don't permit values less than zero or greater than 100%
	 */
	if (fraction.permille < 0)
		fraction.permille = 0;
	else if (fraction.permille > 1000)
		fraction.permille = 1000;
	return fraction;
}

int getCloudURL(QString &filename)
{
	QString email = QString(prefs.cloud_storage_email);
	email.replace(QRegularExpression("[^a-zA-Z0-9@._+-]"), "");
	if (email.isEmpty() || empty_string(prefs.cloud_storage_password))
		return report_error("Please configure Cloud storage email and password in the preferences");
	if (email != prefs.cloud_storage_email_encoded) {
		free((void *)prefs.cloud_storage_email_encoded);
		prefs.cloud_storage_email_encoded = copy_qstring(email);
	}
	filename = QString(QString(prefs.cloud_git_url) + "/%1[%1]").arg(email);
	if (verbose)
		qDebug() << "cloud URL set as" << filename;
	return 0;
}

extern "C" char *cloud_url()
{
	QString filename;
	getCloudURL(filename);
	return copy_qstring(filename);
}

extern "C" bool getProxyString(char **buffer)
{
	if (prefs.proxy_type == QNetworkProxy::HttpProxy) {
		QString proxy;
		if (prefs.proxy_auth)
			proxy = QString("http://%1:%2@%3:%4").arg(prefs.proxy_user).arg(prefs.proxy_pass)
					.arg(prefs.proxy_host).arg(prefs.proxy_port);
		else
			proxy = QString("http://%1:%2").arg(prefs.proxy_host).arg(prefs.proxy_port);
		if (buffer)
			*buffer = copy_qstring(proxy);
		return true;
	}
	return false;
}

extern "C" void subsurface_mkdir(const char *dir)
{
	QDir directory;
	if (!directory.mkpath(QString(dir)))
		qDebug() << "failed to create path" << dir;
}

extern "C" void parse_display_units(char *line)
{
	qDebug() << line;
}

extern "C" bool in_planner()
{
	return getAppState() == ApplicationState::PlanDive || getAppState() == ApplicationState::EditPlannedDive;
}

extern "C" enum deco_mode decoMode()
{
	return in_planner() ? prefs.planner_deco_mode : prefs.display_deco_mode;
}

void init_proxy()
{
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::ProxyType(prefs.proxy_type));
	proxy.setHostName(prefs.proxy_host);
	proxy.setPort(prefs.proxy_port);
	if (prefs.proxy_auth) {
		proxy.setUser(prefs.proxy_user);
		proxy.setPassword(prefs.proxy_pass);
	}
	QNetworkProxy::setApplicationProxy(proxy);
}

QString getUUID()
{
	QString uuidString;
	uuidString = qPrefUpdateManager::uuidString();
	if (uuidString != "") {
		QUuid uuid = QUuid::createUuid();
		uuidString = uuid.toString();
		qPrefUpdateManager::set_uuidString(uuidString);
	}
	uuidString.replace("{", "").replace("}", "");
	return uuidString;
}

static bool contains(const char *s, const QString &filterstring, Qt::CaseSensitivity cs)
{
	return !empty_string(s) && QString(s).contains(filterstring, cs);
}

bool diveContainsText(const struct dive *d, const QString &filterstring, Qt::CaseSensitivity cs, bool includeNotes)
{
	if (!d)
		return false;
	return contains(get_dive_location(d), filterstring, cs) ||
	       (d->divetrip && contains(d->divetrip->location, filterstring, cs)) ||
	       contains(d->buddy, filterstring, cs) ||
	       contains(d->divemaster, filterstring, cs) ||
	       contains(d->suit, filterstring, cs) ||
	       get_taglist_string(d->tag_list).contains(filterstring, cs) ||
	       (includeNotes && contains(d->notes, filterstring, cs));
}
int parse_seabear_header(const char *filename, char **params, int pnr)
{
	QFile f(filename);

	f.open(QFile::ReadOnly);
	QString parseLine = f.readLine();

	/*
	 * Parse dive number from Seabear CSV header
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		if (parseLine.contains("//DIVE NR: ")) {
			params[pnr++] = strdup("diveNro");
			params[pnr++] = copy_qstring(parseLine.replace(QString::fromLatin1("//DIVE NR: "), QString::fromLatin1("")));
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
			params[pnr++] = strdup("hw");
			params[pnr++] = copy_qstring(parseLine.replace(QString::fromLatin1("//Hardware Version: "), QString::fromLatin1("\"Seabear ")).trimmed().append("\""));
			break;
		}
	}
	f.seek(0);

	/*
	 * Grab the sample interval
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		if (parseLine.contains("//Log interval: ")) {
			params[pnr++] = strdup("delta");
			params[pnr++] = copy_qstring(parseLine.remove(QString::fromLatin1("//Log interval: ")).trimmed().remove(QString::fromLatin1(" s")));
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
			params[pnr++] = strdup("diveMode");
			params[pnr++] = copy_qstring(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\""));
		}
	}
	f.seek(0);

	/*
	 * Grabbing some fields for the extradata
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		QString needle = "//Firmware Version: ";
		if (parseLine.contains(needle)) {
			params[pnr++] = strdup("Firmware");
			params[pnr++] = copy_qstring(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\""));
		}
	}
	f.seek(0);

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		QString needle = "//Serial number: ";
		if (parseLine.contains(needle)) {
			params[pnr++] = strdup("Serial");
			params[pnr++] = copy_qstring(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\""));
		}
	}
	f.seek(0);

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		QString needle = "//GF: ";
		if (parseLine.contains(needle)) {
			params[pnr++] = strdup("GF");
			params[pnr++] = copy_qstring(parseLine.replace(needle, QString::fromLatin1("")).prepend("\"").append("\""));
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
			params[pnr++] = strdup("timeField");
			params[pnr++] = intdup(index++);
		} else if (columnText == "Depth") {
			params[pnr++] = strdup("depthField");
			params[pnr++] = intdup(index++);
		} else if (columnText == "Temperature") {
			params[pnr++] = strdup("tempField");
			params[pnr++] = intdup(index++);
		} else if (columnText == "NDT") {
			params[pnr++] = strdup("ndlField");
			params[pnr++] = intdup(index++);
		} else if (columnText == "TTS") {
			params[pnr++] = strdup("ttsField");
			params[pnr++] = intdup(index++);
		} else if (columnText == "pO2_1") {
			params[pnr++] = strdup("o2sensor1Field");
			params[pnr++] = intdup(index++);
		} else if (columnText == "pO2_2") {
			params[pnr++] = strdup("o2sensor2Field");
			params[pnr++] = intdup(index++);
		} else if (columnText == "pO2_3") {
			params[pnr++] = strdup("o2sensor3Field");
			params[pnr++] = intdup(index++);
		} else if (columnText == "Ceiling") {
			/* TODO: Add support for dive computer reported ceiling*/
			params[pnr++] = strdup("ceilingField");
			params[pnr++] = intdup(index++);
		} else if (columnText == "Tank pressure") {
			params[pnr++] = strdup("pressureField");
			params[pnr++] = intdup(index++);
		} else {
			// We do not know about this value
			qDebug() << "Seabear import found an un-handled field: " << columnText;
		}
	}

	/* Separator is ';' and the index for that in DiveLogImportDialog constructor is 2 */
	params[pnr++] = strdup("separatorIndex");
	params[pnr++] = intdup(2);

	/* And metric units */
	params[pnr++] = strdup("units");
	params[pnr++] = intdup(0);

	params[pnr] = NULL;
	f.close();
	return pnr;
}

char *intdup(int index)
{
	char tmpbuf[21];

	snprintf(tmpbuf, sizeof(tmpbuf) - 2, "%d", index);
	tmpbuf[20] = 0;
	return strdup(tmpbuf);
}

QHash<int, double> factor_cache;

QReadWriteLock factorCacheLock;
extern "C" double cache_value(int tissue, int timestep, enum inertgas inertgas)
{
	double value;
	int key = (timestep << 5) + (tissue << 1);
	if (inertgas == HE)
		++key;
	factorCacheLock.lockForRead();
	value = factor_cache.value(key);
	factorCacheLock.unlock();
	return value;
}

extern "C" void cache_insert(int tissue, int timestep, enum inertgas inertgas, double value)
{
	int key = (timestep << 5) + (tissue << 1);
	if (inertgas == HE)
		++key;
	factorCacheLock.lockForWrite();
	factor_cache.insert(key, value);
	factorCacheLock.unlock();
}

extern "C" void print_qt_versions()
{
	printf("%s\n", qPrintable(QStringLiteral("built with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion())));
}

QMutex planLock;

extern "C" void lock_planner()
{
	planLock.lock();
}

extern "C" void unlock_planner()
{
	planLock.unlock();
}

char *copy_qstring(const QString &s)
{
	return strdup(qPrintable(s));
}
