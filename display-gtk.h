#ifndef DISPLAY_GTK_H
#define DISPLAY_GTK_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

extern GtkWidget *main_window;

/* we want a progress bar as part of the device_data_t - let's abstract this out */
typedef struct {
	GtkWidget *bar;
} progressbar_t;

typedef struct {
	gboolean cylinder;
	gboolean temperature;
	gboolean totalweight;
	gboolean nitrox;
	gboolean sac;
	gboolean otu;
} visible_cols_t;

typedef enum {
	PREF_BOOL,
	PREF_STRING
} pref_type_t;

#define BOOL_TO_PTR(_cond) ((_cond) ? (void *)1 : NULL)
#define PTR_TO_BOOL(_ptr) ((_ptr) != NULL)

#if defined __APPLE__
#define CTRLCHAR "<Meta>"
#define PREFERENCE_ACCEL "<Meta>comma"
#else
#define CTRLCHAR "<Control>"
#define PREFERENCE_ACCEL NULL
#endif

extern void subsurface_open_conf(void);
extern void subsurface_set_conf(char *name, pref_type_t type, const void *value);
extern const void *subsurface_get_conf(char *name, pref_type_t type);
extern void subsurface_flush_conf(void);
extern void subsurface_close_conf(void);

extern const char *subsurface_USB_name(void);
extern const char *subsurface_icon_name(void);
extern void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager);

extern const char *divelist_font;

extern visible_cols_t visible_cols;

extern const char *divelist_font;
extern void set_divelist_font(const char *);

extern void import_dialog(GtkWidget *, gpointer);
extern void add_dive_cb(GtkWidget *, gpointer);
extern void report_error(GError* error);
extern int process_ui_events(void);
extern void update_progressbar(progressbar_t *progress, double value);
extern void update_progressbar_text(progressbar_t *progress, const char *text);

extern GtkWidget *dive_profile_widget(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern GtkWidget *equipment_widget(void);
extern GtkWidget *single_stats_widget(void);
extern GtkWidget *total_stats_widget(void);
extern GtkWidget *cylinder_list_widget(void);

extern GtkWidget *dive_list_create(void);

unsigned int amount_selected;

extern void process_selected_dives(GList *, GtkTreeModel *);

typedef void (*data_func_t)(GtkTreeViewColumn *col,
			    GtkCellRenderer *renderer,
			    GtkTreeModel *model,
			    GtkTreeIter *iter,
			    gpointer data);

typedef gint (*sort_func_t)(GtkTreeModel *model,
			    GtkTreeIter *a,
			    GtkTreeIter *b,
			    gpointer user_data);

#define ALIGN_LEFT 1
#define ALIGN_RIGHT 2
#define INVISIBLE 4
#define UNSORTABLE 8

extern GtkTreeViewColumn *tree_view_column(GtkWidget *tree_view, int index, const char *title,
		data_func_t data_func, unsigned int flags);

#endif
