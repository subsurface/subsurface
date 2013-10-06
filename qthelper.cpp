#include "qthelper.h"
#include <QRegExp>
#include <QDebug>

DiveComputerList::DiveComputerList()
{

}

DiveComputerList::~DiveComputerList()
{

}

bool DiveComputerNode::operator == (const DiveComputerNode &a) const {
	return this->model == a.model &&
			this->deviceId == a.deviceId &&
			this->firmware == a.firmware &&
			this->serialNumber == a.serialNumber &&
			this->nickName == a.nickName;
}

bool DiveComputerNode::operator !=(const DiveComputerNode &a) const {
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
	for (QMap<QString,DiveComputerNode>::iterator it = dcMap.find(m); it != dcMap.end() && it.key() == m; ++it)
		if (it->deviceId == d)
			return &*it;
	return NULL;
}

const DiveComputerNode *DiveComputerList::get(QString m)
{
	QMap<QString,DiveComputerNode>::iterator it = dcMap.find(m);
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
		str = QString("%1").arg(lbs, 0, 'f', lbs >= 40.0 ? 0 : 1 );
	}
	return (str);
}

bool parseGpsText(const QString& gps_text, double *latitude, double *longitude)
{
	/* an empty string is interpreted as 0.0,0.0 and therefore "no gps location" */
	if (gps_text.trimmed() == "") {
		*latitude = 0.0;
		*longitude = 0.0;
		return true;
	}
	QRegExp r("\\s*([SN])\\s*(\\d+)[" UTF8_DEGREE "\\s]+(\\d+)\\.(\\d+)[^EW]*([EW])\\s*(\\d+)[" UTF8_DEGREE "\\s]+(\\d+)\\.(\\d+)");
	if (r.indexIn(gps_text) != 8) {
		// qDebug() << "Hemisphere" << r.cap(1) << "deg" << r.cap(2) << "min" << r.cap(3) << "decimal" << r.cap(4);
		// qDebug() << "Hemisphere" << r.cap(5) << "deg" << r.cap(6) << "min" << r.cap(7) << "decimal" << r.cap(8);
		*latitude = r.cap(2).toInt() + (r.cap(3) + QString(".") + r.cap(4)).toDouble() / 60.0;
		*longitude = r.cap(6).toInt() + (r.cap(7) + QString(".") + r.cap(8)).toDouble() / 60.0;
		if (r.cap(1) == "S")
			*latitude *= -1.0;
		if (r.cap(5) == "W")
			*longitude *= -1.0;
		// qDebug("%s -> %8.5f / %8.5f", gps_text.toLocal8Bit().data(), *latitude, *longitude);
		return true;
	}
	return false;
}

bool gpsHasChanged(struct dive *dive, struct dive *master, const QString& gps_text)
{
	double latitude, longitude;
	int latudeg, longudeg;

	/* if we have a master and the dive's gps address is different from it,
	 * don't change the dive */
	if (master && (master->latitude.udeg != dive->latitude.udeg ||
		       master->longitude.udeg != dive->longitude.udeg))
		return false;

	if (!parseGpsText(gps_text, &latitude, &longitude))
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
