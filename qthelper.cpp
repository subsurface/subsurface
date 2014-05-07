#include "qthelper.h"
#include "qt-gui.h"
#include <QRegExp>
#include <QDir>

#include <QDebug>
#include <QSettings>
#include <libxslt/documents.h>
#include "mainwindow.h"

#define tr(_arg) QObject::tr(_arg)

const char *default_dive_computer_vendor;
const char *default_dive_computer_product;
const char *default_dive_computer_device;
DiveComputerList dcList;

DiveComputerList::DiveComputerList()
{
}

DiveComputerList::~DiveComputerList()
{
}

bool DiveComputerNode::operator==(const DiveComputerNode &a) const
{
	return this->model == a.model &&
	       this->deviceId == a.deviceId &&
	       this->firmware == a.firmware &&
	       this->serialNumber == a.serialNumber &&
	       this->nickName == a.nickName;
}

bool DiveComputerNode::operator!=(const DiveComputerNode &a) const
{
	return !(*this == a);
}

bool DiveComputerNode::changesValues(const DiveComputerNode &b) const
{
	if (this->model != b.model || this->deviceId != b.deviceId) {
		qDebug("DiveComputerNodes were not for the same DC");
		return false;
	}
	return (b.firmware != "" && this->firmware != b.firmware) ||
	       (b.serialNumber != "" && this->serialNumber != b.serialNumber) ||
	       (b.nickName != "" && this->nickName != b.nickName);
}

const DiveComputerNode *DiveComputerList::getExact(QString m, uint32_t d)
{
	for (QMap<QString, DiveComputerNode>::iterator it = dcMap.find(m); it != dcMap.end() && it.key() == m; ++it)
		if (it->deviceId == d)
			return &*it;
	return NULL;
}

const DiveComputerNode *DiveComputerList::get(QString m)
{
	QMap<QString, DiveComputerNode>::iterator it = dcMap.find(m);
	if (it != dcMap.end())
		return &*it;
	return NULL;
}

void DiveComputerList::addDC(QString m, uint32_t d, QString n, QString s, QString f)
{
	if (m == "" || d == 0)
		return;
	const DiveComputerNode *existNode = this->getExact(m, d);
	DiveComputerNode newNode(m, d, s, f, n);
	if (existNode) {
		if (newNode.changesValues(*existNode)) {
			if (n != "" && existNode->nickName != n)
				qDebug("new nickname %s for DC model %s deviceId 0x%x", n.toUtf8().data(), m.toUtf8().data(), d);
			if (f != "" && existNode->firmware != f)
				qDebug("new firmware version %s for DC model %s deviceId 0x%x", f.toUtf8().data(), m.toUtf8().data(), d);
			if (s != "" && existNode->serialNumber != s)
				qDebug("new serial number %s for DC model %s deviceId 0x%x", s.toUtf8().data(), m.toUtf8().data(), d);
		} else {
			return;
		}
		dcMap.remove(m, *existNode);
	}
	dcMap.insert(m, newNode);
}

void DiveComputerList::rmDC(QString m, uint32_t d)
{
	const DiveComputerNode *existNode = this->getExact(m, d);
	dcMap.remove(m, *existNode);
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
	trHemisphere[0] = MainWindow::instance()->information()->trHemisphere("N");
	trHemisphere[1] = MainWindow::instance()->information()->trHemisphere("S");
	trHemisphere[2] = MainWindow::instance()->information()->trHemisphere("E");
	trHemisphere[3] = MainWindow::instance()->information()->trHemisphere("W");
	QString regExp;
	/* an empty string is interpreted as 0.0,0.0 and therefore "no gps location" */
	if (gps_text.trimmed() == "") {
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
	for (int i = 0; i < dive_table.nr; i++) {
		struct dive *d = get_dive(i);
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
int getUniqID(struct dive *d)
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

extern "C" void create_device_node(const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname)
{
	dcList.addDC(model, deviceid, nickname, serial, firmware);
}

extern "C" bool compareDC(const DiveComputerNode &a, const DiveComputerNode &b)
{
	return a.deviceId < b.deviceId;
}

extern "C" void call_for_each_dc(void *f, void (*callback)(void *, const char *, uint32_t,
						const char *, const char *, const char *))
{
	QList<DiveComputerNode> values = dcList.dcMap.values();
	qSort(values.begin(), values.end(), compareDC);
	for (int i = 0; i < values.size(); i++) {
		const DiveComputerNode *node = &values.at(i);
		callback(f, node->model.toUtf8().data(), node->deviceId, node->nickName.toUtf8().data(),
			 node->serialNumber.toUtf8().data(), node->firmware.toUtf8().data());
	}
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

extern "C" int is_default_dive_computer(const char *vendor, const char *product)
{
	return default_dive_computer_vendor && !strcmp(vendor, default_dive_computer_vendor) &&
	       default_dive_computer_product && !strcmp(product, default_dive_computer_product);
}

extern "C" int is_default_dive_computer_device(const char *name)
{
	return default_dive_computer_device && !strcmp(name, default_dive_computer_device);
}

void set_default_dive_computer(const char *vendor, const char *product)
{
	QSettings s;

	if (!vendor || !*vendor)
		return;
	if (!product || !*product)
		return;
	if (is_default_dive_computer(vendor, product))
		return;
	if (default_dive_computer_vendor)
		free((void *)default_dive_computer_vendor);
	if (default_dive_computer_product)
		free((void *)default_dive_computer_product);
	default_dive_computer_vendor = strdup(vendor);
	default_dive_computer_product = strdup(product);
	s.beginGroup("DiveComputer");
	s.setValue("dive_computer_vendor", vendor);
	s.setValue("dive_computer_product", product);
	s.endGroup();
}

void set_default_dive_computer_device(const char *name)
{
	QSettings s;

	if (!name || !*name)
		return;
	if (is_default_dive_computer_device(name))
		return;
	if (default_dive_computer_device)
		free((void *)default_dive_computer_device);
	default_dive_computer_device = strdup(name);
	s.beginGroup("DiveComputer");
	s.setValue("dive_computer_device", name);
	s.endGroup();
}

extern "C" void set_dc_nickname(struct dive *dive)
{
	if (!dive)
		return;

	struct divecomputer *dc = &dive->dc;

	while (dc) {
		if (dc->model && *dc->model && dc->deviceid &&
		    !dcList.getExact(dc->model, dc->deviceid)) {
			// we don't have this one, yet
			const DiveComputerNode *existNode = dcList.get(dc->model);
			if (existNode) {
				// we already have this model but a different deviceid
				QString simpleNick(dc->model);
				if (dc->deviceid == 0)
					simpleNick.append(" (unknown deviceid)");
				else
					simpleNick.append(" (").append(QString::number(dc->deviceid, 16)).append(")");
				dcList.addDC(dc->model, dc->deviceid, simpleNick);
			} else {
				dcList.addDC(dc->model, dc->deviceid);
			}
		}
		dc = dc->next;
	}
}
