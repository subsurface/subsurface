// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_H
#define DIVE_H

// dive and dive computer related structures and helpers

#include "divemode.h"
#include "divecomputer.h"
#include "equipment.h"
#include "picture.h" // TODO: remove
#include "tag.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

extern int last_xml_version;

extern const char *divemode_text_ui[];
extern const char *divemode_text[];

struct dive_site;
struct dive_table;
struct dive_trip;
struct full_text_cache;
struct event;
struct trip_table;

/* A unique_ptr that will not be copied if the parent class is copied.
 * This is used to keep a pointer to the fulltext cache and avoid
 * having it copied when the dive is copied, since the new dive is
 * not (yet) registered in the fulltext system. Quite hackish.
 */
template<typename T>
struct non_copying_unique_ptr : public std::unique_ptr<T> {
	using std::unique_ptr<T>::unique_ptr;
	using std::unique_ptr<T>::operator=;
	non_copying_unique_ptr(const non_copying_unique_ptr<T> &) { }
	void operator=(const non_copying_unique_ptr<T> &) { }
};

struct dive {
	struct dive_trip *divetrip = nullptr;
	timestamp_t when = 0;
	struct dive_site *dive_site = nullptr;
	std::string notes;
	std::string diveguide, buddy;
	std::string suit;
	cylinder_table cylinders;
	weightsystem_table weightsystems;
	int number = 0;
	int rating = 0;
	int wavesize = 0, current = 0, visibility = 0, surge = 0, chill = 0; /* 0 - 5 star ratings */
	int sac = 0, otu = 0, cns = 0, maxcns = 0;

	/* Calculated based on dive computer data */
	temperature_t mintemp, maxtemp, watertemp, airtemp;
	depth_t maxdepth, meandepth;
	pressure_t surface_pressure;
	duration_t duration;
	int salinity = 0; // kg per 10000 l
	int user_salinity = 0; // water density reflecting a user-specified type

	tag_list tags;
	std::vector<divecomputer> dcs; // Attn: pointers to divecomputers are not stable!
	int id = 0; // unique ID for this dive
	picture_table pictures;
	unsigned char git_id[20] = {};
	bool notrip = false; /* Don't autogroup this dive to a trip */
	bool selected = false;
	bool hidden_by_filter = false;
	non_copying_unique_ptr<full_text_cache> full_text; /* word cache for full text search */
	bool invalid = false;

	dive();
	~dive();
	dive(const dive &);
	dive(dive &&);
	dive &operator=(const dive &);

	void fixup_no_cylinder();		/* to fix cylinders, we need the divelist (to calculate cns) */
	timestamp_t endtime() const;		/* maximum over divecomputers (with samples) */
	duration_t totaltime() const;		/* maximum over divecomputers (with samples) */
	temperature_t dc_airtemp() const;	/* average over divecomputers */
	temperature_t dc_watertemp() const;	/* average over divecomputers */

	struct get_maximal_gas_result { int o2_p; int he_p; int o2low_p; };
	get_maximal_gas_result get_maximal_gas() const;

	bool is_planned() const;
	bool is_logged() const;
	bool likely_same(const struct dive &b) const;

	int depth_to_mbar(int depth) const;
	double depth_to_mbarf(int depth) const;
	double depth_to_bar(int depth) const;
	double depth_to_atm(int depth) const;
	int rel_mbar_to_depth(int mbar) const;
	int mbar_to_depth(int mbar) const;

	pressure_t calculate_surface_pressure() const;
	pressure_t un_fixup_surface_pressure() const;

	/* Don't call directly, use dive_table::merge_dives()! */
	static std::unique_ptr<dive> create_merged_dive(const struct dive &a, const struct dive &b, int offset, bool prefer_downloaded);
};

/* For the top-level list: an entry is either a dive or a trip */
struct dive_or_trip {
	struct dive *dive;
	struct dive_trip *trip;
};

extern void invalidate_dive_cache(struct dive *dive);
extern bool dive_cache_is_valid(const struct dive *dive);

extern int get_cylinder_idx_by_use(const struct dive *dive, enum cylinderuse cylinder_use_type);
extern void cylinder_renumber(struct dive &dive, int mapping[]);
extern int same_gasmix_cylinder(const cylinder_t &cyl, int cylid, const struct dive *dive, bool check_unused);

/* when selectively copying dive information, which parts should be copied? */
struct dive_components {
	unsigned int divesite : 1;
	unsigned int notes : 1;
	unsigned int diveguide : 1;
	unsigned int buddy : 1;
	unsigned int suit : 1;
	unsigned int rating : 1;
	unsigned int visibility : 1;
	unsigned int wavesize : 1;
	unsigned int current : 1;
	unsigned int surge : 1;
	unsigned int chill : 1;
	unsigned int tags : 1;
	unsigned int cylinders : 1;
	unsigned int weights : 1;
	unsigned int number : 1;
	unsigned int when : 1;
};

extern bool has_gaschange_event(const struct dive *dive, const struct divecomputer *dc, int idx);
extern int explicit_first_cylinder(const struct dive *dive, const struct divecomputer *dc);

extern fraction_t best_o2(depth_t depth, const struct dive *dive, bool in_planner);
extern fraction_t best_he(depth_t depth, const struct dive *dive, bool o2narcotic, fraction_t fo2);

extern int get_surface_pressure_in_mbar(const struct dive *dive, bool non_null);
extern depth_t gas_mod(struct gasmix mix, pressure_t po2_limit, const struct dive *dive, int roundto);
extern depth_t gas_mnd(struct gasmix mix, depth_t end, const struct dive *dive, int roundto);

extern struct dive_site *get_dive_site_for_dive(const struct dive *dive);
extern std::string get_dive_country(const struct dive *dive);
extern std::string get_dive_location(const struct dive *dive);
extern unsigned int number_of_computers(const struct dive *dive);
extern struct divecomputer *get_dive_dc(struct dive *dive, int nr);
extern const struct divecomputer *get_dive_dc(const struct dive *dive, int nr);

extern std::unique_ptr<dive> clone_make_first_dc(const struct dive &d, int dc_number);
extern std::unique_ptr<dive> clone_delete_divecomputer(const struct dive &d, int dc_number);

extern bool dive_site_has_gps_location(const struct dive_site *ds);
extern int dive_has_gps_location(const struct dive *dive);
extern location_t dive_get_gps_location(const struct dive *d);

extern bool time_during_dive_with_offset(const struct dive *dive, timestamp_t when, timestamp_t offset);

extern int save_dives(const char *filename);
extern int save_dives_logic(const char *filename, bool select_only, bool anonymize);
extern int save_dive(FILE *f, const struct dive &dive, bool anonymize);
extern int export_dives_xslt(const char *filename, bool selected, const int units, const char *export_xslt, bool anonymize);

extern int save_dive_sites_logic(const char *filename, const struct dive_site *sites[], int nr_sites, bool anonymize);

struct membuffer;
extern void save_one_dive_to_mb(struct membuffer *b, const struct dive &dive, bool anonymize);

extern void subsurface_console_init();
extern void subsurface_console_exit();
extern bool subsurface_user_is_root();

extern void clear_dive(struct dive *dive);
extern void copy_dive(const struct dive *s, struct dive *d);
extern void selective_copy_dive(const struct dive *s, struct dive *d, struct dive_components what, bool clear);

extern int legacy_format_o2pressures(const struct dive *dive, const struct divecomputer *dc);

extern bool dive_less_than(const struct dive &a, const struct dive &b);
extern bool dive_less_than_ptr(const struct dive *a, const struct dive *b);
extern bool dive_or_trip_less_than(struct dive_or_trip a, struct dive_or_trip b);
extern int get_dive_salinity(const struct dive *dive);
extern int dive_getUniqID();

extern void copy_events_until(const struct dive *sd, struct dive *dd, int dcNr, int time);
extern void copy_used_cylinders(const struct dive *s, struct dive *d, bool used_only);
extern bool is_cylinder_used(const struct dive *dive, int idx);
extern bool is_cylinder_prot(const struct dive *dive, int idx);
extern void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int time, int idx);
extern struct event create_gas_switch_event(struct dive *dive, struct divecomputer *dc, int seconds, int idx);
extern void per_cylinder_mean_depth(const struct dive *dive, struct divecomputer *dc, int *mean, int *duration);
extern int get_cylinder_index(const struct dive *dive, const struct event &ev);
extern struct gasmix get_gasmix_from_event(const struct dive *, const struct event &ev);
extern bool cylinder_with_sensor_sample(const struct dive *dive, int cylinder_id);

/* UI related protopypes */

extern void invalidate_dive_cache(struct dive *dc);

extern int total_weight(const struct dive *);

/* Get gasmix at a given time */
extern struct gasmix get_gasmix_at_time(const struct dive &dive, const struct divecomputer &dc, duration_t time);

extern void update_setpoint_events(const struct dive *dive, struct divecomputer *dc);

/* Make pointers to dive and dive_trip "Qt metatypes" so that they can be passed through
 * QVariants and through QML.
 */
#include <QObject>
Q_DECLARE_METATYPE(struct dive *);

extern std::string existing_filename;

#endif // DIVE_H
