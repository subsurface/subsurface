#ifndef DEVICE_H
#define DEVICE_H

#ifdef __cplusplus
#include "dive.h"
extern "C" {
#endif

extern struct divecomputer *fake_dc(struct divecomputer *dc);
extern void create_device_node(const char *model, uint32_t deviceid, const char *serial, const char *firmware, const char *nickname);
extern void call_for_each_dc(void *f, void (*callback)(void *, const char *, uint32_t,
						       const char *, const char *, const char *), bool select_only);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_H
