// SPDX-License-Identifier: GPL-2.0
#ifndef PREF_H
#define PREF_H

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include "units.h"
#include "taxonomy.h"

typedef struct
{
	bool po2;
	bool pn2;
	bool phe;
	double po2_threshold_min;
	double po2_threshold_max;
	double pn2_threshold;
	double phe_threshold;
} partial_pressure_graphs_t;

typedef struct {
	const char *access_token;
	const char *user_id;
	const char *album_id;
} facebook_prefs_t;

typedef struct {
	enum taxonomy_category category[3];
} geocoding_prefs_t;

typedef struct {
	const char *language;
	const char *lang_locale;
	bool use_system_language;
} locale_prefs_t;

enum deco_mode {
	BUEHLMANN,
	RECREATIONAL,
	VPMB
};

typedef struct {
	bool dont_check_for_updates;
	bool dont_check_exists;
	const char *last_version_used;
	const char *next_check;
} update_manager_prefs_t;

typedef struct {
	const char *vendor;
	const char *product;
	const char *device;
	const char *device_name;
	int download_mode;
} dive_computer_prefs_t;

struct preferences {
	const char *divelist_font;
	const char *default_filename;
	const char *default_cylinder;
	const char *cloud_base_url;
	const char *cloud_git_url;
	const char *time_format;
	const char *date_format;
	const char *date_format_short;
	bool time_format_override;
	bool date_format_override;
	double font_size;
	partial_pressure_graphs_t pp_graphs;
	bool mod;
	double modpO2;
	bool ead;
	bool dcceiling;
	bool redceiling;
	bool calcceiling;
	bool calcceiling3m;
	bool calcalltissues;
	bool calcndltts;
	short gflow;
	short gfhigh;
	int animation_speed;
	bool gf_low_at_maxdepth;
	bool show_ccr_setpoint;
	bool show_ccr_sensors;
	bool display_invalid_dives;
	short unit_system;
	struct units units;
	bool coordinates_traditional;
	bool show_sac;
	bool display_unused_tanks;
	bool show_average_depth;
	bool zoomed_plot;
	bool hrgraph;
	bool percentagegraph;
	bool rulergraph;
	bool tankbar;
	bool save_userid_local;
	const char *userid;
	int ascrate75; // All rates in mm / sec
	int ascrate50;
	int ascratestops;
	int ascratelast6m;
	int descrate;
	int sacfactor;
	int problemsolvingtime;
	int bottompo2;
	int decopo2;
	enum deco_mode display_deco_mode;
	depth_t bestmixend;
	int proxy_type;
	const char *proxy_host;
	int proxy_port;
	bool proxy_auth;
	const char *proxy_user;
	const char *proxy_pass;
	bool doo2breaks;
	bool drop_stone_mode;
	bool last_stop;   // At 6m?
	bool verbatim_plan;
	bool display_runtime;
	bool display_duration;
	bool display_transitions;
	bool display_variations;
	bool safetystop;
	bool switch_at_req_stop;
	int reserve_gas;
	int min_switch_duration; // seconds
	int bottomsac;
	int decosac;
	int o2consumption; // ml per min
	int pscr_ratio; // dump ratio times 1000
	int defaultsetpoint; // default setpoint in mbar
	bool show_pictures_in_profile;
	bool use_default_file;
	short default_file_behavior;
	facebook_prefs_t facebook;
	const char *cloud_storage_password;
	const char *cloud_storage_newpassword;
	const char *cloud_storage_email;
	bool save_password_local;
	short cloud_verification_status;
	bool cloud_background_sync;
	geocoding_prefs_t geocoding;
	enum deco_mode planner_deco_mode;
	short vpmb_conservatism;
	int time_threshold;
	int distance_threshold;
	short cloud_timeout;
	locale_prefs_t locale; //: TODO: move the rest of locale based info here.
	update_manager_prefs_t update_manager;
	dive_computer_prefs_t dive_computer;
};
enum unit_system_values {
	METRIC,
	IMPERIAL,
	PERSONALIZE
};

enum def_file_behavior {
	UNDEFINED_DEFAULT_FILE,
	LOCAL_DEFAULT_FILE,
	NO_DEFAULT_FILE,
	CLOUD_DEFAULT_FILE
};

enum cloud_status {
	CS_UNKNOWN,
	CS_INCORRECT_USER_PASSWD,
	CS_NEED_TO_VERIFY,
	CS_VERIFIED,
	CS_NOCLOUD
};

extern struct preferences prefs, default_prefs, git_prefs;

#define PP_GRAPHS_ENABLED (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe)

extern const char *system_divelist_default_font;
extern double system_divelist_default_font_size;

extern const char *system_default_directory(void);
extern const char *system_default_filename();
extern bool subsurface_ignore_font(const char *font);
extern void subsurface_OS_pref_setup();
extern void copy_prefs(struct preferences *src, struct preferences *dest);

#ifdef __cplusplus
}
#endif

#endif // PREF_H
