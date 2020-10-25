// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTER_H
#define DIVECOMPUTER_H

#include "divemode.h"
#include "units.h"

#ifdef __cplusplus
extern "C" {
#endif

struct extra_data;
struct sample;

/* Is this header the correct place? */
#define SURFACE_THRESHOLD 750 /* somewhat arbitrary: only below 75cm is it really diving */

/*
 * NOTE! The deviceid and diveid are model-specific *hashes* of
 * whatever device identification that model may have. Different
 * dive computers will have different identifying data, it could
 * be a firmware number or a serial ID (in either string or in
 * numeric format), and we do not care.
 *
 * The only thing we care about is that subsurface will hash
 * that information the same way. So then you can check the ID
 * of a dive computer by comparing the hashes for equality.
 *
 * A deviceid or diveid of zero is assumed to be "no ID".
 */
struct divecomputer {
	timestamp_t when;
	duration_t duration, surfacetime, last_manual_time;
	depth_t maxdepth, meandepth;
	temperature_t airtemp, watertemp;
	pressure_t surface_pressure;
	enum divemode_t divemode;	// dive computer type: OC(default) or CCR
	uint8_t no_o2sensors;		// rebreathers: number of O2 sensors used
	int salinity; 			// kg per 10000 l
	const char *model, *serial, *fw_version;
	uint32_t deviceid, diveid;
	int samples, alloc_samples;
	struct sample *sample;
	struct event *events;
	struct extra_data *extra_data;
	struct divecomputer *next;
};

extern void fake_dc(struct divecomputer *dc);
extern void free_dc(struct divecomputer *dc);
extern void free_dc_contents(struct divecomputer *dc);
extern enum divemode_t get_current_divemode(const struct divecomputer *dc, int time, const struct event **evp, enum divemode_t *divemode);
extern int get_depth_at_time(const struct divecomputer *dc, unsigned int time);
extern void free_dive_dcs(struct divecomputer *dc);
extern void alloc_samples(struct divecomputer *dc, int num);
extern void free_samples(struct divecomputer *dc);
extern struct sample *prepare_sample(struct divecomputer *dc);
extern void finish_sample(struct divecomputer *dc);
extern struct sample *add_sample(const struct sample *sample, int time, struct divecomputer *dc);
extern void fixup_dc_duration(struct divecomputer *dc);
extern unsigned int dc_airtemp(const struct divecomputer *dc);
extern unsigned int dc_watertemp(const struct divecomputer *dc);
extern void copy_events(const struct divecomputer *s, struct divecomputer *d);
extern void swap_event(struct divecomputer *dc, struct event *from, struct event *to);
extern void copy_samples(const struct divecomputer *s, struct divecomputer *d);
extern void add_event_to_dc(struct divecomputer *dc, struct event *ev);
extern struct event *add_event(struct divecomputer *dc, unsigned int time, int type, int flags, int value, const char *name);
extern void remove_event_from_dc(struct divecomputer *dc, struct event *event);
extern void add_extra_data(struct divecomputer *dc, const char *key, const char *value);
extern bool is_dc_planner(const struct divecomputer *dc);

/* Check if two dive computer entries are the exact same dive (-1=no/0=maybe/1=yes) */
extern int match_one_dc(const struct divecomputer *a, const struct divecomputer *b);

#ifdef __cplusplus
}
#endif

#endif
