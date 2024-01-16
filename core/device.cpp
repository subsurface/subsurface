// SPDX-License-Identifier: GPL-2.0
#include "device.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "subsurface-string.h"
#include "errorhelper.h" // for verbose flag
#include "selection.h"
#include "core/settings/qPrefDiveComputer.h"

struct fingerprint_table fingerprint_table;

static bool same_device(const device &dev1, const device &dev2)
{
	return dev1.model == dev2.model &&
		dev1.serialNumber == dev2.serialNumber;
}

bool device::operator<(const device &a) const
{
	int diff;

	diff = model.compare(a.model);
	if (diff)
		return diff < 0;

	return serialNumber < a.serialNumber;
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

extern "C" int get_or_add_device_for_dc(struct device_table *table, const struct divecomputer *dc)
{
	if (!dc->model || !dc->serial)
		return -1;
	const struct device *dev = get_device_for_dc(table, dc);
	if (dev) {
		auto it = std::lower_bound(table->devices.begin(), table->devices.end(), *dev);
		return it - table->devices.begin();
	}
	return create_device_node(table, dc->model, dc->serial, "");
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
		it->nickName = n;
		return it - dcs.begin();
	} else {
		dev.deviceId = calculate_string_hash(s.c_str());
		it = dcs.insert(it, dev);
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
	const device *existNode = get_device_for_dc(divelog.devices, dc);

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

// managing fingerprint data
bool fingerprint_record::operator<(const fingerprint_record &a) const
{
	if (model == a.model)
		return serial < a.serial;
	return model < a.model;
}

// annoyingly, the Cressi Edy doesn't support a serial number (it's always 0), but still uses fingerprints
// so we can't bail on the serial number being 0
extern "C" unsigned int get_fingerprint_data(const struct fingerprint_table *table, uint32_t model, uint32_t serial, const unsigned char **fp_out)
{
	if (model == 0 || fp_out == nullptr)
		return 0;
	struct fingerprint_record fpr = { model, serial };
	auto it = std::lower_bound(table->fingerprints.begin(), table->fingerprints.end(), fpr);
	if (it != table->fingerprints.end() && it->model == model && it->serial == serial) {
		// std::lower_bound gets us the first element that isn't smaller than what we are looking
		// for - so if one is found, we still need to check for equality
		if (has_dive(it->fdeviceid, it->fdiveid)) {
			*fp_out = it->raw_data;
			return it->fsize;
		}
	}
	return 0;
}

extern "C" void create_fingerprint_node(struct fingerprint_table *table, uint32_t model, uint32_t serial,
				       const unsigned char *raw_data_in, unsigned int fsize, uint32_t fdeviceid, uint32_t fdiveid)
{
	// since raw data can contain \0 we copy this manually, not as string
	unsigned char *raw_data = (unsigned char *)malloc(fsize);
	if (!raw_data)
		return;
	memcpy(raw_data, raw_data_in, fsize);

	struct fingerprint_record fpr = { model, serial, raw_data, fsize, fdeviceid, fdiveid };
	auto it = std::lower_bound(table->fingerprints.begin(), table->fingerprints.end(), fpr);
	if (it != table->fingerprints.end() && it->model == model && it->serial == serial) {
		// std::lower_bound gets us the first element that isn't smaller than what we are looking
		// for - so if one is found, we still need to check for equality - and then we
		// can update the existing entry; first we free the memory for the stored raw data
		free(it->raw_data);
		it->fdeviceid = fdeviceid;
		it->fdiveid = fdiveid;
		it->raw_data = raw_data;
		it->fsize = fsize;
	} else {
		// insert a new one
		table->fingerprints.insert(it, fpr);
	}
}

extern "C" void create_fingerprint_node_from_hex(struct fingerprint_table *table, uint32_t model, uint32_t serial,
						const char *hex_data, uint32_t fdeviceid, uint32_t fdiveid)
{
	QByteArray raw = QByteArray::fromHex(hex_data);
	create_fingerprint_node(table, model, serial,
				(const unsigned char *)raw.constData(), raw.size(), fdeviceid, fdiveid);
}

extern "C" int nr_fingerprints(struct fingerprint_table *table)
{
	return table->fingerprints.size();
}

extern "C" uint32_t fp_get_model(struct fingerprint_table *table, unsigned int i)
{
	if (!table || i >= table->fingerprints.size())
		return 0;
	return table->fingerprints[i].model;
}

extern "C" uint32_t fp_get_serial(struct fingerprint_table *table, unsigned int i)
{
	if (!table || i >= table->fingerprints.size())
		return 0;
	return table->fingerprints[i].serial;
}

extern "C" uint32_t fp_get_deviceid(struct fingerprint_table *table, unsigned int i)
{
	if (!table || i >= table->fingerprints.size())
		return 0;
	return table->fingerprints[i].fdeviceid;
}

extern "C" uint32_t fp_get_diveid(struct fingerprint_table *table, unsigned int i)
{
	if (!table || i >= table->fingerprints.size())
		return 0;
	return table->fingerprints[i].fdiveid;
}

extern "C" char *fp_get_data(struct fingerprint_table *table, unsigned int i)
{
	if (!table || i >= table->fingerprints.size())
		return 0;
	struct fingerprint_record *fpr = &table->fingerprints[i];
	// fromRawData() avoids one copy of the raw_data
	QByteArray hex = QByteArray::fromRawData((char *)fpr->raw_data, fpr->fsize).toHex();
	return strdup(hex.constData());
}
