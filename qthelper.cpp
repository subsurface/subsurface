#include "qthelper.h"
#include "qt-gui.h"
#include <QRegExp>
#include <QDir>

#include <QDebug>
#include <QSettings>
#include <libxslt/documents.h>

#define tr(_arg) QObject::tr(_arg)



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

QString printGPSCoords(int lat, int lon)
{
	unsigned int latdeg, londeg;
	unsigned int latmin, lonmin;
	double latsec, lonsec;
	QString lath, lonh, result;

	if (!lat && !lon)
		return QString();

	lath = lat >= 0 ? tr("N") : tr("S");
	lonh = lon >= 0 ? tr("E") : tr("W");
	lat = abs(lat);
	lon = abs(lon);
	latdeg = lat / 1000000;
	londeg = lon / 1000000;
	latmin = (lat % 1000000) * 60;
	lonmin = (lon % 1000000) * 60;
	latsec = (latmin % 1000000) * 60;
	lonsec = (lonmin % 1000000) * 60;
	result.sprintf("%u%s%02d\'%06.3f\"%s %u%s%02d\'%06.3f\"%s",
		       latdeg, UTF8_DEGREE, latmin / 1000000, latsec / 1000000, lath.toUtf8().data(),
		       londeg, UTF8_DEGREE, lonmin / 1000000, lonsec / 1000000, lonh.toUtf8().data());
	return result;
}

bool parseGpsText(const QString &gps_text, double *latitude, double *longitude)
{
	enum {
		ISO6709D,
		SECONDS,
		MINUTES,
		DECIMAL
	} gpsStyle = ISO6709D;
	int eastWest = 4;
	int northSouth = 1;
	QString trHemisphere[4];
	trHemisphere[0] = tr("N");
	trHemisphere[1] = tr("S");
	trHemisphere[2] = tr("E");
	trHemisphere[3] = tr("W");
	QString regExp;
	/* an empty string is interpreted as 0.0,0.0 and therefore "no gps location" */
	if (gps_text.trimmed().isEmpty()) {
		*latitude = 0.0;
		*longitude = 0.0;
		return true;
	}
	// trying to parse all formats in one regexp might be possible, but it seems insane
	// so handle the four formats we understand separately

	// ISO 6709 Annex D representation
	// http://en.wikipedia.org/wiki/ISO_6709#Representation_at_the_human_interface_.28Annex_D.29
	// e.g. 52°49'02.388"N 1°36'17.388"E
	if (gps_text.at(0).isDigit() && (gps_text.count(",") % 2) == 0) {
		gpsStyle = ISO6709D;
		regExp = QString("(\\d+)[" UTF8_DEGREE "\\s](\\d+)[\'\\s](\\d+)([,\\.](\\d+))?[\"\\s]([NS%1%2])"
				 "\\s*(\\d+)[" UTF8_DEGREE "\\s](\\d+)[\'\\s](\\d+)([,\\.](\\d+))?[\"\\s]([EW%3%4])")
				.arg(trHemisphere[0])
				.arg(trHemisphere[1])
				.arg(trHemisphere[2])
				.arg(trHemisphere[3]);
	} else if (gps_text.count(QChar('"')) == 2) {
		gpsStyle = SECONDS;
		regExp = QString("\\s*([NS%1%2])\\s*(\\d+)[" UTF8_DEGREE "\\s]+(\\d+)[\'\\s]+(\\d+)([,\\.](\\d+))?[^EW%3%4]*"
				 "([EW%5%6])\\s*(\\d+)[" UTF8_DEGREE "\\s]+(\\d+)[\'\\s]+(\\d+)([,\\.](\\d+))?")
				.arg(trHemisphere[0])
				.arg(trHemisphere[1])
				.arg(trHemisphere[2])
				.arg(trHemisphere[3])
				.arg(trHemisphere[2])
				.arg(trHemisphere[3]);
	} else if (gps_text.count(QChar('\'')) == 2) {
		gpsStyle = MINUTES;
		regExp = QString("\\s*([NS%1%2])\\s*(\\d+)[" UTF8_DEGREE "\\s]+(\\d+)([,\\.](\\d+))?[^EW%3%4]*"
				 "([EW%5%6])\\s*(\\d+)[" UTF8_DEGREE "\\s]+(\\d+)([,\\.](\\d+))?")
				.arg(trHemisphere[0])
				.arg(trHemisphere[1])
				.arg(trHemisphere[2])
				.arg(trHemisphere[3])
				.arg(trHemisphere[2])
				.arg(trHemisphere[3]);
	} else {
		gpsStyle = DECIMAL;
		regExp = QString("\\s*([-NS%1%2]?)\\s*(\\d+)[,\\.](\\d+)[^-EW%3%4\\d]*([-EW%5%6]?)\\s*(\\d+)[,\\.](\\d+)")
				.arg(trHemisphere[0])
				.arg(trHemisphere[1])
				.arg(trHemisphere[2])
				.arg(trHemisphere[3])
				.arg(trHemisphere[2])
				.arg(trHemisphere[3]);
	}
	QRegExp r(regExp);
	if (r.indexIn(gps_text) != -1) {
		// qDebug() << "Hemisphere" << r.cap(1) << "deg" << r.cap(2) << "min" << r.cap(3) << "decimal" << r.cap(4);
		// qDebug() << "Hemisphere" << r.cap(5) << "deg" << r.cap(6) << "min" << r.cap(7) << "decimal" << r.cap(8);
		switch (gpsStyle) {
		case ISO6709D:
			*latitude = r.cap(1).toInt() + r.cap(2).toInt() / 60.0 +
				    (r.cap(3) + QString(".") + r.cap(5)).toDouble() / 3600.0;
			*longitude = r.cap(7).toInt() + r.cap(8).toInt() / 60.0 +
				     (r.cap(9) + QString(".") + r.cap(11)).toDouble() / 3600.0;
			northSouth = 6;
			eastWest = 12;
			break;
		case SECONDS:
			*latitude = r.cap(2).toInt() + r.cap(3).toInt() / 60.0 +
				    (r.cap(4) + QString(".") + r.cap(6)).toDouble() / 3600.0;
			*longitude = r.cap(8).toInt() + r.cap(9).toInt() / 60.0 +
				     (r.cap(10) + QString(".") + r.cap(12)).toDouble() / 3600.0;
			eastWest = 7;
			break;
		case MINUTES:
			*latitude = r.cap(2).toInt() + (r.cap(3) + QString(".") + r.cap(5)).toDouble() / 60.0;
			*longitude = r.cap(7).toInt() + (r.cap(8) + QString(".") + r.cap(10)).toDouble() / 60.0;
			eastWest = 6;
			break;
		case DECIMAL:
		default:
			*latitude = (r.cap(2) + QString(".") + r.cap(3)).toDouble();
			*longitude = (r.cap(5) + QString(".") + r.cap(6)).toDouble();
			break;
		}
		if (r.cap(northSouth) == "S" || r.cap(northSouth) == trHemisphere[1] || r.cap(northSouth) == "-")
			*latitude *= -1.0;
		if (r.cap(eastWest) == "W" || r.cap(eastWest) == trHemisphere[3] || r.cap(eastWest) == "-")
			*longitude *= -1.0;
		// qDebug("%s -> %8.5f / %8.5f", gps_text.toLocal8Bit().data(), *latitude, *longitude);
		return true;
	}
	return false;
}

bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed)
{
	double latitude, longitude;
	int latudeg, longudeg;

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
	if (!f.open(QIODevice::ReadOnly))
		return NULL;

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
