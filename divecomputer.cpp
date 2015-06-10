#include "divecomputer.h"
#include "dive.h"

#include <QSettings>

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
	return model == a.model &&
	       deviceId == a.deviceId &&
	       firmware == a.firmware &&
	       serialNumber == a.serialNumber &&
	       nickName == a.nickName;
}

bool DiveComputerNode::operator!=(const DiveComputerNode &a) const
{
	return !(*this == a);
}

bool DiveComputerNode::changesValues(const DiveComputerNode &b) const
{
	if (model != b.model || deviceId != b.deviceId) {
		qDebug("DiveComputerNodes were not for the same DC");
		return false;
	}
	return (firmware != b.firmware) ||
	       (serialNumber != b.serialNumber) ||
	       (nickName != b.nickName);
}

const DiveComputerNode *DiveComputerList::getExact(const QString &m, uint32_t d)
{
	for (QMap<QString, DiveComputerNode>::iterator it = dcMap.find(m); it != dcMap.end() && it.key() == m; ++it)
		if (it->deviceId == d)
			return &*it;
	return NULL;
}

const DiveComputerNode *DiveComputerList::get(const QString &m)
{
	QMap<QString, DiveComputerNode>::iterator it = dcMap.find(m);
	if (it != dcMap.end())
		return &*it;
	return NULL;
}

void DiveComputerList::addDC(const QString &m, uint32_t d, const QString &n, const QString &s, const QString &f)
{
	if (m.isEmpty() || d == 0)
		return;
	const DiveComputerNode *existNode = this->getExact(m, d);
	DiveComputerNode newNode(m, d, s, f, n);
	if (existNode) {
		if (newNode.changesValues(*existNode)) {
			if (n.size() && existNode->nickName != n)
				qDebug("new nickname %s for DC model %s deviceId 0x%x", n.toUtf8().data(), m.toUtf8().data(), d);
			if (f.size() && existNode->firmware != f)
				qDebug("new firmware version %s for DC model %s deviceId 0x%x", f.toUtf8().data(), m.toUtf8().data(), d);
			if (s.size() && existNode->serialNumber != s)
				qDebug("new serial number %s for DC model %s deviceId 0x%x", s.toUtf8().data(), m.toUtf8().data(), d);
		} else {
			return;
		}
		dcMap.remove(m, *existNode);
	}
	dcMap.insert(m, newNode);
}

extern "C" void create_device_node(const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname)
{
	dcList.addDC(model, deviceid, nickname, serial, firmware);
}

extern "C" bool compareDC(const DiveComputerNode &a, const DiveComputerNode &b)
{
	return a.deviceId < b.deviceId;
}

extern "C" void call_for_each_dc (void *f, void (*callback)(void *, const char *, uint32_t,
							   const char *, const char *, const char *),
				  bool select_only)
{
	QList<DiveComputerNode> values = dcList.dcMap.values();
	qSort(values.begin(), values.end(), compareDC);
	for (int i = 0; i < values.size(); i++) {
		const DiveComputerNode *node = &values.at(i);
		bool found = false;
		if (select_only) {
			int j;
			struct dive *d;
			for_each_dive (j, d) {
				struct divecomputer *dc;
				if (!d->selected)
					continue;
				for_each_dc(d, dc) {
					if (dc->deviceid == node->deviceId) {
						found = true;
						break;
					}
				}
				if (found)
					break;
			}
		} else {
			found = true;
		}
		if (found)
			callback(f, node->model.toUtf8().data(), node->deviceId, node->nickName.toUtf8().data(),
				 node->serialNumber.toUtf8().data(), node->firmware.toUtf8().data());
	}
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

	free((void *)default_dive_computer_vendor);
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

	struct divecomputer *dc;

	for_each_dc (dive, dc) {
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
	}
}
