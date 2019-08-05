// SPDX-License-Identifier: GPL-2.0
#include "divecomputer.h"
#include "dive.h"
#include "errorhelper.h"
#include "core/settings/qPrefDiveComputer.h"
#include "subsurface-string.h"

DiveComputerList dcList;

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

bool DiveComputerNode::operator<(const DiveComputerNode &a) const
{
	return std::tie(model, deviceId) < std::tie(a.model, a.deviceId);
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
	auto it = std::lower_bound(dcs.begin(), dcs.end(), DiveComputerNode{m, d, {}, {}, {}});
	return it != dcs.end() && it->model == m && it->deviceId == d ? &*it : NULL;
}

const DiveComputerNode *DiveComputerList::get(const QString &m)
{
	auto it = std::lower_bound(dcs.begin(), dcs.end(), DiveComputerNode{m, 0, {}, {}, {}});
	return it != dcs.end() && it->model == m ? &*it : NULL;
}

void DiveComputerNode::showchanges(const QString &n, const QString &s, const QString &f) const
{
	if (nickName != n && !n.isEmpty())
		qDebug("new nickname %s for DC model %s deviceId 0x%x", qPrintable(n), qPrintable(model), deviceId);
	if (serialNumber != s && !s.isEmpty())
		qDebug("new serial number %s for DC model %s deviceId 0x%x", qPrintable(s), qPrintable(model), deviceId);
	if (firmware != f && !f.isEmpty())
		qDebug("new firmware version %s for DC model %s deviceId 0x%x", qPrintable(f), qPrintable(model), deviceId);
}

void DiveComputerList::addDC(QString m, uint32_t d, QString n, QString s, QString f)
{
	if (m.isEmpty() || d == 0)
		return;
	auto it = std::lower_bound(dcs.begin(), dcs.end(), DiveComputerNode{m, d, {}, {}, {}});
	if (it != dcs.end() && it->model == m && it->deviceId == d) {
		// debugging: show changes
		if (verbose)
			it->showchanges(n, s, f);
		// Update any non-existent fields from the old entry
		if (!n.isEmpty())
			it->nickName = n;
		if (!s.isEmpty())
			it->serialNumber = s;
		if (!f.isEmpty())
			it->firmware = f;
	} else {
		dcs.insert(it, DiveComputerNode{m, d, s, f, n});
	}
}

extern "C" void create_device_node(const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname)
{
	dcList.addDC(model, deviceid, nickname, serial, firmware);
}

static bool compareDCById(const DiveComputerNode &a, const DiveComputerNode &b)
{
	return a.deviceId < b.deviceId;
}

extern "C" void call_for_each_dc (void *f, void (*callback)(void *, const char *, uint32_t, const char *, const char *, const char *),
				 bool select_only)
{
	QVector<DiveComputerNode> values = dcList.dcs;
	std::sort(values.begin(), values.end(), compareDCById);
	for (const DiveComputerNode &node : values) {
		bool found = false;
		if (select_only) {
			int j;
			struct dive *d;
			for_each_dive (j, d) {
				struct divecomputer *dc;
				if (!d->selected)
					continue;
				for_each_dc (d, dc) {
					if (dc->deviceid == node.deviceId) {
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
	callback(f, qPrintable(node.model), node.deviceId, qPrintable(node.nickName),
				 qPrintable(node.serialNumber), qPrintable(node.firmware));
	}
}

extern "C" int is_default_dive_computer(const char *vendor, const char *product)
{
	return qPrefDiveComputer::vendor() == vendor && qPrefDiveComputer::product() == product;
}

extern "C" int is_default_dive_computer_device(const char *name)
{
	return qPrefDiveComputer::device() == name;
}

extern "C" void set_dc_nickname(struct dive *dive)
{
	if (!dive)
		return;

	struct divecomputer *dc;

	for_each_dc (dive, dc) {
		if (!empty_string(dc->model) && dc->deviceid &&
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
