#ifndef DISPLAY_GTK_H
#define DISPLAY_GTK_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(2,22,0)
#include <gdk/gdkkeysyms-compat.h>
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
extern void quit(GtkWidget *w, gpointer data);
extern gboolean on_delete(GtkWidget* w, gpointer data);

extern const char *divelist_font;
extern void set_divelist_font(const char *);

extern void import_files(GtkWidget *, gpointer);
extern void update_screen(void);
extern void download_dialog(GtkWidget *, gpointer);
extern int is_default_dive_computer_device(const char *);
extern int is_default_dive_computer(const char *, const char *);
extern void add_dive_cb(GtkWidget *, gpointer);
extern void report_error(GError* error);
extern int process_ui_events(void);
extern void update_progressbar(progressbar_t *progress, double value);
extern void update_progressbar_text(progressbar_t *progress, const char *text);

extern const char *default_dive_computer_vendor;
extern const char *default_dive_computer_product;
extern const char *default_dive_computer_device;

// info.c
enum {
	MATCH_EXACT,
	MATCH_PREPEND,
	MATCH_AFTER
} found_string_entry;

extern GtkWidget *create_date_time_widget(struct tm *time, GtkWidget **cal, GtkWidget **h, GtkWidget **m);
extern void add_string_list_entry(const char *string, GtkListStore *list);
extern int match_list(GtkListStore *list, const char *string);

extern GtkWidget *dive_profile_widget(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern GtkWidget *equipment_widget(int w_idx);
extern GtkWidget *single_stats_widget(void);
extern GtkWidget *total_stats_widget(void);
extern GtkWidget *cylinder_list_widget(int w_idx);
extern GtkWidget *weightsystem_list_widget(int w_idx);

extern GtkWidget *dive_list_create(void);
extern void dive_list_destroy(void);

unsigned int amount_selected;

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

#define ALIGN_LEFT 1
#define ALIGN_RIGHT 2
#define INVISIBLE 4
#define UNSORTABLE 8
#define EDITABLE 16

extern GtkTreeViewColumn *tree_view_column(GtkWidget *tree_view, int index, const char *title,
		data_func_t data_func, unsigned int flags);

GError *uemis_download(const char *path, progressbar_t *progress, GtkDialog *dialog, gboolean force_download);

/* from planner.c */
extern void input_plan(void);

#endif
