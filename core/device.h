// SPDX-License-Identifier: GPL-2.0
#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct divecomputer;
extern void fake_dc(struct divecomputer *dc);
extern void set_dc_deviceid(struct divecomputer *dc, unsigned int deviceid);
extern void set_dc_nickname(struct dive *dive);
extern void create_device_node(const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname);
extern void call_for_each_dc(void *f, void (*callback)(void *, const char *, uint32_t,
						       const char *, const char *, const char *), bool select_only);
extern void clear_device_nodes();
const char *get_dc_nickname(const struct divecomputer *dc);

#ifdef __cplusplus
}
#endif

// Functions and global variables that are only available to C++ code
#ifdef __cplusplus

#include <string>
#include <vector>
struct device {
	bool operator==(const device &a) const;
	bool operator!=(const device &a) const;
	bool operator<(const device &a) const;
	void showchanges(const std::string &n, const std::string &s, const std::string &f) const;
	std::string model;
	uint32_t deviceId;
	std::string serialNumber;
	std::string firmware;
	std::string nickName;
};

struct device_table {
	// Keep the dive computers in a vector sorted by (model, deviceId)
	std::vector<device> devices;
};

extern struct device_table device_table;

#endif

#endif // DEVICE_H
