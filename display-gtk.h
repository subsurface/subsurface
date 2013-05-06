#ifndef DISPLAY_GTK_H
#define DISPLAY_GTK_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(2,22,0)
#include <gdk/gdkkeysyms-compat.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern GtkWidget *main_window;

/* we want a progress bar as part of the device_data_t - let's abstract this out */
typedef struct {
	GtkWidget *bar;
} progressbar_t;

#if defined __APPLE__
#define CTRLCHAR "<Meta>"
#define SHIFTCHAR "<Shift>"
#define PREFERENCE_ACCEL "<Meta>comma"
#else
#define CTRLCHAR "<Control>"
#define SHIFTCHAR "<Shift>"
#define PREFERENCE_ACCEL NULL
#endif

extern int subsurface_fill_device_list(GtkListStore *store);
extern const char *subsurface_icon_name(void);
extern void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager);
extern gboolean on_delete(GtkWidget* w, gpointer data);

extern void set_divelist_font(const char *);

extern void update_screen(void);
extern void download_dialog(GtkWidget *, gpointer);

extern void add_dive_cb(GtkWidget *, gpointer);
extern void update_progressbar(progressbar_t *progress, double value);
extern void update_progressbar_text(progressbar_t *progress, const char *text);


// info.c
enum {
	MATCH_EXACT,
	MATCH_PREPEND,
	MATCH_AFTER
} found_string_entry;

extern GtkWidget *create_date_time_widget(struct tm *time, GtkWidget **cal, GtkWidget **h, GtkWidget **m, GtkWidget **timehbox);
extern void add_string_list_entry(const char *string, GtkListStore *list);
extern int match_list(GtkListStore *list, const char *string);

extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern GtkWidget *equipment_widget(int w_idx);
extern GtkWidget *single_stats_widget(void);
extern GtkWidget *total_stats_widget(void);
extern int select_cylinder(struct dive *dive, int when);
extern GtkWidget *dive_list_create(void);
extern void dive_list_destroy(void);
extern void info_widget_destroy(void);
extern GdkPixbuf *get_gps_icon(void);

/* Helper functions for gtk combo boxes */
extern GtkEntry *get_entry(GtkComboBox *);
extern const char *get_active_text(GtkComboBox *);
extern void set_active_text(GtkComboBox *, const char *);
extern GtkWidget *combo_box_with_model_and_entry(GtkListStore *);

extern GtkWidget *create_label(const char *fmt, ...);

extern gboolean icon_click_cb(GtkWidget *w, GdkEventButton *event, gpointer data);

extern void process_selected_dives(void);

typedef void (*data_func_t)(GtkTreeViewColumn *col,
			    GtkCellRenderer *renderer,
			    GtkTreeModel *model,
			    GtkTreeIter *iter,
			    gpointer data);

typedef gint (*sort_func_t)(GtkTreeModel *model,
			    GtkTreeIter *a,
			    GtkTreeIter *b,
			    gpointer user_data);

extern GtkTreeViewColumn *tree_view_column(GtkWidget *tree_view, int index, const char *title,
		data_func_t data_func, unsigned int flags);
extern GtkTreeViewColumn *tree_view_column_add_pixbuf(GtkWidget *tree_view, data_func_t data_func, GtkTreeViewColumn *col);

GError *uemis_download(const char *path, progressbar_t *progress, GtkDialog *dialog, gboolean force_download);

/* from planner.c */
extern void input_plan(void);

#ifdef __cplusplus
}
#endif

#endif
