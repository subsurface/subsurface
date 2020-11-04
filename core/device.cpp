// SPDX-License-Identifier: GPL-2.0
#include "ssrf.h"
#include "dive.h"
#include "subsurface-string.h"
#include "device.h"
#include "errorhelper.h" // for verbose flag
#include "selection.h"
#include "core/settings/qPrefDiveComputer.h"
#include <QString> // for QString::number

struct device_table device_table;

bool device::operator==(const device &a) const
{
	return model == a.model &&
	       deviceId == a.deviceId &&
	       firmware == a.firmware &&
	       serialNumber == a.serialNumber &&
	       nickName == a.nickName;
}

static bool same_device(const device &dev1, const device &dev2)
{
	return dev1.deviceId == dev2.deviceId && strcoll(dev1.model.c_str(), dev2.model.c_str()) == 0;
}

bool device::operator<(const device &a) const
{
	if (deviceId != a.deviceId)
		return deviceId < a.deviceId;

	// Use strcoll to compare model-strings, since these might be unicode
	// and therefore locale dependent? Let's hope that not, but who knows?
	return strcoll(model.c_str(), a.model.c_str()) < 0;
}

extern "C" const struct device *get_device_for_dc(const struct device_table *table, const struct divecomputer *dc)
{
	const std::vector<device> &dcs = table->devices;
	device dev { dc->model ?: "", dc->deviceid, {}, {}, {} };
	auto it = std::lower_bound(dcs.begin(), dcs.end(), dev);
	return it != dcs.end() && same_device(*it, dev) ? &*it : NULL;
}

extern "C" bool device_exists(const struct device_table *device_table, const struct device *dev)
{
	auto it = std::lower_bound(device_table->devices.begin(), device_table->devices.end(), *dev);
	return it != device_table->devices.end() && same_device(*it, *dev);
}

/*
 * When setting the device ID, we also fill in the
 * serial number and firmware version data
 */
extern "C" void set_dc_deviceid(struct divecomputer *dc, unsigned int deviceid, const struct device_table *device_table)
{
	if (!deviceid)
		return;

	dc->deviceid = deviceid;

	// Serial and firmware can only be deduced if we know the model
	if (empty_string(dc->model))
		return;

	const device *node = get_device_for_dc(device_table, dc);
	if (!node)
		return;

	if (!node->serialNumber.empty() && empty_string(dc->serial)) {
		free((void *)dc->serial);
		dc->serial = strdup(node->serialNumber.c_str());
	}
	if (!node->firmware.empty() && empty_string(dc->fw_version)) {
		free((void *)dc->fw_version);
		dc->fw_version = strdup(node->firmware.c_str());
	}
}

void device::showchanges(const std::string &n, const std::string &s, const std::string &f) const
{
	if (nickName != n && !n.empty())
		qDebug("new nickname %s for DC model %s deviceId 0x%x", n.c_str(), model.c_str(), deviceId);
	if (serialNumber != s && !s.empty())
		qDebug("new serial number %s for DC model %s deviceId 0x%x", s.c_str(), model.c_str(), deviceId);
	if (firmware != f && !f.empty())
		qDebug("new firmware version %s for DC model %s deviceId 0x%x", f.c_str(), model.c_str(), deviceId);
}

static void addDC(std::vector<device> &dcs, const std::string &m, uint32_t d, const std::string &n, const std::string &s, const std::string &f)
{
	if (m.empty() || d == 0)
		return;
	device dev { m, d, {}, {}, {} };
	auto it = std::lower_bound(dcs.begin(), dcs.end(), dev);
	if (it != dcs.end() && same_device(*it, dev)) {
		// debugging: show changes
		if (verbose)
			it->showchanges(n, s, f);
		// Update any non-existent fields from the old entry
		if (!n.empty())
			it->nickName = n;
		if (!s.empty())
			it->serialNumber = s;
		if (!f.empty())
			it->firmware = f;
	} else {
		dcs.insert(it, device{m, d, s, f, n});
	}
}

extern "C" void create_device_node(struct device_table *device_table, const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname)
{
	addDC(device_table->devices, model ?: "", deviceid, nickname ?: "", serial ?: "", firmware ?: "");
}

/* Does not check for duplicates! */
extern "C" int add_to_device_table(struct device_table *device_table, const struct device *dev)
{
	auto it = std::lower_bound(device_table->devices.begin(), device_table->devices.end(), *dev);
	int idx = it - device_table->devices.begin();
	device_table->devices.insert(it, *dev);
	return idx;
}

extern "C" int remove_device(struct device_table *device_table, const struct device *dev)
{
	auto it = std::lower_bound(device_table->devices.begin(), device_table->devices.end(), *dev);
	if (it != device_table->devices.end() && same_device(*it, *dev)) {
		int idx = it - device_table->devices.begin();
		device_table->devices.erase(it);
		return idx;
	} else {
		return -1;
	}
}

extern "C" void remove_from_device_table(struct device_table *device_table, int idx)
{
	if (idx < 0 || idx >= (int)device_table->devices.size())
		return;
	device_table->devices.erase(device_table->devices.begin() + idx);
}

extern "C" void clear_device_table(struct device_table *device_table)
{
	device_table->devices.clear();
}

/* Returns whether the given device is used by a selected dive. */
extern "C" bool device_used_by_selected_dive(const struct device *dev)
{
	for (dive *d: getDiveSelection()) {
		struct divecomputer *dc;
		for_each_dc (d, dc) {
			if (dc->deviceid == dev->deviceId)
				return true;
		}
	}
	return false;
}

extern "C" int is_default_dive_computer_device(const char *name)
{
	return qPrefDiveComputer::device() == name;
}

extern "C" void add_devices_of_dive(const struct dive *dive, struct device_table *device_table)
{
	if (!dive)
		return;

	const struct divecomputer *dc;

	for_each_dc (dive, dc) {
		if (!empty_string(dc->model) && dc->deviceid &&
		    !get_device_for_dc(device_table, dc)) {
			// we don't have this one, yet
			if (std::any_of(device_table->devices.begin(), device_table->devices.end(),
				        [dc] (const device &dev)
				        { return !strcasecmp(dev.model.c_str(), dc->model); })) {
				// we already have this model but a different deviceid
				std::string simpleNick(dc->model);
				if (dc->deviceid == 0)
					simpleNick += " (unknown deviceid)";
				else
					simpleNick += " (" + QString::number(dc->deviceid, 16).toStdString() + ")";
				addDC(device_table->devices, dc->model, dc->deviceid, simpleNick, {}, {});
			} else {
				addDC(device_table->devices, dc->model, dc->deviceid, {}, {}, {});
			}
		}
	}
}

const char *get_dc_nickname(const struct divecomputer *dc)
{
	const device *existNode = get_device_for_dc(&device_table, dc);

	if (existNode && !existNode->nickName.empty())
		return existNode->nickName.c_str();
	else
		return dc->model;
}

extern "C" int nr_devices(const struct device_table *table)
{
	return (int)table->devices.size();
}

extern "C" const struct device *get_device(const struct device_table *table, int i)
{
	if (i < 0 || i > nr_devices(table))
		return NULL;
	return &table->devices[i];
}

extern "C" struct device *get_device_mutable(struct device_table *table, int i)
{
	if (i < 0 || i > nr_devices(table))
		return NULL;
	return &table->devices[i];
}

extern "C" const char *device_get_model(const struct device *dev)
{
	return dev ? dev->model.c_str() : NULL;
}

extern "C" const uint32_t device_get_id(const struct device *dev)
{
	return dev ? dev->deviceId : -1;
}

extern "C" const char *device_get_serial(const struct device *dev)
{
	return dev ? dev->serialNumber.c_str() : NULL;
}

extern "C" const char *device_get_firmware(const struct device *dev)
{
	return dev ? dev->firmware.c_str() : NULL;
}

extern "C" const char *device_get_nickname(const struct device *dev)
{
	return dev ? dev->nickName.c_str() : NULL;
}

extern "C" struct device_table *alloc_device_table()
{
	return new struct device_table;
}

extern "C" void free_device_table(struct device_table *devices)
{
	delete devices;
}
