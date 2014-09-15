#ifndef PREF_H
#define PREF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "units.h"

/* can't use 'bool' for the boolean values - different size in C and C++ */
typedef struct
{
	short po2;
	short pn2;
	short phe;
	double po2_threshold;
	double pn2_threshold;
	double phe_threshold;
} partial_pressure_graphs_t;

struct preferences {
	const char *divelist_font;
	const char *default_filename;
	const char *default_cylinder;
	double font_size;
	partial_pressure_graphs_t pp_graphs;
	short mod;
	double modpO2;
	short ead;
	short dcceiling;
	short redceiling;
	short calcceiling;
	short calcceiling3m;
	short calcalltissues;
	short calcndltts;
	short gflow;
	short gfhigh;
	int animation_speed;
	bool gf_low_at_maxdepth;
	short display_invalid_dives;
	short unit_system;
	struct units units;
	short show_sac;
	short display_unused_tanks;
	short show_average_depth;
	short zoomed_plot;
	short hrgraph;
	short percentagegraph;
	short rulergraph;
	short tankbar;
	short save_userid_local;
	char *userid;
	int ascrate75;
	int ascrate50;
	int ascratestops;
	int ascratelast6m;
	int descrate;
	int bottompo2;
	int decopo2;
	int proxy_type;
	char *proxy_host;
	int proxy_port;
	short proxy_auth;
	char *proxy_user;
	char *proxy_pass;
	bool doo2breaks;
	bool drop_stone_mode;
	int bottomsac;
	int decosac;
	bool show_pictures_in_profile;
	bool use_default_file;
};
enum unit_system_values {
	METRIC,
	IMPERIAL,
	PERSONALIZE
};

extern struct preferences prefs, default_prefs;

#define PP_GRAPHS_ENABLED (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe)

extern const char *system_divelist_default_font;
extern double system_divelist_default_font_size;
extern const char *system_default_filename();
extern bool subsurface_ignore_font(const char *font);
extern void subsurface_OS_pref_setup();

#ifdef __cplusplus
}
#endif

#endif // PREF_H
