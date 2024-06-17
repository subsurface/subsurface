// SPDX-License-Identifier: GPL-2.0
#ifndef PARSE_H
#define PARSE_H

#include "event.h"
#include "equipment.h" // for cylinder_t
#include "extradata.h"
#include "filterpreset.h"
#include "picture.h"

#include <memory>
#include <sqlite3.h>
#include <string>
#include <time.h>
#include <vector>

struct xml_params;
struct divelog;
struct fingerprint_record;

/*
 * Dive info as it is being built up..
 */

struct parser_settings {
	struct {
		std::string model;
		uint32_t deviceid = 0;
		std::string nickname, serial_nr, firmware;
	} dc;
	struct {
		 uint32_t model;
		 uint32_t serial;
		 uint32_t fdeviceid;
		 uint32_t fdiveid;
		 std::string data;
	} fingerprint;
};

/*
 * parser_state is the state needed by the parser(s). It is initialized
 * "owning" marks pointers to objects that are freed in the destructor.
 * In contrast, "non-owning" marks pointers to objects that are owned
 * by other data-structures.
 */
struct parser_state {
	enum import_source {
		UNKNOWN,
		LIBDIVECOMPUTER,
		DIVINGLOG,
		UDDF,
	};

	bool metric = true;
	struct parser_settings cur_settings;
	enum import_source import_source = UNKNOWN;

	struct divecomputer *cur_dc = nullptr;			/* non-owning */
	std::unique_ptr<dive> cur_dive;				/* owning */
	std::unique_ptr<dive_site> cur_dive_site;		/* owning */
	location_t cur_location;
	std::unique_ptr<dive_trip> cur_trip;			/* owning */
	struct sample *cur_sample = nullptr;			/* non-owning */
	struct picture cur_picture;				/* owning */
	std::unique_ptr<filter_preset> cur_filter;		/* owning */
	std::string fulltext;					/* owning */
	std::string fulltext_string_mode;			/* owning */
	std::string filter_constraint_type;			/* owning */
	std::string filter_constraint_string_mode;		/* owning */
	std::string filter_constraint_range_mode;		/* owning */
	bool filter_constraint_negate = false;
	std::string filter_constraint;				/* owning */
	std::string country, city;				/* owning */
	int taxonomy_category = 0, taxonomy_origin = 0;

	bool in_settings = false;
	bool in_userid = false;
	bool in_fulltext = false;
	bool in_filter_constraint = false;
	struct tm cur_tm{ 0 };
	int lastcylinderindex = 0, next_o2_sensor = 0;
	int o2pressure_sensor = 0;
	int sample_rate = 0;
	struct { std::string key; std::string value; } cur_extra_data;
	struct units xml_parsing_units;
	struct divelog *log = nullptr;				/* non-owning */
	std::vector<fingerprint_record> *fingerprints = nullptr;
								/* non-owning */

	sqlite3 *sql_handle = nullptr;				/* for SQL based parsers */
	bool event_active = false;
	event cur_event;
	parser_state();
	~parser_state();
};

void event_start(struct parser_state *state);
void event_end(struct parser_state *state);
struct divecomputer *get_dc(struct parser_state *state);

bool is_dive(struct parser_state *state);
void reset_dc_info(struct divecomputer *dc, struct parser_state *state);
void reset_dc_settings(struct parser_state *state);
void settings_start(struct parser_state *state);
void settings_end(struct parser_state *state);
void fingerprint_settings_start(struct parser_state *state);
void fingerprint_settings_end(struct parser_state *state);
void dc_settings_start(struct parser_state *state);
void dc_settings_end(struct parser_state *state);
void dive_site_start(struct parser_state *state);
void dive_site_end(struct parser_state *state);
void dive_start(struct parser_state *state);
void dive_end(struct parser_state *state);
void filter_preset_start(struct parser_state *state);
void filter_preset_end(struct parser_state *state);
void filter_constraint_start(struct parser_state *state);
void filter_constraint_end(struct parser_state *state);
void fulltext_start(struct parser_state *state);
void fulltext_end(struct parser_state *state);
void trip_start(struct parser_state *state);
void trip_end(struct parser_state *state);
void picture_start(struct parser_state *state);
void picture_end(struct parser_state *state);
cylinder_t *cylinder_start(struct parser_state *state);
void cylinder_end(struct parser_state *state);
void ws_start(struct parser_state *state);
void ws_end(struct parser_state *state);

void sample_start(struct parser_state *state);
void sample_end(struct parser_state *state);
void divecomputer_start(struct parser_state *state);
void divecomputer_end(struct parser_state *state);
void userid_start(struct parser_state *state);
void userid_stop(struct parser_state *state);
void utf8_string_std(const char *buffer, std::string *res);

void add_dive_site(const char *ds_name, struct dive *dive, struct parser_state *state);

int trimspace(char *buffer);
void start_match(const char *type, const char *name, char *buffer);
void nonmatch(const char *type, const char *name, char *buffer);

void parse_xml_init();
int parse_xml_buffer(const char *url, const char *buf, int size, struct divelog *log, const struct xml_params *params);
void parse_xml_exit();
int parse_dm4_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct divelog *log);
int parse_dm5_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct divelog *log);
int parse_seac_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct divelog *log);
int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct divelog *log);
int parse_shearwater_cloud_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct divelog *log);
int parse_cobalt_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct divelog *log);
int parse_divinglog_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct divelog *log);
int parse_dlf_buffer(unsigned char *buffer, size_t size, struct divelog *log);
std::string trimspace(const char *buffer);

#endif
