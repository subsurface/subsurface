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
	       serialNumber == a.serialNumber;
}

static bool same_device(const device &dev1, const device &dev2)
{
	return strcmp(dev1.model.c_str(), dev2.model.c_str()) == 0 &&
		strcmp(dev1.serialNumber.c_str(), dev2.serialNumber.c_str()) == 0;
}

bool device::operator<(const device &a) const
{
	int diff;

	diff = strcmp(model.c_str(), a.model.c_str());
	if (diff)
		return diff < 0;

	return strcmp(serialNumber.c_str(), a.serialNumber.c_str()) < 0;
}

extern "C" const struct device *get_device_for_dc(const struct device_table *table, const struct divecomputer *dc)
{
	if (!dc->model || !dc->serial)
		return NULL;

	const std::vector<device> &dcs = table->devices;
	device dev { dc->model, dc->serial };
	auto it = std::lower_bound(dcs.begin(), dcs.end(), dev);
	return it != dcs.end() && same_device(*it, dev) ? &*it : NULL;
}

extern "C" bool device_exists(const struct device_table *device_table, const struct device *dev)
{
	auto it = std::lower_bound(device_table->devices.begin(), device_table->devices.end(), *dev);
	return it != device_table->devices.end() && same_device(*it, *dev);
}

void device::showchanges(const std::string &n) const
{
	if (nickName != n) {
		if (!n.empty())
			qDebug("new nickname %s for DC model %s serial %s", n.c_str(), model.c_str(), serialNumber.c_str());
		else
			qDebug("deleted nickname %s for DC model %s serial %s", nickName.c_str(), model.c_str(), serialNumber.c_str());
	}
}

static int addDC(std::vector<device> &dcs, const std::string &m, const std::string &s, const std::string &n)
{
	if (m.empty() || s.empty())
		return -1;
	device dev { m, s, n };
	auto it = std::lower_bound(dcs.begin(), dcs.end(), dev);
	if (it != dcs.end() && same_device(*it, dev)) {
		// debugging: show changes
		if (verbose)
			it->showchanges(n);
		// Update any non-existent fields from the old entry
		if (n.empty()) {
			dcs.erase(it);
			return -1;
		}
		it->nickName = n;
		return it - dcs.begin();
	} else {
		if (n.empty())
			return -1;

		dev.deviceId = calculate_string_hash(s.c_str());
		dcs.insert(it, dev);
		return it - dcs.begin();
	}
}

extern "C" int create_device_node(struct device_table *device_table, const char *model, const char *serial, const char *nickname)
{
	return addDC(device_table->devices, model ?: "", serial ?: "", nickname ?: "");
}

extern "C" int add_to_device_table(struct device_table *device_table, const struct device *dev)
{
	return create_device_node(device_table, dev->model.c_str(), dev->serialNumber.c_str(), dev->nickName.c_str());
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

extern "C" const char *device_get_serial(const struct device *dev)
{
	return dev ? dev->serialNumber.c_str() : NULL;
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
