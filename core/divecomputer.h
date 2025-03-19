// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECOMPUTER_H
#define DIVECOMPUTER_H

#include "divemode.h"
#include "units.h"
#include <set>
#include <string>
#include <vector>

struct extra_data;
struct event;
struct sample;
struct tank_sensor_mapping;

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
	timestamp_t when = 0;
	duration_t duration, surfacetime, last_manual_time;
	depth_t maxdepth, meandepth;
	temperature_t airtemp, watertemp;
	pressure_t surface_pressure;
	enum divemode_t divemode = OC;	// dive computer type: OC(default) or CCR
	uint8_t no_o2sensors = 0;	// rebreathers: number of O2 sensors used
	int salinity = 0;		// kg per 10000 l
	std::string model, serial, fw_version;
	uint32_t deviceid = 0, diveid = 0;
	// Note: ve store samples, events and extra_data in std::vector<>s.
	// This means that pointers to these items are *not* stable.
	std::vector<struct sample> samples;
	std::vector<struct event> events;
	std::vector<struct extra_data> extra_data;
	std::vector<struct tank_sensor_mapping> tank_sensor_mappings;

	divecomputer();
	~divecomputer();
	divecomputer(const divecomputer &);
	divecomputer(divecomputer &&);
	divecomputer &operator=(const divecomputer &);
};

extern void fake_dc(struct divecomputer *dc);
extern void free_dc_contents(struct divecomputer *dc);
extern int get_depth_at_time(const struct divecomputer *dc, unsigned int time);
extern struct sample *prepare_sample(struct divecomputer *dc);
extern void append_sample(const struct sample &sample, struct divecomputer *dc);
extern std::set<int16_t> get_tank_sensor_ids(const struct divecomputer &dc);
extern void fixup_dc_sample_sensors(struct divecomputer &dc);
extern void fixup_dc_duration(struct divecomputer &dc);
extern int add_event_to_dc(struct divecomputer *dc, struct event ev); // event structure is consumed, returns index of inserted event
extern struct event *add_event(struct divecomputer *dc, unsigned int time, int type, int flags, int value, const std::string &name);
extern struct event remove_event_from_dc(struct divecomputer *dc, int idx);
struct event *get_event(struct divecomputer *dc, int idx);
extern void add_extra_data(struct divecomputer *dc, const std::string &key, const std::string &value);
extern uint32_t calculate_string_hash(const char *str);
extern bool is_dc_planner(const struct divecomputer *dc);
extern void make_planner_dc(struct divecomputer *dc);
extern const char *manual_dc_name;
extern bool is_dc_manually_added_dive(const struct divecomputer *dc);
extern void make_manually_added_dive_dc(struct divecomputer *dc);

/* Check if two dive computer entries are the exact same dive (-1=no/0=maybe/1=yes) */
extern int match_one_dc(const struct divecomputer &a, const struct divecomputer &b);

#endif
