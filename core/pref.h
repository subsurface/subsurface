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

enum def_file_behavior {
	UNDEFINED_DEFAULT_FILE,
	LOCAL_DEFAULT_FILE,
	NO_DEFAULT_FILE,
	CLOUD_DEFAULT_FILE
};

typedef struct {
	bool dont_check_for_updates;
	const char *last_version_used;
	int next_check;
} update_manager_prefs_t;

typedef struct {
	const char *vendor;
	const char *product;
	const char *device;
	const char *device_name;
} dive_computer_prefs_t;

// NOTE: these enums are duplicated in mobile-widgets/qmlinterface.h
enum unit_system_values {
	METRIC,
	IMPERIAL,
	PERSONALIZE
};

// ********** PREFERENCES **********
// This struct is kept global for all of ssrf
// most of the fields are loaded from git as
// part of the dives, but some fields are loaded
// from local storage (QSettings)
// The struct is divided in groups (sorted)
// and elements within the group is sorted
//
// When adding items to this list, please keep
// the list sorted (easier to find something)
struct preferences {
	// ********** Animations **********
	int animation_speed;

	// ********** CloudStorage **********
	bool        cloud_auto_sync;
	const char *cloud_base_url;
	const char *cloud_git_url;
	const char *cloud_storage_email;
	const char *cloud_storage_email_encoded;
	const char *cloud_storage_password;
	const char *cloud_storage_pin;
	int         cloud_timeout;
	int         cloud_verification_status;
	bool        save_password_local;

	// ********** DiveComputer **********
	dive_computer_prefs_t dive_computer;
	dive_computer_prefs_t dive_computer1;
	dive_computer_prefs_t dive_computer2;
	dive_computer_prefs_t dive_computer3;
	dive_computer_prefs_t dive_computer4;

	// ********** Display *************
	bool        display_invalid_dives;
	const char *divelist_font;
	double      font_size;
	double      mobile_scale;
	bool        show_developer;

	// ********** Equipment tab *******
	const char *default_cylinder;
	bool        display_unused_tanks;
	bool        display_default_tank_infos;

	// ********** General **********
	bool        auto_recalculate_thumbnails;
	bool	    extract_video_thumbnails;
	int	    extract_video_thumbnails_position; // position in stream: 0=first 100=last second
	const char *ffmpeg_executable; // path of ffmpeg binary
	int         defaultsetpoint; // default setpoint in mbar
	const char *default_filename;
	enum def_file_behavior default_file_behavior;
	int         o2consumption; // ml per min
	int         pscr_ratio; // dump ratio times 1000
	bool        use_default_file;
	bool        extraEnvironmentalDefault;
	bool        salinityEditDefault;

	// ********** Geocoding **********
	geocoding_prefs_t geocoding;

	// ********** Language **********
	const char     *date_format;
	bool            date_format_override;
	const char     *date_format_short;
	locale_prefs_t  locale; //: TODO: move the rest of locale based info here.
	const char     *time_format;
	bool            time_format_override;

	// ********** LocationService **********
	int time_threshold;
	int distance_threshold;

	// ********** Network **********
	bool        proxy_auth;
	const char *proxy_host;
	int         proxy_port;
	int         proxy_type;
	const char *proxy_user;
	const char *proxy_pass;

	// ********** Planner **********
	int             ascratelast6m;
	int             ascratestops;
	int             ascrate50;
	int             ascrate75; // All rates in mm / sec
	depth_t         bestmixend;
	int             bottompo2;
	int             bottomsac;
	int             decopo2;
	int             decosac;
	int             descrate;
	bool            display_duration;
	bool            display_runtime;
	bool            display_transitions;
	bool            display_variations;
	bool            doo2breaks;
	bool            dobailout;
	bool		o2narcotic;
	bool            drop_stone_mode;
	bool            last_stop;   // At 6m?
	int             min_switch_duration; // seconds
	int             surface_segment; // seconds at the surface after planned dive
	enum deco_mode  planner_deco_mode;
	int             problemsolvingtime;
	int             reserve_gas;
	int             sacfactor;
	bool            safetystop;
	bool            switch_at_req_stop;
	bool            verbatim_plan;

	// ********** TecDetails **********
	bool                        calcalltissues;
	bool                        calcceiling;
	bool                        calcceiling3m;
	bool                        calcndltts;
	bool                        decoinfo; // Show deco info in infobox
	bool                        dcceiling;
	enum deco_mode              display_deco_mode;
	bool                        ead;
	int                         gfhigh;
	int                         gflow;
	bool                        gf_low_at_maxdepth;
	bool                        hrgraph;
	bool                        mod;
	double                      modpO2;
	bool                        percentagegraph;
	partial_pressure_graphs_t   pp_graphs;
	bool                        redceiling;
	bool                        rulergraph;
	bool                        show_average_depth;
	bool                        show_ccr_sensors;
	bool                        show_ccr_setpoint;
	bool                        show_icd;
	bool                        show_pictures_in_profile;
	bool                        show_sac;
	bool                        show_scr_ocpo2;
	bool                        tankbar;
	int                         vpmb_conservatism;
	bool                        zoomed_plot;

	// ********** Units **********
	bool                    coordinates_traditional;
	enum unit_system_values unit_system;
	struct units            units;

	// ********** UpdateManager **********
	update_manager_prefs_t update_manager;
};

extern struct preferences prefs, default_prefs, git_prefs;

extern const char *system_divelist_default_font;
extern double system_divelist_default_font_size;

extern const char *system_default_directory(void);
extern const char *system_default_filename();
extern bool subsurface_ignore_font(const char *font);
extern void subsurface_OS_pref_setup();
extern void copy_prefs(struct preferences *src, struct preferences *dest);

extern void set_informational_units(const char *units);
extern void set_git_prefs(const char *prefs);

#ifdef __cplusplus
}
#endif

#endif // PREF_H
