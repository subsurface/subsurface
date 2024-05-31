// SPDX-License-Identifier: GPL-2.0
#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

struct divecomputer;
struct dive_table;

struct device {
	bool operator<(const device &a) const;
	void showchanges(const std::string &n) const;
	std::string model;
	std::string serialNumber;
	std::string nickName;
	uint32_t deviceId;	// Always the string hash of the serialNumber
};

using device_table = std::vector<device>;

extern int create_device_node(device_table &table, const std::string &model, const std::string &serial, const std::string &nickname);
std::string get_dc_nickname(const struct divecomputer *dc);
extern bool device_used_by_selected_dive(const struct device &dev);

extern const struct device *get_device_for_dc(const device_table &table, const struct divecomputer *dc);
extern int get_or_add_device_for_dc(device_table &table, const struct divecomputer *dc);
extern bool device_exists(const device_table &table, const struct device &dev);
extern int add_to_device_table(device_table &table, const struct device &dev); // returns index
extern int remove_device(device_table &table, const struct device &dev); // returns index or -1 if not found
extern void remove_from_device_table(device_table &table, int idx);

struct fingerprint_record {
	bool operator<(const fingerprint_record &a) const;
	uint32_t       model;     // model and libdivecomputer serial number to
	uint32_t       serial;    //    look up the fingerprint
	std::unique_ptr<unsigned char[]> raw_data;  // fingerprint data as provided by libdivecomputer
	unsigned int   fsize;     // size of raw fingerprint data
	unsigned int   fdeviceid; // corresponding deviceid
	unsigned int   fdiveid;   // corresponding diveid
	std::string get_data() const;	// As hex-string
};

using fingerprint_table = std::vector<fingerprint_record>;

// global device table
extern fingerprint_table fingerprints;

// create fingerprint entry - raw data remains owned by caller
extern void create_fingerprint_node(fingerprint_table &table, uint32_t model, uint32_t serial,
				   const unsigned char *raw_data, unsigned int fsize, uint32_t fdeviceid, uint32_t fdiveid);
extern void create_fingerprint_node_from_hex(fingerprint_table &table, uint32_t model, uint32_t serial,
					    const std::string &hex_data, uint32_t fdeviceid, uint32_t fdiveid);
// look up the fingerprint for model/serial - returns the number of bytes in the fingerprint; memory owned by the table
extern std::pair <int, const unsigned char *> get_fingerprint_data(const fingerprint_table &table, uint32_t model, uint32_t serial);

extern int is_default_dive_computer_device(const char *);

typedef void (*device_callback_t)(const char *name, void *userdata);

extern int enumerate_devices(device_callback_t callback, void *userdata, unsigned int transport);


#endif // DEVICE_H
