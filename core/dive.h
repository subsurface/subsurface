// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_H
#define DIVE_H

// dive and dive computer related structures and helpers

#include "divemode.h"
#include "divecomputer.h"
#include "equipment.h"
#include "event.h"
#include "picture.h" // TODO: remove
#include "tag.h"

#include <array>
#include <memory>
#include <optional>
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
	std::array<unsigned char, 20> git_id = {};
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

	void invalidate_cache();
	bool cache_is_valid() const;

	struct divecomputer *get_dc(int nr);
	const struct divecomputer *get_dc(int nr) const;

	void clear();
	int number_of_computers() const;
	void fixup_dive();
	void fixup_dive_dc(struct divecomputer &dc);
	timestamp_t endtime() const;		/* maximum over divecomputers (with samples) */
	duration_t totaltime() const;		/* maximum over divecomputers (with samples) */
	temperature_t dc_airtemp() const;	/* average over divecomputers */
	temperature_t dc_watertemp() const;	/* average over divecomputers */
	pressure_t get_surface_pressure() const;

	struct get_maximal_gas_result { int o2_p; int he_p; int o2low_p; };
	get_maximal_gas_result get_maximal_gas() const;

	bool is_planned() const;
	bool is_logged() const;
	bool likely_same(const struct dive &b) const;
	bool is_cylinder_used(int idx) const;
	bool is_cylinder_prot(int idx) const;
	int get_cylinder_index(const struct event &ev, const struct divecomputer &dc) const;
	bool has_gaschange_event(const struct divecomputer *dc, int idx) const;
	std::pair<const struct gasmix, divemode_t> get_gasmix_from_event(const struct event &ev, const struct divecomputer &dc) const;
	struct gasmix get_gasmix_at_time(const struct divecomputer &dc, duration_t time) const;
	cylinder_t *get_cylinder(int idx);
	cylinder_t *get_or_create_cylinder(int idx);
	const cylinder_t *get_cylinder(int idx) const;
	weight_t total_weight() const;
	int get_salinity() const;
	bool time_during_dive_with_offset(timestamp_t when, timestamp_t offset) const;
	std::string get_country() const;
	std::string get_location() const;

	int depth_to_mbar(depth_t depth) const;
	double depth_to_mbarf(depth_t depth) const;
	double depth_to_bar(depth_t depth) const;
	double depth_to_atm(depth_t depth) const;
	depth_t rel_mbar_to_depth(int mbar) const;
	depth_t mbar_to_depth(int mbar) const;

	pressure_t calculate_surface_pressure() const;
	pressure_t un_fixup_surface_pressure() const;
	depth_t gas_mod(struct gasmix mix, pressure_t po2_limit, depth_t roundto) const;
	depth_t gas_mnd(struct gasmix mix, depth_t end, depth_t roundto) const;
	fraction_t best_o2(depth_t depth, bool in_planner) const;
	fraction_t best_he(depth_t depth, bool o2narcotic, fraction_t fo2) const;

	struct depth_duration {
		depth_t depth;
		duration_t duration;
	};
	std::vector<depth_duration> per_cylinder_mean_depth_and_duration(int dc_nr) const;

	bool dive_has_gps_location() const;
	location_t get_gps_location() const;

	/* Don't call directly, use dive_table::merge_dives()! */
	static std::unique_ptr<dive> create_merged_dive(const struct dive &a, const struct dive &b, int offset, bool prefer_downloaded);
};

/* For the top-level list: an entry is either a dive or a trip */
struct dive_or_trip {
	struct dive *dive;
	struct dive_trip *trip;
};

extern void cylinder_renumber(struct dive &dive, int mapping[]);
extern int same_gasmix_cylinder(const cylinder_t &cyl, int cylid, const struct dive *dive, bool check_unused);
extern bool is_cylinder_use_appropriate(const struct divecomputer &dc, const cylinder_t &cyl, bool allowNonUsable);
extern divemode_t get_effective_divemode(const struct divecomputer &dc, const struct cylinder_t &cylinder);
extern std::tuple<divemode_t, int, const struct gasmix *> get_dive_status_at(const struct dive &dive, const struct divecomputer &dc, int seconds, divemode_loop *loop_mode = nullptr, gasmix_loop *loop_gas = nullptr);
extern const std::vector<struct tank_sensor_mapping> get_tank_sensor_mappings_for_storage(const struct dive &dive, const struct divecomputer &dc);

/* Data stored when copying a dive */
struct dive_paste_data {
	std::optional<uint32_t> divesite; // We save the uuid not a pointer, because the
					  // user might copy and then delete the dive site.
	std::optional<std::string> notes;
	std::optional<std::string> diveguide;
	std::optional<std::string> buddy;
	std::optional<std::string> suit;
	std::optional<int> rating;
	std::optional<int> visibility;
	std::optional<int> wavesize;
	std::optional<int> current;
	std::optional<int> surge;
	std::optional<int> chill;
	std::optional<tag_list> tags;
	std::optional<cylinder_table> cylinders;
	std::optional<weightsystem_table> weights;
	std::optional<int> number;
	std::optional<timestamp_t> when;
};

extern std::unique_ptr<dive> clone_make_first_dc(const struct dive &d, int dc_number);

extern int save_dives(const char *filename);
extern int save_dives_logic(const char *filename, bool select_only, bool anonymize);
extern int save_dive(FILE *f, const struct dive &dive, bool anonymize);
extern int export_dives_xslt(const char *filename, bool selected, const int units, const char *export_xslt, bool anonymize);

extern int save_dive_sites_logic(const char *filename, const struct dive_site *sites[], int nr_sites, bool anonymize);

struct membuffer;
extern void save_one_dive_to_mb(struct membuffer *b, const struct dive &dive, bool anonymize);

extern void copy_dive(const struct dive *s, struct dive *d);

extern int legacy_format_o2pressures(const struct dive *dive, const struct divecomputer *dc);

extern bool dive_less_than(const struct dive &a, const struct dive &b);
extern bool dive_less_than_ptr(const struct dive *a, const struct dive *b);
extern bool dive_or_trip_less_than(struct dive_or_trip a, struct dive_or_trip b);
extern int dive_getUniqID();

extern void copy_events_until(const struct dive *sd, struct dive *dd, int dcNr, int time);
extern void copy_used_cylinders(const struct dive *s, struct dive *d, bool used_only);
extern void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int time, int idx);
extern struct event create_gas_switch_event(struct dive *dive, struct divecomputer *dc, int seconds, int idx);
extern bool cylinder_with_sensor_sample(const struct dive *dive, int cylinder_id);

extern void update_setpoint_events(const struct dive *dive, struct divecomputer *dc);

extern int check_dc_cylinder_use(struct dive &dive, struct divecomputer &dc);

/* Make pointers to dive and dive_trip "Qt metatypes" so that they can be passed through
 * QVariants and through QML.
 */
#include <QObject>
Q_DECLARE_METATYPE(struct dive *);

extern std::string existing_filename;

#endif // DIVE_H
