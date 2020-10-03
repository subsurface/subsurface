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

#ifdef __cplusplus
}
#endif

// Functions and global variables that are only available to C++ code
#ifdef __cplusplus

#include <QString>
#include <QVector>
struct device {
	bool operator==(const device &a) const;
	bool operator!=(const device &a) const;
	bool operator<(const device &a) const;
	void showchanges(const QString &n, const QString &s, const QString &f) const;
	QString model;
	uint32_t deviceId;
	QString serialNumber;
	QString firmware;
	QString nickName;
};

struct device_table {
	// Keep the dive computers in a vector sorted by (model, deviceId)
	QVector<device> devices;
};

QString get_dc_nickname(const struct divecomputer *dc);
extern struct device_table device_table;

#endif

#endif // DEVICE_H
