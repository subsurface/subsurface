// SPDX-License-Identifier: GPL-2.0
#include "qthelper.h"
#include "helpers.h"
#include "gettextfromc.h"
#include "statistics.h"
#include "membuffer.h"
#include "subsurfacesysinfo.h"
#include "version.h"
#include "divecomputer.h"
#include "time.h"
#include "gettextfromc.h"
#include <sys/time.h>
#include "exif.h"
#include "file.h"
#include "prefs-macros.h"
#include <QFile>
#include <QRegExp>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QDateTime>
#include <QImageReader>
#include <QtConcurrent>
#include <QFont>
#include <QApplication>
#include <QTextDocument>

#include <libxslt/documents.h>

const char *existing_filename;
static QLocale loc;

#define translate(_context, arg) trGettext(arg)
static const QString DEGREE_SIGNS("dD" UTF8_DEGREE);

QString weight_string(int weight_in_grams)
{
	QString str;
	if (get_units()->weight == units::KG) {
		int gr = weight_in_grams % 1000;
		int kg = weight_in_grams / 1000;
		if (kg >= 20.0)
			str = QString("%1").arg(kg + (gr >= 500 ? 1 : 0));
		else
			str = QString("%1.%2").arg(kg).arg((unsigned)(gr + 50) / 100);
	} else {
		double lbs = grams_to_lbs(weight_in_grams);
		if (lbs >= 40.0)
			lbs = lrint(lbs + 0.5);
		else
			lbs = lrint(lbs + 0.05);
		str = QString("%1").arg(lbs, 0, 'f', lbs >= 40.0 ? 0 : 1);
	}
	return (str);
}

QString distance_string(int distanceInMeters)
{
	QString str;
	if(get_units()->length == units::METERS) {
		if (distanceInMeters >= 1000)
			str = QString(translate("gettextFromC", "%1km")).arg(distanceInMeters / 1000);
		else
			str = QString(translate("gettextFromC", "%1m")).arg(distanceInMeters);
	} else {
		double miles = m_to_mile(distanceInMeters);
		if (miles >= 1.0)
			str = QString(translate("gettextFromC", "%1mi")).arg((int)miles);
		else
			str = QString(translate("gettextFromC", "%1yd")).arg((int)(miles * 1760));
	}
	return str;
}

extern "C" const char *printGPSCoords(int lat, int lon)
{
	unsigned int latdeg, londeg;
	unsigned int latmin, lonmin;
	double latsec, lonsec;
	QString lath, lonh, result;

	if (!lat && !lon)
		return strdup("");

	if (prefs.coordinates_traditional) {
		lath = lat >= 0 ? translate("gettextFromC", "N") : translate("gettextFromC", "S");
		lonh = lon >= 0 ? translate("gettextFromC", "E") : translate("gettextFromC", "W");
		lat = abs(lat);
		lon = abs(lon);
		latdeg = lat / 1000000U;
		londeg = lon / 1000000U;
		latmin = (lat % 1000000U) * 60U;
		lonmin = (lon % 1000000U) * 60U;
		latsec = (latmin % 1000000) * 60;
		lonsec = (lonmin % 1000000) * 60;
		result.sprintf("%u%s%02d\'%06.3f\"%s %u%s%02d\'%06.3f\"%s",
			       latdeg, UTF8_DEGREE, latmin / 1000000, latsec / 1000000, lath.toUtf8().data(),
			       londeg, UTF8_DEGREE, lonmin / 1000000, lonsec / 1000000, lonh.toUtf8().data());
	} else {
		result.sprintf("%f %f", (double) lat / 1000000.0, (double) lon / 1000000.0);
	}
	return strdup(result.toUtf8().data());
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
		} else if (DEGREE_SIGNS.indexOf(txt[pos]) >= 0 ||
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
	static const QString POS_LAT = QString("+N") + translate("gettextFromC", "N");
	static const QString NEG_LAT = QString("-S") + translate("gettextFromC", "S");
	static const QString POS_LON = QString("+E") + translate("gettextFromC", "E");
	static const QString NEG_LON = QString("-W") + translate("gettextFromC", "W");

	//remove the useless spaces (but keep the ones separating numbers)
	static const QRegExp SPACE_CLEANER("\\s*([" + POS_LAT + NEG_LAT + POS_LON +
		NEG_LON + DEGREE_SIGNS + "'\"\\s])\\s*");
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
	double latitude, longitude;
	int latudeg, longudeg;
	bool ignore;
	bool *parsed = parsed_out ?: &ignore;
	*parsed = true;

	/* if we have a master and the dive's gps address is different from it,
	 * don't change the dive */
	if (master && (master->latitude.udeg != dive->latitude.udeg ||
		       master->longitude.udeg != dive->longitude.udeg))
		return false;

	if (!(*parsed = parseGpsText(gps_text, &latitude, &longitude)))
		return false;

	latudeg = lrint(1000000 * latitude);
	longudeg = lrint(1000000 * longitude);

	/* if dive gps didn't change, nothing changed */
	if (dive->latitude.udeg == latudeg && dive->longitude.udeg == longudeg)
		return false;
	/* ok, update the dive and mark things changed */
	dive->latitude.udeg = latudeg;
	dive->longitude.udeg = longudeg;
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

// we need this to be uniq, but also make sure
// it doesn't change during the life time of a Subsurface session
// oh, and it has no meaning whatsoever - that's why we have the
// silly initial number and increment by 3 :-)
int dive_getUniqID(struct dive *d)
{
	static QSet<int> ids;
	static int maxId = 83529;

	int id = d->id;
	if (id) {
		if (!ids.contains(id)) {
			qDebug() << "WTF - only I am allowed to create IDs";
			ids.insert(id);
		}
		return id;
	}
	maxId += 3;
	id = maxId;
	Q_ASSERT(!ids.contains(id));
	ids.insert(id);
	return id;
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


extern "C" timestamp_t picture_get_timestamp(const char *filename)
{
	easyexif::EXIFInfo exif;
	memblock mem;
	int retval;

	// filename might not be the actual filename, so let's go via the hash.
	if (readfile(localFilePath(QString(filename)).toUtf8().data(), &mem) <= 0)
		return 0;
	retval = exif.parseFrom((const unsigned char *)mem.buffer, (unsigned)mem.size);
	free(mem.buffer);
	if (retval != PARSE_EXIF_SUCCESS)
		return 0;
	return exif.epoch();
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
	return strdup(qPrintable(newPath));
}

extern "C" char *get_file_name(const char *fileName)
{
	QFileInfo fileInfo(fileName);
	return strdup(fileInfo.fileName().toUtf8());
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

extern "C" bool string_sequence_contains(const char *string_sequence, const char *text)
{
	if (same_string(text, "") || same_string(string_sequence, ""))
		return false;

	QString stringSequence(string_sequence);
	QStringList strings = stringSequence.split(",", QString::SkipEmptyParts);
	Q_FOREACH (const QString& string, strings) {
		if (string.trimmed().compare(QString(text).trimmed(), Qt::CaseInsensitive) == 0)
			return true;
	}
	return false;
}

static bool lessThan(const QPair<QString, int> &a, const QPair<QString, int> &b)
{
	return a.second < b.second;
}

void selectedDivesGasUsed(QVector<QPair<QString, int> > &gasUsedOrdered)
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
				QString gasName = gasname(&d->cylinder[j].gasmix);
				gasUsed[gasName] += diveGases[j].mliter;
			}
	}
	Q_FOREACH(const QString& gas, gasUsed.keys()) {
		gasUsedOrdered.append(qMakePair(gas, gasUsed[gas]));
	}
	qSort(gasUsedOrdered.begin(), gasUsedOrdered.end(), lessThan);
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

	return strdup(qPrintable(uA));
}

/* TOOD: Move this to SettingsObjectWrapper, and also fix this complexity.
 * gezus.
 */
QString uiLanguage(QLocale *callerLoc)
{
	QString shortDateFormat;
	QString dateFormat;
	QString timeFormat;
	QSettings s;
	QVariant v;
	s.beginGroup("Language");
	GET_BOOL("UseSystemLanguage", locale.use_system_language);

	if (!prefs.locale.use_system_language) {
		loc = QLocale(s.value("UiLangLocale", QLocale().uiLanguages().first()).toString());
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
	prefs.locale.lang_locale = copy_string(qPrintable(uiLang));
	GET_BOOL("time_format_override", time_format_override);
	GET_BOOL("date_format_override", date_format_override);
	GET_TXT("time_format", time_format);
	GET_TXT("date_format", date_format);
	GET_TXT("date_format_short", date_format_short);
	s.endGroup();

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

	if (!prefs.date_format_override || same_string(prefs.date_format_short, "") || same_string(prefs.date_format, "")) {
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
		if (!prefs.date_format_override || same_string(prefs.date_format, "")) {
			free((void*)prefs.date_format);
			prefs.date_format = strdup(qPrintable(dateFormat));
		}
		if (!prefs.date_format_override || same_string(prefs.date_format_short, "")) {
			free((void*)prefs.date_format_short);
			prefs.date_format_short = strdup(qPrintable(shortDateFormat));
		}
	}
	if (!prefs.time_format_override || same_string(prefs.time_format, "")) {
		timeFormat = loc.timeFormat();
		timeFormat.replace("(t)", "").replace(" t", "").replace("t", "").replace("hh", "h").replace("HH", "H").replace("'kl'.", "");
		timeFormat.replace(".ss", "").replace(":ss", "").replace("ss", "");
		free((void*)prefs.time_format);
		prefs.time_format = strdup(qPrintable(timeFormat));
	}
	return uiLang;
}

QLocale getLocale()
{
	return loc;
}

void set_filename(const char *filename, bool force)
{
	if (!force && existing_filename)
		return;
	free((void *)existing_filename);
	if (filename)
		existing_filename = strdup(filename);
	else
		existing_filename = NULL;
}

const QString get_dc_nickname(const char *model, uint32_t deviceid)
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
		return QString("%1%2").arg(meters, 0, 'f', (showdecimal && meters < 20.0) ? 1 : 0).arg(showunit ? translate("gettextFromC", "m") : "");
	} else {
		double feet = mm_to_feet(mm);
		return QString("%1%2").arg(feet, 0, 'f', 0).arg(showunit ? translate("gettextFromC", "ft") : "");
	}
}

QString get_depth_string(depth_t depth, bool showunit, bool showdecimal)
{
	return get_depth_string(depth.mm, showunit, showdecimal);
}

QString get_depth_unit()
{
	if (prefs.units.length == units::METERS)
		return QString("%1").arg(translate("gettextFromC", "m"));
	else
		return QString("%1").arg(translate("gettextFromC", "ft"));
}

QString get_weight_string(weight_t weight, bool showunit)
{
	QString str = weight_string(weight.grams);
	if (get_units()->weight == units::KG) {
		str = QString("%1%2").arg(str).arg(showunit ? translate("gettextFromC", "kg") : "");
	} else {
		str = QString("%1%2").arg(str).arg(showunit ? translate("gettextFromC", "lbs") : "");
	}
	return (str);
}

QString get_weight_unit()
{
	if (prefs.units.weight == units::KG)
		return QString("%1").arg(translate("gettextFromC", "kg"));
	else
		return QString("%1").arg(translate("gettextFromC", "lbs"));
}

QString get_temperature_string(temperature_t temp, bool showunit)
{
	if (temp.mkelvin == 0) {
		return ""; //temperature not defined
	} else if (prefs.units.temperature == units::CELSIUS) {
		double celsius = mkelvin_to_C(temp.mkelvin);
		return QString("%1%2%3").arg(celsius, 0, 'f', 1).arg(showunit ? (UTF8_DEGREE) : "").arg(showunit ? translate("gettextFromC", "C") : "");
	} else {
		double fahrenheit = mkelvin_to_F(temp.mkelvin);
		return QString("%1%2%3").arg(fahrenheit, 0, 'f', 1).arg(showunit ? (UTF8_DEGREE) : "").arg(showunit ? translate("gettextFromC", "F") : "");
	}
}

QString get_temp_unit()
{
	if (prefs.units.temperature == units::CELSIUS)
		return QString(UTF8_DEGREE "C");
	else
		return QString(UTF8_DEGREE "F");
}

QString get_volume_string(volume_t volume, bool showunit)
{
	const char *unit;
	int decimals;
	double value = get_volume_units(volume.mliter, &decimals, &unit);
	return QString("%1%2").arg(value, 0, 'f', decimals).arg(showunit ? unit : "");
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
		return QString("%1%2").arg(bar, 0, 'f', 1).arg(showunit ? translate("gettextFromC", "bar") : "");
	} else {
		double psi = mbar_to_PSI(pressure.mbar);
		return QString("%1%2").arg(psi, 0, 'f', 0).arg(showunit ? translate("gettextFromC", "psi") : "");
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
	return QString("");
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

void copyPath(QString src, QString dst)
{
	QDir dir(src);
	if (!dir.exists())
		return;
	foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		QString dst_path = dst + QDir::separator() + d;
		dir.mkpath(dst_path);
		copyPath(src + QDir::separator() + d, dst_path);
	}
	foreach (QString f, dir.entryList(QDir::Files))
		QFile::copy(src + QDir::separator() + f, dst + QDir::separator() + f);
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
	if (text.contains(QObject::tr("m"), Qt::CaseInsensitive)) {
		mm = lrint(number * 1000);
	} else if (text.contains(QObject::tr("ft"), Qt::CaseInsensitive)) {
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
	if (text.contains(QObject::tr("C"), Qt::CaseInsensitive)) {
		mkelvin = C_to_mkelvin(number);
	} else if (text.contains(QObject::tr("F"), Qt::CaseInsensitive)) {
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
	if (text.contains(QObject::tr("kg"), Qt::CaseInsensitive)) {
		grams = lrint(number * 1000);
	} else if (text.contains(QObject::tr("lbs"), Qt::CaseInsensitive)) {
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
	if (text.contains(QObject::tr("bar"), Qt::CaseInsensitive)) {
		mbar = lrint(number * 1000);
	} else if (text.contains(QObject::tr("psi"), Qt::CaseInsensitive)) {
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
	if (gasString.contains(QObject::tr("AIR"), Qt::CaseInsensitive)) {
		o2 = O2_IN_AIR;
	} else if (gasString.contains(QObject::tr("EAN"), Qt::CaseInsensitive)) {
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

QString get_dive_duration_string(timestamp_t when, QString hourText, QString minutesText)
{
	int hrs, mins;
	mins = (when + 59) / 60;
	hrs = mins / 60;
	mins -= hrs * 60;

	QString displayTime;
	if (hrs)
		displayTime = QString("%1%2%3%4").arg(hrs).arg(hourText).arg(mins, 2, 10, QChar('0')).arg(minutesText);
	else
		displayTime = QString("%1%2").arg(mins).arg(minutesText);

	return displayTime;
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

const char *get_dive_date_c_string(timestamp_t when)
{
	QString text = get_dive_date_string(when);
	return strdup(text.toUtf8().data());
}

extern "C" const char *get_current_date()
{
	QDateTime ts(QDateTime::currentDateTime());;
	QString current_date;
	
	current_date = loc.toString(ts, QString(prefs.date_format_short));

	return strdup(current_date.toUtf8().data());
}

bool is_same_day(timestamp_t trip_when, timestamp_t dive_when)
{
	static timestamp_t twhen = (timestamp_t) 0;
	static struct tm tmt;
	struct tm tmd;

	utc_mkdate(dive_when, &tmd);

	if (twhen != trip_when) {
		twhen = trip_when;
		utc_mkdate(twhen, &tmt);
	}

	return ((tmd.tm_mday == tmt.tm_mday) && (tmd.tm_mon == tmt.tm_mon) && (tmd.tm_year == tmt.tm_year));
}

QString get_trip_date_string(timestamp_t when, int nr, bool getday)
{
	struct tm tm;
	utc_mkdate(when, &tm);
	QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000*when,Qt::UTC);
	localTime.setTimeSpec(Qt::UTC);
	QString ret ;

	QString suffix = " " + QObject::tr("(%n dive(s))", "", nr);
	if (getday) {
		ret = localTime.date().toString(prefs.date_format) + suffix;
	} else {
		ret = localTime.date().toString("MMM yyyy") + suffix;
	}
	return ret;

}

extern "C" void reverseGeoLookup(degrees_t latitude, degrees_t longitude, uint32_t uuid)
{
	QNetworkRequest request;
	QNetworkAccessManager *rgl = new QNetworkAccessManager();
	request.setUrl(QString("http://open.mapquestapi.com/nominatim/v1/reverse.php?format=json&accept-language=%1&lat=%2&lon=%3")
		       .arg(uiLanguage(NULL)).arg(latitude.udeg / 1000000.0).arg(longitude.udeg / 1000000.0));
	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QNetworkReply *reply = rgl->get(request);
	QEventLoop loop;
	QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();
	QJsonParseError errorObject;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &errorObject);
	if (errorObject.error != QJsonParseError::NoError) {
		qDebug() << errorObject.errorString();
	} else {
		QJsonObject obj = jsonDoc.object();
		QJsonObject address = obj.value("address").toObject();
		qDebug() << "found country:" << address.value("country").toString();
		struct dive_site *ds = get_dive_site_by_uuid(uuid);
		ds->notes = add_to_string(ds->notes, "countrytag: %s", address.value("country").toString().toUtf8().data());
	}
}

QHash<QString, QByteArray> hashOf;
QMutex hashOfMutex;
QHash<QByteArray, QString> localFilenameOf;
QHash <QString, QImage > thumbnailCache;

extern "C" char * hashstring(const char *filename)
{
	QMutexLocker locker(&hashOfMutex);
	return hashOf[QString(filename)].toHex().data();
}

const QString hashfile_name()
{
	return QString(system_default_directory()).append("/hashes");
}

extern "C" char *hashfile_name_string()
{
	return strdup(hashfile_name().toUtf8().data());
}

void read_hashes()
{
	QFile hashfile(hashfile_name());
	QMutexLocker locker(&hashOfMutex);
	if (hashfile.open(QIODevice::ReadOnly)) {
		QDataStream stream(&hashfile);
		stream >> localFilenameOf;
		stream >> hashOf;
		stream >> thumbnailCache;
		hashfile.close();
	}
	localFilenameOf.remove("");
	QMutableHashIterator<QString, QByteArray> iter(hashOf);
	while (iter.hasNext()) {
		iter.next();
		if (iter.value().isEmpty())
			iter.remove();
	}
}

void write_hashes()
{
	QSaveFile hashfile(hashfile_name());
	QMutexLocker locker(&hashOfMutex);

	if (hashfile.open(QIODevice::WriteOnly)) {
		QDataStream stream(&hashfile);
		stream << localFilenameOf;
		stream << hashOf;
		stream << thumbnailCache;
		hashfile.commit();
	} else {
		qDebug() << "cannot open" << hashfile.fileName();
	}
}

void add_hash(const QString filename, QByteArray hash)
{
	if (hash.isEmpty())
		return;
	QMutexLocker locker(&hashOfMutex);
	hashOf[filename] =  hash;
	localFilenameOf[hash] = filename;
}

QByteArray hashFile(const QString filename)
{
	QCryptographicHash hash(QCryptographicHash::Sha1);
	QFile imagefile(filename);
	if (imagefile.exists() && imagefile.open(QIODevice::ReadOnly)) {
		hash.addData(&imagefile);
		add_hash(filename, hash.result());
		return hash.result();
	} else {
		return QByteArray();
	}
}

void learnHash(struct picture *picture, QByteArray hash)
{
	if (hash.isNull())
		return;
	if (picture->hash)
		free(picture->hash);
	QMutexLocker locker(&hashOfMutex);
	hashOf[QString(picture->filename)] = hash;
	picture->hash = strdup(hash.toHex());
}

bool haveHash(const QString &filename)
{
	QMutexLocker locker(&hashOfMutex);
	return hashOf.contains(filename);
}

QString localFilePath(const QString originalFilename)
{
	QMutexLocker locker(&hashOfMutex);

	if (hashOf.contains(originalFilename) && localFilenameOf.contains(hashOf[originalFilename]))
		return localFilenameOf[hashOf[originalFilename]];
	else
		return originalFilename;
}

QString fileFromHash(const char *hash)
{
	if (!hash || !*hash)
		return "";
	QMutexLocker locker(&hashOfMutex);

	return localFilenameOf[QByteArray::fromHex(hash)];
}

// This needs to operate on a copy of picture as it frees it after finishing!
void updateHash(struct picture *picture) {
	if (!picture)
		return;
	QByteArray hash = hashFile(fileFromHash(picture->hash));
	learnHash(picture, hash);
	picture_free(picture);
}

// This needs to operate on a copy of picture as it frees it after finishing!
void hashPicture(struct picture *picture)
{
	if (!picture)
		return;
	char *oldHash = copy_string(picture->hash);
	learnHash(picture, hashFile(localFilePath(picture->filename)));
	if (!same_string(picture->hash, "") && !same_string(picture->hash, oldHash))
		mark_divelist_changed((true));
	free(oldHash);
	picture_free(picture);
}

extern "C" void cache_picture(struct picture *picture)
{
	QString filename = picture->filename;
	if (!haveHash(filename))
		QtConcurrent::run(hashPicture, clone_picture(picture));
}

void learnImages(const QDir dir, int max_recursions)
{
	QStringList filters, files;

	if (max_recursions) {
		foreach (QString dirname, dir.entryList(QStringList(), QDir::NoDotAndDotDot | QDir::Dirs)) {
			learnImages(QDir(dir.filePath(dirname)), max_recursions - 1);
		}
	}

	foreach (QString format, QImageReader::supportedImageFormats()) {
		filters.append(QString("*.").append(format));
	}

	foreach (QString file, dir.entryList(filters, QDir::Files)) {
		files.append(dir.absoluteFilePath(file));
	}

	QtConcurrent::blockingMap(files, hashFile);
}

extern "C" const char *local_file_path(struct picture *picture)
{
	QString hashString = picture->hash;
	if (hashString.isEmpty()) {
		QByteArray hash = hashFile(picture->filename);
		free(picture->hash);
		picture->hash = strdup(hash.toHex().data());
	}
	QString localFileName = fileFromHash(picture->hash);
	if (localFileName.isEmpty())
		localFileName = picture->filename;
	return strdup(qPrintable(localFileName));
}

extern "C" bool picture_exists(struct picture *picture)
{
	QString localFilename = fileFromHash(picture->hash);
	QByteArray hash = hashFile(localFilename);
	return same_string(hash.toHex().data(), picture->hash);
}

const QString picturedir()
{
	return QString(system_default_directory()).append("/picturedata/");
}

extern "C" char *picturedir_string()
{
	return strdup(picturedir().toUtf8().data());
}

/* when we get a picture from git storage (local or remote) and can't find the picture
 * based on its hash, we create a local copy with the hash as filename and the appropriate
 * suffix */
extern "C" void savePictureLocal(struct picture *picture, const char *data, int len)
{
	QString dirname = picturedir();
	QDir localPictureDir(dirname);
	localPictureDir.mkpath(dirname);
	QString suffix(picture->filename);
	suffix.replace(QRegularExpression(".*\\."), "");
	QString filename(dirname + picture->hash + "." + suffix);
	QSaveFile out(filename);
	if (out.open(QIODevice::WriteOnly)) {
		out.write(data, len);
		out.commit();
		add_hash(filename, QByteArray::fromHex(picture->hash));
	}
}

extern "C" void picture_load_exif_data(struct picture *p)
{
	easyexif::EXIFInfo exif;
	memblock mem;

	if (readfile(localFilePath(QString(p->filename)).toUtf8().data(), &mem) <= 0)
		goto picture_load_exit;
	if (exif.parseFrom((const unsigned char *)mem.buffer, (unsigned)mem.size) != PARSE_EXIF_SUCCESS)
		goto picture_load_exit;
	p->longitude.udeg= llrint(1000000.0 * exif.GeoLocation.Longitude);
	p->latitude.udeg  = llrint(1000000.0 * exif.GeoLocation.Latitude);

picture_load_exit:
	free(mem.buffer);
	return;
}

QString get_gas_string(struct gasmix gas)
{
	uint o2 = (get_o2(&gas) + 5) / 10, he = (get_he(&gas) + 5) / 10;
	QString result = gasmix_is_air(&gas) ? QObject::tr("AIR") : he == 0 ? (o2 == 100 ? QObject::tr("OXYGEN") : QString("EAN%1").arg(o2, 2, 10, QChar('0'))) : QString("%1/%2").arg(o2).arg(he);
	return result;
}

QString get_divepoint_gas_string(struct dive *d, const divedatapoint &p)
{
	int idx = p.cylinderid;
	return get_gas_string(d->cylinder[idx].gasmix);
}

weight_t string_to_weight(const char *str)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
	QString rest = QString(end).trimmed();
	QString local_kg = QObject::tr("kg");
	QString local_lbs = QObject::tr("lbs");
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
	QString local_ft = QObject::tr("ft");
	QString local_m = QObject::tr("m");
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
	QString local_psi = QObject::tr("psi");
	QString local_bar = QObject::tr("bar");
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
	QString local_l = QObject::tr("l");
	QString local_cuft = QObject::tr("cuft");
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
	if (email.isEmpty() || same_string(prefs.cloud_storage_password, ""))
		return report_error("Please configure Cloud storage email and password in the preferences");
	if (email != prefs.cloud_storage_email_encoded) {
		free(prefs.cloud_storage_email_encoded);
		prefs.cloud_storage_email_encoded = strdup(qPrintable(email));
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
	return strdup(filename.toUtf8().data());
}

extern "C" bool isCloudUrl(const char *filename)
{
	QString email = QString(prefs.cloud_storage_email);
	email.replace(QRegularExpression("[^a-zA-Z0-9@._+-]"), "");
	if (!email.isEmpty() &&
	    QString(QString(prefs.cloud_git_url) + "/%1[%1]").arg(email) == filename)
		return true;
	return false;
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
			*buffer = strdup(qPrintable(proxy));
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

static QByteArray currentApplicationState;

QByteArray getCurrentAppState()
{
	return currentApplicationState;
}

void setCurrentAppState(QByteArray state)
{
	currentApplicationState = state;
}

extern "C" bool in_planner()
{
	return (currentApplicationState == "PlanDive" || currentApplicationState == "EditPlannedDive");
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
	// This is a correct usage of QSettings,
	// it's not a setting per se - the user cannot change it
	// and thus, don't need to be on the prefs structure
	// and this is the *only* point of access from it,
	QString uuidString;
	QSettings settings;
	settings.beginGroup("UpdateManager");
	if (settings.contains("UUID")) {
		uuidString = settings.value("UUID").toString();
	} else {
		QUuid uuid = QUuid::createUuid();
		uuidString = uuid.toString();
		settings.setValue("UUID", uuidString);
	}
	uuidString.replace("{", "").replace("}", "");
	return uuidString;
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
			qDebug() << "DEBGU: " << parseLine;
			params[pnr++] = strdup("diveNro");
			params[pnr++] = strdup(parseLine.replace(QString::fromLatin1("//DIVE NR: "), QString::fromLatin1("")).toUtf8().data());
			break;
		}
	}

	/*
	 * Parse header - currently only interested in sample
	 * interval and hardware version. If we have old format
	 * the interval value is missing from the header.
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		if (parseLine.contains("//Hardware Version: ")) {
			params[pnr++] = strdup("hw");
			params[pnr++] = strdup(parseLine.replace(QString::fromLatin1("//Hardware Version: "), QString::fromLatin1("\"Seabear ")).trimmed().append("\"").toUtf8().data());
			break;
		}
	}

	/*
	 * Note that we scan over the "Log interval" on purpose
	 */

	while ((parseLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
		if (parseLine.contains("//Log interval: ")) {
			params[pnr++] = strdup("delta");
			params[pnr++] = strdup(parseLine.remove(QString::fromLatin1("//Log interval: ")).trimmed().remove(QString::fromLatin1(" s")).toUtf8().data());
		}
	}

	/*
	 * Parse CSV fields
	 */

	parseLine = f.readLine().trimmed();

	QStringList currColumns = parseLine.split(';');
	unsigned short index = 0;
	Q_FOREACH (QString columnText, currColumns) {
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
