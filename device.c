#include <string.h>
#include "dive.h"
#include "device.h"

static struct device_info *device_info_list;

struct device_info *head_of_device_info_list(void)
{
	return device_info_list;
}

static int match_device_info(struct device_info *entry, const char *model, uint32_t deviceid)
{
	return !strcmp(entry->model, model) && entry->deviceid == deviceid;
}

/* just find the entry for this divecomputer */
struct device_info *get_device_info(const char *model, uint32_t deviceid)
{
	struct device_info *known = device_info_list;

	/* a 0 deviceid doesn't get a nickname - those come from development
	 * versions of Subsurface that didn't store the deviceid in the divecomputer entries */
	if (!deviceid || !model)
		return NULL;
	while (known) {
		if (match_device_info(known, model, deviceid))
			return known;
		known = known->next;
	}
	return NULL;
}

/* Get an existing device info model or create a new one if valid */
struct device_info *create_device_info(const char *model, uint32_t deviceid)
{
	struct device_info *entry;

	if (!deviceid || !model || !*model)
		return NULL;
	entry = get_device_info(model, deviceid);
	if (entry)
		return entry;
	entry = calloc(1, sizeof(*entry));
	if (entry) {
		entry->model = strdup(model);
		entry->deviceid = deviceid;
		entry->next = device_info_list;
		device_info_list = entry;
	}
	return entry;
}

/* do we have a DIFFERENT divecomputer of the same model? */
struct device_info *get_different_device_info(const char *model, uint32_t deviceid)
{
	struct device_info *known = device_info_list;

	/* a 0 deviceid matches any DC of the same model - those come from development
	 * versions of Subsurface that didn't store the deviceid in the divecomputer entries */
	if (!deviceid)
		return NULL;
	if (!model)
		model = "";
	while (known) {
		if (known->model && !strcmp(known->model, model) &&
		    known->deviceid != deviceid)
			return known;
		known = known->next;
	}
	return NULL;
}

struct device_info *remove_device_info(const char *model, uint32_t deviceid)
{
	struct device_info *entry, **p;

	if (!deviceid || !model || !*model)
		return NULL;
	p = &device_info_list;
	while ((entry = *p) != NULL) {
		if (match_device_info(entry, model, deviceid)) {
			*p = entry->next;
			break;
		}
		p = &entry->next;
	}
	return entry;
}
