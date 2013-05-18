#ifndef PREF_H
#define PREF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	gboolean cylinder;
	gboolean temperature;
	gboolean totalweight;
	gboolean suit;
	gboolean nitrox;
	gboolean sac;
	gboolean otu;
	gboolean maxcns;
} visible_cols_t;

typedef struct {
	gboolean po2;
	gboolean pn2;
	gboolean phe;
	double po2_threshold;
	double pn2_threshold;
	double phe_threshold;
} partial_pressure_graphs_t;

struct preferences {
	struct units units;
	visible_cols_t visible_cols;
	partial_pressure_graphs_t pp_graphs;
	gboolean mod;
	double mod_ppO2;
	gboolean ead;
	gboolean profile_red_ceiling;
	gboolean profile_calc_ceiling;
	gboolean calc_ceiling_3m_incr;
	double gflow;
	double gfhigh;
	int map_provider;
	const char *divelist_font;
	const char *default_filename;
        short display_invalid_dives;
};

extern struct preferences prefs, default_prefs;

#define PP_GRAPHS_ENABLED (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe)

extern void subsurface_open_conf(void);
extern void subsurface_set_conf(const char *name, const char *value);
extern void subsurface_set_conf_bool(const char *name, gboolean value);
extern void subsurface_set_conf_int(const char *name, int value);
extern void subsurface_unset_conf(const char *name);
extern const char *subsurface_get_conf(const char *name);
extern int subsurface_get_conf_bool(const char *name);
extern int subsurface_get_conf_int(const char *name);
extern void subsurface_flush_conf(void);
extern void subsurface_close_conf(void);

extern const char system_divelist_default_font[];
extern const char *system_default_filename();

extern void load_preferences(void);
extern void save_preferences(void);

#ifdef __cplusplus
}
#endif

#endif /* PREF_H */
