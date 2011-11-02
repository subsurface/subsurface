/* equipment.c */
/* creates the UI for the equipment page -
 * controlled through the following interfaces:
 *
 * void show_dive_equipment(struct dive *dive)
 * void flush_dive_equipment_changes(struct dive *dive)
 *
 * called from gtk-ui:
 * GtkWidget *equipment_widget(void)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"
#include "divelist.h"

static GtkListStore *cylinder_model;

enum {
	CYL_DESC,
	CYL_SIZE,
	CYL_WORKP,
	CYL_STARTP,
	CYL_ENDP,
	CYL_O2,
	CYL_HE,
	CYL_COLUMNS
};

static struct {
	int max_index;
	GtkListStore *model;
	GtkWidget *tree_view;
	GtkWidget *edit, *add, *del;
	GtkTreeViewColumn *desc, *size, *workp, *startp, *endp, *o2, *he;
} cylinder_list;

struct cylinder_widget {
	int index, changed;
	const char *name;
	GtkWidget *hbox;
	GtkComboBox *description;
	GtkSpinButton *size, *pressure, *start, *end;
	GtkWidget *o2, *gasmix_button;
};

/* we want bar - so let's not use our unit functions */
static int convert_pressure(int mbar, double *p)
{
	int decimals = 1;
	double pressure;

	if (output_units.pressure == PSI) {
		pressure = mbar_to_PSI(mbar);
		decimals = 0;
	} else {
		pressure = mbar / 1000.0;
	}

	*p = pressure;
	return decimals;
}

static int convert_volume_pressure(int ml, int mbar, double *v, double *p)
{
	int decimals = 1;
	double volume, pressure;

	if (mbar) {
		if (output_units.volume == CUFT) {
			volume = ml_to_cuft(ml);
			volume *= bar_to_atm(mbar / 1000.0);
		} else
			volume = ml / 1000.0;

		if (output_units.pressure == PSI) {
			pressure = mbar_to_PSI(mbar);
			decimals = 0;
		} else
			pressure = mbar / 1000.0;
	}
	*v = volume;
	*p = pressure;
	return decimals;
}

static void set_cylinder_type_spinbuttons(struct cylinder_widget *cylinder, int ml, int mbar)
{
	double volume, pressure;

	convert_volume_pressure(ml, mbar, &volume, &pressure);
	gtk_spin_button_set_value(cylinder->size, volume);
	gtk_spin_button_set_value(cylinder->pressure, pressure);
}

static void set_cylinder_pressure_spinbuttons(struct cylinder_widget *cylinder, int start, int end)
{
	double pressure;

	convert_pressure(start, &pressure);
	gtk_spin_button_set_value(cylinder->start, pressure);
	convert_pressure(end, &pressure);
	gtk_spin_button_set_value(cylinder->end, pressure);
}

/*
 * The gtk_tree_model_foreach() interface is bad. It could have
 * returned whether the callback ever returned true
 */
static GtkTreeIter *found_match = NULL;
static GtkTreeIter match_iter;

static gboolean match_cylinder(GtkTreeModel *model,
				GtkTreePath *path,
				GtkTreeIter *iter,
				gpointer data)
{
	int match;
	gchar *name;
	const char *desc = data;

	gtk_tree_model_get(model, iter, 0, &name, -1);
	match = !strcmp(desc, name);
	g_free(name);
	if (match) {
		match_iter = *iter;
		found_match = &match_iter;
	}
	return match;
}

static int get_active_cylinder(GtkComboBox *combo_box, GtkTreeIter *iter)
{
	char *desc;

	if (gtk_combo_box_get_active_iter(combo_box, iter))
		return TRUE;

	desc = gtk_combo_box_get_active_text(combo_box);

	found_match = NULL;
	gtk_tree_model_foreach(GTK_TREE_MODEL(cylinder_model), match_cylinder, (void *)desc);

	g_free(desc);
	if (!found_match)
		return FALSE;

	*iter = *found_match;
	gtk_combo_box_set_active_iter(combo_box, iter);
	return TRUE;
}

static void cylinder_cb(GtkComboBox *combo_box, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo_box);
	int ml, mbar;
	struct cylinder_widget *cylinder = data;
	cylinder_t *cyl = current_dive->cylinder + cylinder->index;

	/* Did the user set it to some non-standard value? */
	if (!get_active_cylinder(combo_box, &iter)) {
		cylinder->changed = 1;
		return;
	}

	/*
	 * We get "change" signal callbacks just because we set
	 * the description by hand. Whatever. So ignore them if
	 * they are no-ops.
	 */
	if (!cylinder->changed && cyl->type.description) {
		int same;
		char *desc = gtk_combo_box_get_active_text(combo_box);

		same = !strcmp(desc, cyl->type.description);
		g_free(desc);
		if (same)
			return;
	}
	cylinder->changed = 1;

	gtk_tree_model_get(model, &iter,
		CYL_SIZE, &ml,
		CYL_WORKP, &mbar,
		-1);

	set_cylinder_type_spinbuttons(cylinder, ml, mbar);
}

static GtkTreeIter *add_cylinder_type(const char *desc, int ml, int mbar, GtkTreeIter *iter)
{
	GtkTreeModel *model;

	/* Don't even bother adding stuff without a size */
	if (!ml)
		return NULL;

	found_match = NULL;
	model = GTK_TREE_MODEL(cylinder_model);
	gtk_tree_model_foreach(model, match_cylinder, (void *)desc);

	if (!found_match) {
		GtkListStore *store = GTK_LIST_STORE(model);

		gtk_list_store_append(store, iter);
		gtk_list_store_set(store, iter,
			0, desc,
			1, ml,
			2, mbar,
			-1);
		return iter;
	}
	return found_match;
}

/*
 * When adding a dive, we'll add all the pre-existing cylinder
 * information from that dive to our cylinder model.
 */
void add_cylinder_description(cylinder_type_t *type)
{
	GtkTreeIter iter;
	const char *desc;
	unsigned int size, workp;

	desc = type->description;
	if (!desc)
		return;
	size = type->size.mliter;
	workp = type->workingpressure.mbar;
	add_cylinder_type(desc, size, workp, &iter);
}

static void add_cylinder(struct cylinder_widget *cylinder, const char *desc, int ml, int mbar)
{
	GtkTreeIter iter, *match;

	cylinder->name = desc;
	match = add_cylinder_type(desc, ml, mbar, &iter);
	if (match)
		gtk_combo_box_set_active_iter(cylinder->description, match);
}

static void show_cylinder(cylinder_t *cyl, struct cylinder_widget *cylinder)
{
	const char *desc;
	int ml, mbar;
	double o2;

	/* Don't show uninitialized cylinder widgets */
	if (!cylinder->description)
		return;

	desc = cyl->type.description;
	if (!desc)
		desc = "";
	ml = cyl->type.size.mliter;
	mbar = cyl->type.workingpressure.mbar;
	add_cylinder(cylinder, desc, ml, mbar);

	set_cylinder_type_spinbuttons(cylinder,
		cyl->type.size.mliter, cyl->type.workingpressure.mbar);
	set_cylinder_pressure_spinbuttons(cylinder,
		cyl->start.mbar, cyl->end.mbar);
	o2 = cyl->gasmix.o2.permille / 10.0;
	gtk_widget_set_sensitive(cylinder->o2, !!o2);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cylinder->gasmix_button), !!o2);
	if (!o2)
		o2 = 21.0;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cylinder->o2), o2);
}

static int cyl_nothing(cylinder_t *cyl)
{
	return	!cyl->type.size.mliter &&
		!cyl->type.workingpressure.mbar &&
		!cyl->type.description &&
		!cyl->gasmix.o2.permille &&
		!cyl->gasmix.he.permille &&
		!cyl->start.mbar &&
		!cyl->end.mbar;
}

static void set_one_cylinder(int index, cylinder_t *cyl, GtkListStore *model, GtkTreeIter *iter)
{
	gtk_list_store_set(model, iter,
		CYL_DESC, cyl->type.description ? : "",
		CYL_SIZE, cyl->type.size.mliter,
		CYL_WORKP, cyl->type.workingpressure.mbar,
		CYL_STARTP, cyl->start.mbar,
		CYL_ENDP, cyl->end.mbar,
		CYL_O2, cyl->gasmix.o2.permille,
		CYL_HE, cyl->gasmix.he.permille,
		-1);
}

void show_dive_equipment(struct dive *dive)
{
	int i, max;
	GtkTreeIter iter;
	GtkListStore *model;

	model = cylinder_list.model;
	gtk_list_store_clear(model);
	max = MAX_CYLINDERS;
	do {
		cylinder_t *cyl = &dive->cylinder[max-1];

		if (!cyl_nothing(cyl))
			break;
	} while (--max);

	cylinder_list.max_index = max;

	gtk_widget_set_sensitive(cylinder_list.edit, 0);
	gtk_widget_set_sensitive(cylinder_list.del, 0);
	gtk_widget_set_sensitive(cylinder_list.add, max < MAX_CYLINDERS);

	for (i = 0; i < max; i++) {
		cylinder_t *cyl = dive->cylinder+i;

		gtk_list_store_append(model, &iter);
		set_one_cylinder(i, cyl, model, &iter);
	}
}

static GtkWidget *create_spinbutton(GtkWidget *vbox, const char *name, double min, double max, double incr)
{
	GtkWidget *frame, *hbox, *button;

	frame = gtk_frame_new(name);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	button = gtk_spin_button_new_with_range(min, max, incr);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);

	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(button), GTK_UPDATE_IF_VALID);

	return button;
}

static void fill_cylinder_info(struct cylinder_widget *cylinder, cylinder_t *cyl, const char *desc,
		double volume, double pressure, double start, double end, int o2)
{
	int mbar, ml;

	if (output_units.pressure == PSI) {
		pressure = psi_to_bar(pressure);
		start = psi_to_bar(start);
		end = psi_to_bar(end);
	}

	if (pressure && output_units.volume == CUFT) {
		volume = cuft_to_l(volume);
		volume /= bar_to_atm(pressure);
	}

	ml = volume * 1000 + 0.5;
	mbar = pressure * 1000 + 0.5;

	if (o2 < 211)
		o2 = 0;
	cyl->type.description = desc;
	cyl->type.size.mliter = ml;
	cyl->type.workingpressure.mbar = mbar;
	cyl->start.mbar = start * 1000 + 0.5;
	cyl->end.mbar = end * 1000 + 0.5;
	cyl->gasmix.o2.permille = o2;

	/*
	 * Also, insert it into the model if it doesn't already exist
	 */
	add_cylinder(cylinder, desc, ml, mbar);
}

static void record_cylinder_changes(cylinder_t *cyl, struct cylinder_widget *cylinder)
{
	const gchar *desc;
	GtkComboBox *box;
	double volume, pressure, start, end;
	int o2;

	/* Ignore uninitialized cylinder widgets */
	box = cylinder->description;
	if (!box)
		return;

	desc = gtk_combo_box_get_active_text(box);
	volume = gtk_spin_button_get_value(cylinder->size);
	pressure = gtk_spin_button_get_value(cylinder->pressure);
	start = gtk_spin_button_get_value(cylinder->start);
	end = gtk_spin_button_get_value(cylinder->end);
	o2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cylinder->o2))*10 + 0.5;
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cylinder->gasmix_button)))
		o2 = 0;
	fill_cylinder_info(cylinder, cyl, desc, volume, pressure, start, end, o2);
}

void flush_dive_equipment_changes(struct dive *dive)
{
	/* We do nothing: we require the "Ok" button press */
}

/*
 * We hardcode the most common standard cylinders,
 * we should pick up any other names from the dive
 * logs directly.
 */
static struct tank_info {
	const char *name;
	int size;	/* cuft if < 1000, otherwise mliter */
	int psi;	/* If zero, size is in mliter */
} tank_info[100] = {
	/* Need an empty entry for the no-cylinder case */
	{ "", 0, 0 },

	/* Size-only metric cylinders */
	{ "10.0 l", 10000 },
	{ "11.1 l", 11100 },

	/* Most common AL cylinders */
	{ "AL50",   50, 3000 },
	{ "AL63",   63, 3000 },
	{ "AL72",   72, 3000 },
	{ "AL80",   80, 3000 },
	{ "AL100", 100, 3300 },

	/* Somewhat common LP steel cylinders */
	{ "LP85",   85, 2640 },
	{ "LP95",   95, 2640 },
	{ "LP108", 108, 2640 },
	{ "LP121", 121, 2640 },

	/* Somewhat common HP steel cylinders */
	{ "HP65",   65, 3442 },
	{ "HP80",   80, 3442 },
	{ "HP100", 100, 3442 },
	{ "HP119", 119, 3442 },
	{ "HP130", 130, 3442 },

	/* We'll fill in more from the dive log dynamically */
	{ NULL, }
};

static void fill_tank_list(GtkListStore *store)
{
	GtkTreeIter iter;
	struct tank_info *info = tank_info;

	while (info->name) {
		int size = info->size;
		int psi = info->psi;
		int mbar = 0, ml = size;

		/* Is it in cuft and psi? */
		if (psi) {
			double bar = psi_to_bar(psi);
			double airvolume = cuft_to_l(size) * 1000.0;
			double atm = bar_to_atm(bar);

			ml = airvolume / atm + 0.5;
			mbar = bar*1000 + 0.5;
		}

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, info->name,
			1, ml,
			2, mbar,
			-1);
		info++;
	}
}

static void nitrox_cb(GtkToggleButton *button, gpointer data)
{
	struct cylinder_widget *cylinder = data;
	int state;

	state = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(cylinder->o2, state);
}

static gboolean completion_cb(GtkEntryCompletion *widget, GtkTreeModel *model, GtkTreeIter *iter, struct cylinder_widget *cylinder)
{
	const char *desc;
	unsigned int ml, mbar;

	gtk_tree_model_get(model, iter, CYL_DESC, &desc, CYL_SIZE, &ml, CYL_WORKP, &mbar, -1);
	add_cylinder(cylinder, desc, ml, mbar);
	return TRUE;
}

static void cylinder_activate_cb(GtkComboBox *combo_box, gpointer data)
{
	struct cylinder_widget *cylinder = data;
	cylinder_cb(cylinder->description, data);
}

static void cylinder_widget(GtkWidget *vbox, struct cylinder_widget *cylinder, GtkListStore *model)
{
	GtkWidget *frame, *hbox;
	GtkEntry *entry;
	GtkEntryCompletion *completion;
	GtkWidget *widget;

	/*
	 * Cylinder type: description, size and
	 * working pressure
	 */
	frame = gtk_frame_new("Cylinder");

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	widget = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(model), 0);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	cylinder->description = GTK_COMBO_BOX(widget);
	g_signal_connect(widget, "changed", G_CALLBACK(cylinder_cb), cylinder);

	entry = GTK_ENTRY(GTK_BIN(widget)->child);
	g_signal_connect(entry, "activate", G_CALLBACK(cylinder_activate_cb), cylinder);

	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(model));
	g_signal_connect(completion, "match-selected", G_CALLBACK(completion_cb), cylinder);
	gtk_entry_set_completion(entry, completion);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	widget = create_spinbutton(hbox, "Size", 0, 300, 0.1);
	cylinder->size = GTK_SPIN_BUTTON(widget);

	widget = create_spinbutton(hbox, "Pressure", 0, 5000, 1);
	cylinder->pressure = GTK_SPIN_BUTTON(widget);

	/*
	 * Cylinder start/end pressures
	 */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	widget = create_spinbutton(hbox, "Start Pressure", 0, 5000, 1);
	cylinder->start = GTK_SPIN_BUTTON(widget);

	widget = create_spinbutton(hbox, "End Pressure", 0, 5000, 1);
	cylinder->end = GTK_SPIN_BUTTON(widget);

	/*
	 * Cylinder gas mix: Air, Nitrox or Trimix
	 */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	widget = create_spinbutton(hbox, "Nitrox", 21, 100, 0.1);
	cylinder->o2 = widget;
	cylinder->gasmix_button = gtk_check_button_new();
	gtk_box_pack_start(GTK_BOX(gtk_widget_get_parent(cylinder->o2)),
		cylinder->gasmix_button, FALSE, FALSE, 3);
	g_signal_connect(cylinder->gasmix_button, "toggled", G_CALLBACK(nitrox_cb), cylinder);

	gtk_spin_button_set_range(GTK_SPIN_BUTTON(cylinder->o2), 21.0, 100.0);
}

static int edit_cylinder_dialog(int index, cylinder_t *cyl)
{
	int success;
	GtkWidget *dialog, *vbox;
	struct cylinder_widget cylinder;
	struct dive *dive;

	cylinder.index = index;
	cylinder.changed = 0;

	dive = current_dive;
	if (!dive)
		return 0;
	*cyl = dive->cylinder[index];

	dialog = gtk_dialog_new_with_buttons("Cylinder",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	cylinder_widget(vbox, &cylinder, cylinder_model);

	show_cylinder(cyl, &cylinder);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		record_cylinder_changes(cyl, &cylinder);
		dive->cylinder[index] = *cyl;
		mark_divelist_changed(TRUE);
		flush_divelist(dive);
	}

	gtk_widget_destroy(dialog);

	return success;
}

static int get_model_index(GtkListStore *model, GtkTreeIter *iter)
{
	int *p, index;
	GtkTreePath *path;

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), iter);
	p = gtk_tree_path_get_indices(path);
	index = p ? *p : 0;
	gtk_tree_path_free(path);
	return index;
}

static void edit_cb(GtkButton *button, gpointer data)
{
	int index;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list.model;
	GtkTreeSelection *selection;
	cylinder_t cyl;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(cylinder_list.tree_view));

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);
	if (!edit_cylinder_dialog(index, &cyl))
		return;

	set_one_cylinder(index, &cyl, model, &iter);
	repaint_dive();
}

static void add_cb(GtkButton *button, gpointer data)
{
	int index = cylinder_list.max_index;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list.model;
	GtkTreeSelection *selection;
	cylinder_t cyl;

	if (!edit_cylinder_dialog(index, &cyl))
		return;

	gtk_list_store_append(model, &iter);
	set_one_cylinder(index, &cyl, model, &iter);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(cylinder_list.tree_view));
	gtk_tree_selection_select_iter(selection, &iter);

	cylinder_list.max_index++;
	gtk_widget_set_sensitive(cylinder_list.add, cylinder_list.max_index < MAX_CYLINDERS);
}

static void del_cb(GtkButton *button, gpointer data)
{
	int index, nr;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list.model;
	GtkTreeSelection *selection;
	struct dive *dive;
	cylinder_t *cyl;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(cylinder_list.tree_view));

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);

	dive = current_dive;
	if (!dive)
		return;
	cyl = dive->cylinder + index;
	nr = cylinder_list.max_index - index - 1;

	gtk_list_store_remove(model, &iter);

	cylinder_list.max_index--;
	memmove(cyl, cyl+1, nr*sizeof(*cyl));
	memset(cyl+nr, 0, sizeof(*cyl));

	mark_divelist_changed(TRUE);
	flush_divelist(dive);

	gtk_widget_set_sensitive(cylinder_list.edit, 0);
	gtk_widget_set_sensitive(cylinder_list.del, 0);
	gtk_widget_set_sensitive(cylinder_list.add, 1);
}

static GtkListStore *create_tank_size_model(void)
{
	GtkListStore *model;

	model = gtk_list_store_new(3,
		G_TYPE_STRING,		/* Tank name */
		G_TYPE_INT,		/* Tank size in mliter */
		G_TYPE_INT,		/* Tank working pressure in mbar */
		-1);

	fill_tank_list(model);
	return model;
}

static void size_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int ml, mbar;
	double size, pressure;
	char buffer[10];

	gtk_tree_model_get(model, iter, CYL_SIZE, &ml, CYL_WORKP, &mbar, -1);
	convert_volume_pressure(ml, mbar, &size, &pressure);
	if (size)
		snprintf(buffer, sizeof(buffer), "%.1f", size);
	else
		strcpy(buffer, "unkn");
	g_object_set(renderer, "text", buffer, NULL);
}

static void pressure_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int index = (long)data;
	int mbar, decimals;
	double pressure;
	char buffer[10];

	gtk_tree_model_get(model, iter, index, &mbar, -1);
	decimals = convert_pressure(mbar, &pressure);
	if (mbar)
		snprintf(buffer, sizeof(buffer), "%.*f", decimals, pressure);
	else
		*buffer = 0;
	g_object_set(renderer, "text", buffer, NULL);
}

static void percentage_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int index = (long)data;
	int permille;
	char buffer[10];

	gtk_tree_model_get(model, iter, index, &permille, -1);
	if (permille)
		snprintf(buffer, sizeof(buffer), "%.1f%%", permille / 10.0);
	else
		*buffer = 0;
	g_object_set(renderer, "text", buffer, NULL);
}

static void selection_cb(GtkTreeSelection *selection, GtkTreeModel *model)
{
	GtkTreeIter iter;
	int selected;

	selected = gtk_tree_selection_get_selected(selection, NULL, &iter);
	gtk_widget_set_sensitive(cylinder_list.edit, selected);
	gtk_widget_set_sensitive(cylinder_list.del, selected);
}

static void row_activated_cb(GtkTreeView *tree_view,
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			GtkTreeModel *model)
{
	edit_cb(NULL, NULL);
}

static GtkWidget *cylinder_list_create(void)
{
	GtkWidget *tree_view;
	GtkTreeSelection *selection;
	GtkListStore *model;

	model = gtk_list_store_new(CYL_COLUMNS,
		G_TYPE_STRING,		/* CYL_DESC: utf8 */
		G_TYPE_INT,		/* CYL_SIZE: mliter */
		G_TYPE_INT,		/* CYL_WORKP: mbar */
		G_TYPE_INT,		/* CYL_STARTP: mbar */
		G_TYPE_INT,		/* CYL_ENDP: mbar */
		G_TYPE_INT,		/* CYL_O2: permille */
		G_TYPE_INT		/* CYL_HE: permille */
		);
	cylinder_list.model = model;
	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	g_signal_connect(tree_view, "row-activated", G_CALLBACK(row_activated_cb), model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_BROWSE);
	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), model);

	g_object_set(G_OBJECT(tree_view), "headers-visible", TRUE,
					  "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_BOTH,
					  NULL);

	cylinder_list.desc = tree_view_column(tree_view, CYL_DESC, "Type", NULL, PANGO_ALIGN_LEFT, TRUE);
	cylinder_list.size = tree_view_column(tree_view, CYL_SIZE, "Size", size_data_func, PANGO_ALIGN_RIGHT, TRUE);
	cylinder_list.workp = tree_view_column(tree_view, CYL_WORKP, "MaxPress", pressure_data_func, PANGO_ALIGN_RIGHT, TRUE);
	cylinder_list.startp = tree_view_column(tree_view, CYL_STARTP, "Start", pressure_data_func, PANGO_ALIGN_RIGHT, TRUE);
	cylinder_list.endp = tree_view_column(tree_view, CYL_ENDP, "End", pressure_data_func, PANGO_ALIGN_RIGHT, TRUE);
	cylinder_list.o2 = tree_view_column(tree_view, CYL_O2, "O" UTF8_SUBSCRIPT_2 "%", percentage_data_func, PANGO_ALIGN_RIGHT, TRUE);
	cylinder_list.he = tree_view_column(tree_view, CYL_HE, "He%", percentage_data_func, PANGO_ALIGN_RIGHT, TRUE);
	return tree_view;
}

GtkWidget *equipment_widget(void)
{
	GtkWidget *vbox, *hbox, *frame, *framebox;
	GtkWidget *add, *del, *edit;

	vbox = gtk_vbox_new(FALSE, 3);

	/*
	 * We create the cylinder size model at startup, since
	 * we're going to share it across all cylinders and all
	 * dives. So if you add a new cylinder type in one dive,
	 * it will show up when you edit the cylinder types for
	 * another dive.
	 */
	cylinder_model = create_tank_size_model();

	cylinder_list.tree_view = cylinder_list_create();

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	frame = gtk_frame_new("Cylinders");
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 3);

	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), framebox);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	gtk_box_pack_start(GTK_BOX(hbox), cylinder_list.tree_view, TRUE, FALSE, 3);

	hbox = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	edit = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	add = gtk_button_new_from_stock(GTK_STOCK_ADD);
	del = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_box_pack_start(GTK_BOX(hbox), edit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), del, FALSE, FALSE, 0);

	cylinder_list.edit = edit;
	cylinder_list.add = add;
	cylinder_list.del = del;

	g_signal_connect(edit, "clicked", G_CALLBACK(edit_cb), NULL);
	g_signal_connect(add, "clicked", G_CALLBACK(add_cb), NULL);
	g_signal_connect(del, "clicked", G_CALLBACK(del_cb), NULL);

	return vbox;
}
