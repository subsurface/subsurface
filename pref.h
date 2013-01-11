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
};

extern struct preferences prefs, default_prefs;

#define PP_GRAPHS_ENABLED (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe)

typedef enum {
	PREF_BOOL,
	PREF_STRING
} pref_type_t;

#define BOOL_TO_PTR(_cond) ((_cond) ? (void *)1 : NULL)
#define PTR_TO_BOOL(_ptr) ((_ptr) != NULL)

extern void subsurface_open_conf(void);
extern void subsurface_set_conf(char *name, pref_type_t type, const void *value);
extern void subsurface_unset_conf(char *name);
extern const void *subsurface_get_conf(char *name, pref_type_t type);
extern void subsurface_flush_conf(void);
extern void subsurface_close_conf(void);

#endif /* PREF_H */
