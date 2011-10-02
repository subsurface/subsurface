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
	gboolean sac;
	gboolean otu;
} visible_cols_t;

extern visible_cols_t visible_cols;

extern const char *divelist_font;
extern void set_divelist_font(const char *);

extern void import_dialog(GtkWidget *, gpointer);
extern void report_error(GError* error);
extern int process_ui_events(void);
extern void update_progressbar(progressbar_t *progress, double value);

extern GtkWidget *dive_profile_widget(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern GtkWidget *equipment_widget(void);

extern GtkWidget *dive_list_create(void);

typedef void (*data_func_t)(GtkTreeViewColumn *col,
			    GtkCellRenderer *renderer,
			    GtkTreeModel *model,
			    GtkTreeIter *iter,
			    gpointer data);

extern GtkTreeViewColumn *tree_view_column(GtkWidget *tree_view, int index, const char *title,
		data_func_t data_func, PangoAlignment align, gboolean visible);

#endif
