// SPDX-License-Identifier: GPL-2.0
#include "device.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "subsurface-string.h"
#include "errorhelper.h"
#include "selection.h"
#include "core/settings/qPrefDiveComputer.h"

fingerprint_table fingerprints;

static bool same_device(const device &dev1, const device &dev2)
{
	return dev1.model == dev2.model &&
		dev1.serialNumber == dev2.serialNumber;
}

bool device::operator<(const device &a) const
{
	return std::tie(model, serialNumber) < std::tie(a.model, a.serialNumber);
}

const struct device *get_device_for_dc(const device_table &table, const struct divecomputer *dc)
{
	if (dc->model.empty() || dc->serial.empty())
		return NULL;

	device dev { dc->model, dc->serial };
	auto it = std::lower_bound(table.begin(), table.end(), dev);
	return it != table.end() && same_device(*it, dev) ? &*it : NULL;
}

int get_or_add_device_for_dc(device_table &table, const struct divecomputer *dc)
{
	if (dc->model.empty() || dc->serial.empty())
		return -1;
	const struct device *dev = get_device_for_dc(table, dc);
	if (dev) {
		auto it = std::lower_bound(table.begin(), table.end(), *dev);
		return it - table.begin();
	}
	return create_device_node(table, dc->model, dc->serial, std::string());
}

bool device_exists(const device_table &table, const struct device &dev)
{
	auto it = std::lower_bound(table.begin(), table.end(), dev);
	return it != table.end() && same_device(*it, dev);
}

void device::showchanges(const std::string &n) const
{
	if (nickName != n) {
		if (!n.empty())
			report_info("new nickname %s for DC model %s serial %s", n.c_str(), model.c_str(), serialNumber.c_str());
		else
			report_info("deleted nickname %s for DC model %s serial %s", nickName.c_str(), model.c_str(), serialNumber.c_str());
	}
}

int create_device_node(device_table &dcs, const std::string &m, const std::string &s, const std::string &n)
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

int add_to_device_table(device_table &device_table, const struct device &dev)
{
	return create_device_node(device_table, dev.model, dev.serialNumber, dev.nickName);
}

int remove_device(device_table &table, const struct device &dev)
{
	auto it = std::lower_bound(table.begin(), table.end(), dev);
	if (it != table.end() && same_device(*it, dev)) {
		int idx = it - table.begin();
		table.erase(it);
		return idx;
	} else {
		return -1;
	}
}

void remove_from_device_table(device_table &table, int idx)
{
	if (idx < 0 || idx >= (int)table.size())
		return;
	table.erase(table.begin() + idx);
}

/* Returns whether the given device is used by a selected dive. */
bool device_used_by_selected_dive(const struct device &dev)
{
	for (dive *d: getDiveSelection()) {
		for (auto &dc: d->dcs) {
			if (dc.deviceid == dev.deviceId)
				return true;
		}
	}
	return false;
}

int is_default_dive_computer_device(const char *name)
{
	return qPrefDiveComputer::device() == name;
}

std::string get_dc_nickname(const struct divecomputer *dc)
{
	const device *existNode = get_device_for_dc(divelog.devices, dc);

	if (existNode && !existNode->nickName.empty())
		return existNode->nickName;
	else
		return dc->model;
}

// managing fingerprint data
bool fingerprint_record::operator<(const fingerprint_record &a) const
{
	return std::tie(model, serial) < std::tie(a.model, a.serial);
}

// annoyingly, the Cressi Edy doesn't support a serial number (it's always 0), but still uses fingerprints
// so we can't bail on the serial number being 0
std::pair<int, const unsigned char *> get_fingerprint_data(const fingerprint_table &table, uint32_t model, uint32_t serial)
{
	if (model == 0)
		return { 0, nullptr };
	struct fingerprint_record fpr = { model, serial };
	auto it = std::lower_bound(table.begin(), table.end(), fpr);
	if (it != table.end() && it->model == model && it->serial == serial) {
		// std::lower_bound gets us the first element that isn't smaller than what we are looking
		// for - so if one is found, we still need to check for equality
		if (divelog.dives.has_dive(it->fdeviceid, it->fdiveid))
			return { it->fsize, it->raw_data.get() };
	}
	return { 0, nullptr };
}

void create_fingerprint_node(fingerprint_table &table, uint32_t model, uint32_t serial,
			     const unsigned char *raw_data_in, unsigned int fsize, uint32_t fdeviceid, uint32_t fdiveid)
{
	// since raw data can contain \0 we copy this manually, not as string
	auto raw_data = std::make_unique<unsigned char []>(fsize);
	std::copy(raw_data_in, raw_data_in + fsize, raw_data.get());

	struct fingerprint_record fpr = { model, serial, std::move(raw_data), fsize, fdeviceid, fdiveid };
	auto it = std::lower_bound(table.begin(), table.end(), fpr);
	if (it != table.end() && it->model == model && it->serial == serial) {
		// std::lower_bound gets us the first element that isn't smaller than what we are looking
		// for - so if one is found, we still need to check for equality - and then we
		// can update the existing entry; first we free the memory for the stored raw data
		it->fdeviceid = fdeviceid;
		it->fdiveid = fdiveid;
		it->raw_data = std::move(fpr.raw_data);
		it->fsize = fsize;
	} else {
		// insert a new one
		table.insert(it, std::move(fpr));
	}
}

void create_fingerprint_node_from_hex(fingerprint_table &table, uint32_t model, uint32_t serial,
						const std::string &hex_data, uint32_t fdeviceid, uint32_t fdiveid)
{
	QByteArray raw = QByteArray::fromHex(hex_data.c_str());
	create_fingerprint_node(table, model, serial,
				(const unsigned char *)raw.constData(), raw.size(), fdeviceid, fdiveid);
}

static char to_hex_digit(unsigned char d)
{
	return d <= 9 ? d + '0' : d - 10 + 'a';
}

std::string fingerprint_record::get_data() const
{
	std::string res(fsize * 2, ' ');
	for (unsigned int i = 0; i < fsize; ++i) {
		res[2 * i] = to_hex_digit((raw_data[i] >> 4) & 0xf);
		res[2 * i + 1] = to_hex_digit(raw_data[i] & 0xf);
	}
	return res;
}
