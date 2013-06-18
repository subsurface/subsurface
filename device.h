#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#ifdef __cplusplus
#include "dive.h"
extern "C" {
#endif

extern struct divecomputer *fake_dc(struct divecomputer* dc);
extern void create_device_node(const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname);
extern void call_for_each_dc(FILE *f, void (*callback)(FILE *, const char *, uint32_t,
						const char *, const char *, const char *));

#ifdef __cplusplus
}
#endif

#endif
