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
	double mod_ppO2;
	short ead;
	short profile_dc_ceiling;
	short profile_red_ceiling;
	short profile_calc_ceiling;
	short calc_ceiling_3m_incr;
	short calc_all_tissues;
	short calc_ndl_tts;
	short gflow;
	short gfhigh;
	bool gf_low_at_maxdepth;
	short display_invalid_dives;
	short unit_system;
	struct units units;
	short show_sac;
	bool display_unused_tanks;
	bool show_average_depth;
	bool zoomed_plot;
};
enum unit_system_values {
	METRIC,
	IMPERIAL,
	PERSONALIZE
};

extern struct preferences prefs, default_prefs;

#define PP_GRAPHS_ENABLED (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe)

extern void subsurface_open_conf(void);
extern void subsurface_set_conf(const char *name, const char *value);
extern void subsurface_set_conf_bool(const char *name, bool value);
extern void subsurface_set_conf_int(const char *name, int value);
extern void subsurface_unset_conf(const char *name);
extern const char *subsurface_get_conf(const char *name);
extern int subsurface_get_conf_bool(const char *name);
extern int subsurface_get_conf_int(const char *name);
extern void subsurface_flush_conf(void);
extern void subsurface_close_conf(void);

extern const char system_divelist_default_font[];
extern const int system_divelist_default_font_size;
extern const char *system_default_filename();

extern void load_preferences(void);
extern void save_preferences(void);

#ifdef __cplusplus
}
#endif

#endif // PREF_H
