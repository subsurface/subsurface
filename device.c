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

/*
 * Sort the device_info list, so that we write it out
 * in a stable order. Otherwise we'll end up having the
 * XML file have the devices listed in some arbitrary
 * order, which is annoying.
 */
static void add_entry_sorted(struct device_info *entry)
{
	struct device_info *p, **pp = &device_info_list;

	while ((p = *pp) != NULL) {
		int cmp = strcmp(p->model, entry->model);
		if (cmp > 0)
			break;
		if (!cmp && p->deviceid > entry->deviceid)
			break;
		pp = &p->next;
	}

	entry->next = p;
	*pp = entry;
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
		add_entry_sorted(entry);
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

struct divecomputer* fake_dc(struct divecomputer* dc)
{
	static struct sample fake[4];
	static struct divecomputer fakedc;
	static bool initialized = 0;
	if (!initialized){
		fakedc = (*dc);
		fakedc.sample = fake;
		fakedc.samples = 4;

		/* The dive has no samples, so create a few fake ones.  This assumes an
		ascent/descent rate of 9 m/min, which is just below the limit for FAST. */
		int duration = dc->duration.seconds;
		int maxdepth = dc->maxdepth.mm;
		int asc_desc_time = dc->maxdepth.mm*60/9000;
		if (asc_desc_time * 2 >= duration)
			asc_desc_time = duration / 2;
		fake[1].time.seconds = asc_desc_time;
		fake[1].depth.mm = maxdepth;
		fake[2].time.seconds = duration - asc_desc_time;
		fake[2].depth.mm = maxdepth;
		fake[3].time.seconds = duration * 1.00;
		fakedc.events = dc->events;
	}
	return &fakedc;
}

