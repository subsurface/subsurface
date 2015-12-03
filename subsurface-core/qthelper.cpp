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
#include <exif.h>
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

#define EMPTY_DIVE_STRING "--"

Dive::Dive() :
	m_number(-1),
	dive(NULL)
{
}

Dive::~Dive()
{
}

int Dive::number() const
{
	return m_number;
}

int Dive::id() const
{
	return m_id;
}

QString Dive::date() const
{
	return m_date;
}

timestamp_t Dive::timestamp() const
{
	return m_timestamp;
}

QString Dive::time() const
{
	return m_time;
}

QString Dive::location() const
{
	return m_location;
}

QString Dive::duration() const
{
	return m_duration;
}

QString Dive::depth() const
{
	return m_depth;
}

QString Dive::divemaster() const
{
	return m_divemaster;
}

QString Dive::buddy() const
{
	return m_buddy;
}

QString Dive::airTemp() const
{
	return m_airTemp;
}

QString Dive::waterTemp() const
{
	return m_waterTemp;
}

QString Dive::notes() const
{
	return m_notes;
}

QString Dive::tags() const
{
	return m_tags;
}

QString Dive::gas() const
{
	return m_gas;
}

QString Dive::sac() const
{
	return m_sac;
}

QString Dive::weights() const
{
	QString str = "";
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		QString entry = m_weights.at(i);
		if (entry == EMPTY_DIVE_STRING)
			continue;
		str += QObject::tr("Weight %1: ").arg(i + 1) + entry + "; ";
	}
	return str;
}

QString Dive::weight(int idx) const
{
	if (idx < 0 || idx > m_weights.size() - 1)
		return QString(EMPTY_DIVE_STRING);
	return m_weights.at(idx);
}

QString Dive::suit() const
{
	return m_suit;
}

QString Dive::cylinders() const
{
	QString str = "";
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		QString entry = m_cylinders.at(i);
		if (entry == EMPTY_DIVE_STRING)
			continue;
		str += QObject::tr("Cylinder %1: ").arg(i + 1) + entry + "; ";
	}
	return str;
}

QString Dive::cylinder(int idx) const
{
	if (idx < 0 || idx > m_cylinders.size() - 1)
		return QString(EMPTY_DIVE_STRING);
	return m_cylinders.at(idx);
}

QString Dive::trip() const
{
	return m_trip;
}

QString Dive::maxcns() const
{
	return m_maxcns;
}

QString Dive::otu() const
{
	return m_otu;
}

int Dive::rating() const
{
	return m_rating;
}

void Dive::put_divemaster()
{
	if (!dive->divemaster)
		m_divemaster = EMPTY_DIVE_STRING;
	else
		m_divemaster = dive->divemaster;
}

void Dive::put_date_time()
{
	QDateTime localTime = QDateTime::fromTime_t(dive->when - gettimezoneoffset(displayed_dive.when));
	localTime.setTimeSpec(Qt::UTC);
	m_date = localTime.date().toString(prefs.date_format);
	m_time = localTime.time().toString(prefs.time_format);
}

void Dive::put_timestamp()
{
	m_timestamp = dive->when;
}

void Dive::put_location()
{
	m_location = QString::fromUtf8(get_dive_location(dive));
	if (m_location.isEmpty()) {
		m_location = EMPTY_DIVE_STRING;
	}
}

void Dive::put_depth()
{
	m_depth = get_depth_string(dive->dc.maxdepth.mm, true, true);
}

void Dive::put_duration()
{
	m_duration = get_dive_duration_string(dive->duration.seconds, QObject::tr("h:"), QObject::tr("min"));
}

void Dive::put_buddy()
{
	if (!dive->buddy)
		m_buddy = EMPTY_DIVE_STRING;
	else
		m_buddy = dive->buddy;
}

void Dive::put_temp()
{
	m_airTemp = get_temperature_string(dive->airtemp, true);
	m_waterTemp = get_temperature_string(dive->watertemp, true);
	if (m_airTemp.isEmpty()) {
		m_airTemp = EMPTY_DIVE_STRING;
	}
	if (m_waterTemp.isEmpty()) {
		m_waterTemp = EMPTY_DIVE_STRING;
	}
}

void Dive::put_notes()
{
	m_notes = QString::fromUtf8(dive->notes);
	if (m_notes.isEmpty()) {
		m_notes = EMPTY_DIVE_STRING;
		return;
	}
	if (same_string(dive->dc.model, "planned dive")) {
		QTextDocument notes;
		QString notesFormatted = m_notes;
#define _NOTES_BR "&#92n"
		notesFormatted = notesFormatted.replace("<thead>", "<thead>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("<br>", "<br>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("<tr>", "<tr>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("</tr>", "</tr>" _NOTES_BR);
		notes.setHtml(notesFormatted);
		m_notes = notes.toPlainText();
		m_notes.replace(_NOTES_BR, "<br>");
#undef _NOTES_BR
	} else {
		m_notes.replace("\n", "<br>");
	}
}

void Dive::put_tags()
{
	char buffer[256];
	taglist_get_tagstring(dive->tag_list, buffer, 256);
	m_tags = QString(buffer);
}

void Dive::put_gas()
{
	int added = 0;
	QString gas, gases;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!is_cylinder_used(dive, i))
			continue;
		gas = dive->cylinder[i].type.description;
		gas += QString(!gas.isEmpty() ? " " : "") + gasname(&dive->cylinder[i].gasmix);
		// if has a description and if such gas is not already present
		if (!gas.isEmpty() && gases.indexOf(gas) == -1) {
			if (added > 0)
				gases += QString(" / ");
			gases += gas;
			added++;
		}
	}
	m_gas = gases;
}

void Dive::put_sac()
{
	if (dive->sac) {
		const char *unit;
		int decimal;
		double value = get_volume_units(dive->sac, &decimal, &unit);
		m_sac = QString::number(value, 'f', decimal).append(unit);
	}
}

static QString getFormattedWeight(struct dive *dive, unsigned int idx)
{
	weightsystem_t *weight = &dive->weightsystem[idx];
	if (!weight->description)
		return QString(EMPTY_DIVE_STRING);
	QString fmt = QString(weight->description);
	fmt += ", " + get_weight_string(weight->weight, true);
	return fmt;
}

void Dive::put_weight()
{
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		m_weights << getFormattedWeight(dive, i);
}

void Dive::put_suit()
{
	m_suit = QString(dive->suit);
}

static QString getFormattedCylinder(struct dive *dive, unsigned int idx)
{
	cylinder_t *cyl = &dive->cylinder[idx];
	const char *desc = cyl->type.description;
	if (!desc && idx > 0)
		return QString(EMPTY_DIVE_STRING);
	QString fmt = desc ? QString(desc) : QObject::tr("unknown");
	fmt += ", " + get_volume_string(cyl->type.size, true, 0);
	fmt += ", " + get_pressure_string(cyl->type.workingpressure, true);
	fmt += ", " + get_pressure_string(cyl->start, false) + " - " + get_pressure_string(cyl->end, true);
	fmt += ", " + get_gas_string(cyl->gasmix);
	return fmt;
}

void Dive::put_cylinder()
{
	for (int i = 0; i < MAX_CYLINDERS; i++)
		m_cylinders << getFormattedCylinder(dive, i);
}

void Dive::put_trip()
{
	dive_trip *trip = dive->divetrip;
	if (trip) {
		m_trip = QString(trip->location);
	}
}

void Dive::put_maxcns()
{
	m_maxcns = QString::number(dive->maxcns);
}

void Dive::put_otu()
{
	m_otu = QString::number(dive->otu);
}

QString weight_string(int weight_in_grams)
{
	QString str;
	if (get_units()->weight == units::KG) {
		int gr = weight_in_grams % 1000;
		int kg = weight_in_grams / 1000;
		if (kg >= 20.0) {
			str = QString("0");
		} else {
			str = QString("%1.%2").arg(kg).arg((unsigned)(gr) / 100);
		}
	} else {
		double lbs = grams_to_lbs(weight_in_grams);
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
			while (txt[++pos].isSpace())
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

	latudeg = rint(1000000 * latitude);
	longudeg = rint(1000000 * longitude);

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
			qDebug() << "cannot open stylesheet" << QLatin1String(":/xslt/") + (const char *)uri;
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


extern "C" timestamp_t picture_get_timestamp(char *filename)
{
	EXIFInfo exif;
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
	QString userAgent = QString("Subsurface-mobile:%1:").arg(subsurface_version());
#else
	QString userAgent = QString("Subsurface:%1:").arg(subsurface_version());
#endif
	userAgent.append(SubsurfaceSysInfo::prettyOsName().replace(':', ' ') + ":");
	arch = SubsurfaceSysInfo::buildCpuArchitecture().replace(':', ' ');
	userAgent.append(arch);
	if (arch == "i386")
		userAgent.append("/" + SubsurfaceSysInfo::currentCpuArchitecture());
	userAgent.append(":" + uiLanguage(NULL));
	return userAgent;

}

QString uiLanguage(QLocale *callerLoc)
{
	QString shortDateFormat;
	QString dateFormat;
	QString timeFormat;
	QSettings s;
	QVariant v;
	s.beginGroup("Language");

	if (!s.value("UseSystemLanguage", true).toBool()) {
		loc = QLocale(s.value("UiLanguage", QLocale().uiLanguages().first()).toString());
	} else {
		loc = QLocale(QLocale().uiLanguages().first());
	}

	QString uiLang = loc.uiLanguages().first();
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
		uiLang = loc2.uiLanguages().first();
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

/* these methods retrieve used gas per cylinder */
static unsigned start_pressure(cylinder_t *cyl)
{
	return cyl->start.mbar ?: cyl->sample_start.mbar;
}

static unsigned end_pressure(cylinder_t *cyl)
{
	return cyl->end.mbar ?: cyl->sample_end.mbar;
}

QString get_cylinder_used_gas_string(cylinder_t *cyl, bool showunit)
{
	int decimals;
	const char *unit;
	double gas_usage;
	/* Get the cylinder gas use in mbar */
	gas_usage = start_pressure(cyl) - end_pressure(cyl);
	/* Can we turn it into a volume? */
	if (cyl->type.size.mliter) {
		gas_usage = bar_to_atm(gas_usage / 1000);
		gas_usage *= cyl->type.size.mliter;
		gas_usage = get_volume_units(gas_usage, &decimals, &unit);
	} else {
		gas_usage = get_pressure_units(gas_usage, &unit);
		decimals = 0;
	}
	// translate("gettextFromC","%.*f %s"
	return QString("%1 %2").arg(gas_usage, 0, 'f', decimals).arg(showunit ? unit : "");
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

QString get_volume_string(volume_t volume, bool showunit, int mbar)
{
	const char *unit;
	int decimals;
	double value = get_volume_units(volume.mliter, &decimals, &unit);
	if (mbar) {
		// we are showing a tank size
		// fix the weird imperial way of denominating size and provide
		// reasonable number of decimals
		if (prefs.units.volume == units::CUFT)
			value *= bar_to_atm(mbar / 1000.0);
		decimals = (value > 20.0) ? 0 : (value > 2.0) ? 1 : 2;
	}
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

int parseTemperatureToMkelvin(const QString &text)
{
	int mkelvin;
	QString numOnly = text;
	numOnly.replace(",", ".").remove(QRegExp("[^-0-9.]"));
	if (numOnly.isEmpty())
		return 0;
	double number = numOnly.toDouble();
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
	return mkelvin;
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
	QDateTime localTime = QDateTime::fromTime_t(when);
	localTime.setTimeSpec(Qt::UTC);
	QString ret ;

	QString suffix = " " + QObject::tr("(%n dive(s))", "", nr);
	if (getday) {
		ret = localTime.date().toString(prefs.date_format) + suffix;
	} else {
		ret = localTime.date().toString("MMM yy") + suffix;
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

extern "C" char * hashstring(char * filename)
{
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
	if (hashfile.open(QIODevice::ReadOnly)) {
		QDataStream stream(&hashfile);
		stream >> localFilenameOf;
		stream >> hashOf;
		stream >> thumbnailCache;
		hashfile.close();
	}
}

void write_hashes()
{
	QSaveFile hashfile(hashfile_name());
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
	if (picture->hash)
		free(picture->hash);
	QMutexLocker locker(&hashOfMutex);
	hashOf[QString(picture->filename)] = hash;
	picture->hash = strdup(hash.toHex());
}

QString localFilePath(const QString originalFilename)
{
	if (hashOf.contains(originalFilename) && localFilenameOf.contains(hashOf[originalFilename]))
		return localFilenameOf[hashOf[originalFilename]];
	else
		return originalFilename;
}

QString fileFromHash(char *hash)
{
	return localFilenameOf[QByteArray::fromHex(hash)];
}

void updateHash(struct picture *picture) {
	QByteArray hash = hashFile(fileFromHash(picture->hash));
	QMutexLocker locker(&hashOfMutex);
	hashOf[QString(picture->filename)] = hash;
	char *old = picture->hash;
	picture->hash = strdup(hash.toHex());
	free(old);
}

void hashPicture(struct picture *picture)
{
	char *oldHash = copy_string(picture->hash);
	learnHash(picture, hashFile(QString(picture->filename)));
	if (!same_string(picture->hash, "") && !same_string(picture->hash, oldHash))
		mark_divelist_changed((true));
	free(oldHash);
}

extern "C" void cache_picture(struct picture *picture)
{
	QString filename = picture->filename;
	if (!hashOf.contains(filename))
		QtConcurrent::run(hashPicture, picture);
}

void learnImages(const QDir dir, int max_recursions, bool recursed)
{
	QDir current(dir);
	QStringList filters, files;

	if (max_recursions) {
		foreach (QString dirname, dir.entryList(QStringList(), QDir::NoDotAndDotDot | QDir::Dirs)) {
			learnImages(QDir(dir.filePath(dirname)), max_recursions - 1, true);
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
	EXIFInfo exif;
	memblock mem;

	if (readfile(localFilePath(QString(p->filename)).toUtf8().data(), &mem) <= 0)
		goto picture_load_exit;
	if (exif.parseFrom((const unsigned char *)mem.buffer, (unsigned)mem.size) != PARSE_EXIF_SUCCESS)
		goto picture_load_exit;
	p->longitude.udeg= lrint(1000000.0 * exif.GeoLocation.Longitude);
	p->latitude.udeg  = lrint(1000000.0 * exif.GeoLocation.Latitude);

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

QString get_divepoint_gas_string(const divedatapoint &p)
{
	return get_gas_string(p.gasmix);
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
	weight.grams = rint(value * 1000);
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

	if (rest.startsWith("m") || rest.startsWith(local_m))
		goto m;
	if (rest.startsWith("ft") || rest.startsWith(local_ft))
		goto ft;
	if (prefs.units.length == prefs.units.FEET)
		goto ft;
m:
	depth.mm = rint(value * 1000);
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
	pressure.mbar = rint(value * 1000);
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
	volume.mliter = rint(value * 1000);
	return volume;
}

fraction_t string_to_fraction(const char *str)
{
	const char *end;
	double value = strtod_flags(str, &end, 0);
	fraction_t fraction;

	fraction.permille = rint(value * 10);
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

void loadPreferences()
{
	QSettings s;
	QVariant v;

	s.beginGroup("Units");
	if (s.value("unit_system").toString() == "metric") {
		prefs.unit_system = METRIC;
		prefs.units = SI_units;
	} else if (s.value("unit_system").toString() == "imperial") {
		prefs.unit_system = IMPERIAL;
		prefs.units = IMPERIAL_units;
	} else {
		prefs.unit_system = PERSONALIZE;
		GET_UNIT("length", length, units::FEET, units::METERS);
		GET_UNIT("pressure", pressure, units::PSI, units::BAR);
		GET_UNIT("volume", volume, units::CUFT, units::LITER);
		GET_UNIT("temperature", temperature, units::FAHRENHEIT, units::CELSIUS);
		GET_UNIT("weight", weight, units::LBS, units::KG);
	}
	GET_UNIT("vertical_speed_time", vertical_speed_time, units::MINUTES, units::SECONDS);
	GET_BOOL("coordinates", coordinates_traditional);
	s.endGroup();
	s.beginGroup("TecDetails");
	GET_BOOL("po2graph", pp_graphs.po2);
	GET_BOOL("pn2graph", pp_graphs.pn2);
	GET_BOOL("phegraph", pp_graphs.phe);
	GET_DOUBLE("po2threshold", pp_graphs.po2_threshold);
	GET_DOUBLE("pn2threshold", pp_graphs.pn2_threshold);
	GET_DOUBLE("phethreshold", pp_graphs.phe_threshold);
	GET_BOOL("mod", mod);
	GET_DOUBLE("modpO2", modpO2);
	GET_BOOL("ead", ead);
	GET_BOOL("redceiling", redceiling);
	GET_BOOL("dcceiling", dcceiling);
	GET_BOOL("calcceiling", calcceiling);
	GET_BOOL("calcceiling3m", calcceiling3m);
	GET_BOOL("calcndltts", calcndltts);
	GET_BOOL("calcalltissues", calcalltissues);
	GET_BOOL("hrgraph", hrgraph);
	GET_BOOL("tankbar", tankbar);
	GET_BOOL("percentagegraph", percentagegraph);
	GET_INT("gflow", gflow);
	GET_INT("gfhigh", gfhigh);
	GET_BOOL("gf_low_at_maxdepth", gf_low_at_maxdepth);
	GET_BOOL("show_ccr_setpoint",show_ccr_setpoint);
	GET_BOOL("show_ccr_sensors",show_ccr_sensors);
	GET_BOOL("zoomed_plot", zoomed_plot);
	set_gf(prefs.gflow, prefs.gfhigh, prefs.gf_low_at_maxdepth);
	GET_BOOL("show_sac", show_sac);
	GET_BOOL("display_unused_tanks", display_unused_tanks);
	GET_BOOL("show_average_depth", show_average_depth);
	s.endGroup();

	s.beginGroup("GeneralSettings");
	GET_TXT("default_filename", default_filename);
	GET_INT("default_file_behavior", default_file_behavior);
	if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		if (QString(prefs.default_filename).isEmpty())
			prefs.default_file_behavior = NO_DEFAULT_FILE;
		else
			prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
	}
	GET_TXT("default_cylinder", default_cylinder);
	GET_BOOL("use_default_file", use_default_file);
	GET_INT("defaultsetpoint", defaultsetpoint);
	GET_INT("o2consumption", o2consumption);
	GET_INT("pscr_ratio", pscr_ratio);
	s.endGroup();

	s.beginGroup("Display");
	// get the font from the settings or our defaults
	// respect the system default font size if none is explicitly set
	QFont defaultFont = s.value("divelist_font", prefs.divelist_font).value<QFont>();
	if (IS_FP_SAME(system_divelist_default_font_size, -1.0)) {
		prefs.font_size = qApp->font().pointSizeF();
		system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
	}
	prefs.font_size = s.value("font_size", prefs.font_size).toFloat();
	// painful effort to ignore previous default fonts on Windows - ridiculous
	QString fontName = defaultFont.toString();
	if (fontName.contains(","))
		fontName = fontName.left(fontName.indexOf(","));
	if (subsurface_ignore_font(fontName.toUtf8().constData())) {
		defaultFont = QFont(prefs.divelist_font);
	} else {
		free((void *)prefs.divelist_font);
		prefs.divelist_font = strdup(fontName.toUtf8().constData());
	}
	defaultFont.setPointSizeF(prefs.font_size);
	qApp->setFont(defaultFont);
	GET_INT("displayinvalid", display_invalid_dives);
	s.endGroup();

	s.beginGroup("Animations");
	GET_INT("animation_speed", animation_speed);
	s.endGroup();

	s.beginGroup("Network");
	GET_INT_DEF("proxy_type", proxy_type, QNetworkProxy::DefaultProxy);
	GET_TXT("proxy_host", proxy_host);
	GET_INT("proxy_port", proxy_port);
	GET_BOOL("proxy_auth", proxy_auth);
	GET_TXT("proxy_user", proxy_user);
	GET_TXT("proxy_pass", proxy_pass);
	s.endGroup();

	s.beginGroup("CloudStorage");
	GET_TXT("email", cloud_storage_email);
	GET_BOOL("save_password_local", save_password_local);
	if (prefs.save_password_local) { // GET_TEXT macro is not a single statement
		GET_TXT("password", cloud_storage_password);
	}
	GET_INT("cloud_verification_status", cloud_verification_status);
	GET_BOOL("cloud_background_sync", cloud_background_sync);

	// creating the git url here is simply a convenience when C code wants
	// to compare against that git URL - it's always derived from the base URL
	GET_TXT("cloud_base_url", cloud_base_url);
	prefs.cloud_git_url = strdup(qPrintable(QString(prefs.cloud_base_url) + "/git"));
	s.endGroup();

	// Subsurface webservice id is stored outside of the groups
	GET_TXT("subsurface_webservice_uid", userid);

	// but the related time / distance threshold (only used in the mobile app)
	// are in their own group
	s.beginGroup("locationService");
	GET_INT("distance_threshold", distance_threshold);
	GET_INT("time_threshold", time_threshold);
	s.endGroup();

	// GeoManagement
	s.beginGroup("geocoding");
#ifdef DISABLED
	GET_BOOL("enable_geocoding", geocoding.enable_geocoding);
	GET_BOOL("parse_dive_without_gps", geocoding.parse_dive_without_gps);
	GET_BOOL("tag_existing_dives", geocoding.tag_existing_dives);
#else
	prefs.geocoding.enable_geocoding = true;
#endif
	GET_ENUM("cat0", taxonomy_category, geocoding.category[0]);
	GET_ENUM("cat1", taxonomy_category, geocoding.category[1]);
	GET_ENUM("cat2", taxonomy_category, geocoding.category[2]);
	s.endGroup();

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
