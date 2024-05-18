// SPDX-License-Identifier: GPL-2.0
#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <string>
#include <vector>

struct divecomputer;
struct device;
struct device_table;
struct dive_table;

// global device table
extern struct fingerprint_table fingerprint_table;

extern int create_device_node(struct device_table *table, const std::string &model, const std::string &serial, const std::string &nickname);
extern int nr_devices(const struct device_table *table);
extern const struct device *get_device(const struct device_table *table, int i);
extern struct device *get_device_mutable(struct device_table *table, int i);
extern void clear_device_table(struct device_table *table);
std::string get_dc_nickname(const struct divecomputer *dc);
extern bool device_used_by_selected_dive(const struct device *dev);

extern const struct device *get_device_for_dc(const struct device_table *table, const struct divecomputer *dc);
extern int get_or_add_device_for_dc(struct device_table *table, const struct divecomputer *dc);
extern bool device_exists(const struct device_table *table, const struct device *dev);
extern int add_to_device_table(struct device_table *table, const struct device *dev); // returns index
extern int remove_device(struct device_table *table, const struct device *dev); // returns index or -1 if not found
extern void remove_from_device_table(struct device_table *table, int idx);

// struct device accessors for C-code. The returned strings are not stable!
std::string device_get_model(const struct device *dev);
std::string device_get_serial(const struct device *dev);
std::string device_get_nickname(const struct device *dev);

// for C code that needs to alloc/free a device table. (Let's try to get rid of those)
extern struct device_table *alloc_device_table();
extern void free_device_table(struct device_table *devices);

// create fingerprint entry - raw data remains owned by caller
extern void create_fingerprint_node(struct fingerprint_table *table, uint32_t model, uint32_t serial,
				   const unsigned char *raw_data, unsigned int fsize, uint32_t fdeviceid, uint32_t fdiveid);
extern void create_fingerprint_node_from_hex(struct fingerprint_table *table, uint32_t model, uint32_t serial,
					    const char *hex_data, uint32_t fdeviceid, uint32_t fdiveid);
// look up the fingerprint for model/serial - returns the number of bytes in the fingerprint; memory owned by the table
extern unsigned int get_fingerprint_data(const struct fingerprint_table *table, uint32_t model, uint32_t serial, const unsigned char **fp_out);

// access the fingerprint data from C
extern int nr_fingerprints(struct fingerprint_table *table);
extern uint32_t fp_get_model(struct fingerprint_table *table, unsigned int i);
extern uint32_t fp_get_serial(struct fingerprint_table *table, unsigned int i);
extern uint32_t fp_get_deviceid(struct fingerprint_table *table, unsigned int i);
extern uint32_t fp_get_diveid(struct fingerprint_table *table, unsigned int i);

extern int is_default_dive_computer_device(const char *);

typedef void (*device_callback_t)(const char *name, void *userdata);

extern int enumerate_devices(device_callback_t callback, void *userdata, unsigned int transport);

// Functions and global variables that are only available to C++ code
struct device {
	bool operator<(const device &a) const;
	void showchanges(const std::string &n) const;
	std::string model;
	std::string serialNumber;
	std::string nickName;
	uint32_t deviceId;	// Always the string hash of the serialNumber
};

struct fingerprint_record {
	bool operator<(const fingerprint_record &a) const;
	uint32_t       model;     // model and libdivecomputer serial number to
	uint32_t       serial;    //    look up the fingerprint
	unsigned char *raw_data;  // fingerprint data as provided by libdivecomputer
	unsigned int   fsize;     // size of raw fingerprint data
	unsigned int   fdeviceid; // corresponding deviceid
	unsigned int   fdiveid;   // corresponding diveid
};

struct device_table {
	// Keep the dive computers in a vector sorted by (model, serial)
	std::vector<device> devices;
};

struct fingerprint_table {
	// Keep the fingerprint records in a vector sorted by (model, serial) - these are uint32_t here
	std::vector<fingerprint_record> fingerprints;
};

std::string fp_get_data(struct fingerprint_table *table, unsigned int i);

#endif // DEVICE_H
