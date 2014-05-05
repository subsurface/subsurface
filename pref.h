#ifndef PREF_H
#define PREF_H

#ifdef __cplusplus
extern "C" {
#endif

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
	double modppO2;
	short ead;
	short dcceiling;
	short redceiling;
	short calcceiling;
	short calcceiling3m;
	short calcalltissues;
	short calcndltts;
	short gflow;
	short gfhigh;
	short animation;
	bool gf_low_at_maxdepth;
	short display_invalid_dives;
	short unit_system;
	struct units units;
	short show_sac;
	short display_unused_tanks;
	short show_average_depth;
	short zoomed_plot;
	short hrgraph;
	short rulergraph;
	short save_userid_local;
	char *userid;
};
enum unit_system_values {
	METRIC,
	IMPERIAL,
	PERSONALIZE
};

extern struct preferences prefs, default_prefs;

#define PP_GRAPHS_ENABLED (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe)

extern void subsurface_set_conf(const char *name, const char *value);
extern void subsurface_set_conf_bool(const char *name, bool value);
extern void subsurface_set_conf_int(const char *name, int value);

extern const char system_divelist_default_font[];
extern const int system_divelist_default_font_size;
extern const char *system_default_filename();

extern void load_preferences(void);
extern void save_preferences(void);

#ifdef __cplusplus
}
#endif

#endif // PREF_H
