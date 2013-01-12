#ifndef PREF_H
#define PREF_H

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
	gboolean profile_red_ceiling;
	gboolean profile_calc_ceiling;
	gboolean calc_ceiling_3m_incr;
	double gflow;
	double gfhigh;
	const char *divelist_font;
	const char *default_filename;
};

extern struct preferences prefs, default_prefs;

#define PP_GRAPHS_ENABLED (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe)

extern void subsurface_open_conf(void);
extern void subsurface_set_conf(char *name, const char *value);
extern void subsurface_set_conf_bool(char *name, gboolean value);
extern void subsurface_unset_conf(char *name);
extern const void *subsurface_get_conf(char *name);
extern int subsurface_get_conf_bool(char *name);
extern void subsurface_flush_conf(void);
extern void subsurface_close_conf(void);

extern const char system_divelist_default_font[];
extern const char *system_default_filename();

extern void load_preferences(void);
extern void save_preferences(void);

#endif /* PREF_H */
