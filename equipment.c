/* equipment.c */
/* creates the UI for the equipment page -
 * controlled through the following interfaces:
 *
 * void show_dive_equipment(struct dive *dive, int w_idx)
 *
 * called from gtk-ui:
 * GtkWidget *equipment_widget(int w_idx)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <glib/gi18n.h>

#include "dive.h"
#include "display.h"
#if USE_GTK_UI
#include "display-gtk.h"
#endif
#include "divelist.h"
#include "conversions.h"

#if USE_GTK_UI
#include "display-gtk.h"
static GtkListStore *cylinder_model, *weightsystem_model;

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

enum {
	WS_DESC,
	WS_WEIGHT,
	WS_COLUMNS
};

struct equipment_list {
	int max_index;
	GtkListStore *model;
	GtkTreeView *tree_view;
	GtkWidget *edit, *add, *del;
};

static struct equipment_list cylinder_list[2], weightsystem_list[2];

struct dive edit_dive;

struct cylinder_widget {
	int index, changed;
	const char *name;
	GtkWidget *hbox;
	GtkComboBox *description;
	GtkSpinButton *size, *pressure;
	GtkWidget *start, *end, *pressure_button;
	GtkWidget *o2, *he, *gasmix_button;
	int w_idx;
};

struct ws_widget {
	int index, changed;
	const char *name;
	GtkWidget *hbox;
	GtkComboBox *description;
	GtkSpinButton *weight;
	int w_idx;
};
#endif /* USE_GTK_UI */

/* we want bar - so let's not use our unit functions */
int convert_pressure(int mbar, double *p)
{
	int decimals = 1;
	double pressure;

	if (prefs.units.pressure == PSI) {
		pressure = mbar_to_PSI(mbar);
		decimals = 0;
	} else {
		pressure = mbar / 1000.0;
	}

	*p = pressure;
	return decimals;
}

void convert_volume_pressure(int ml, int mbar, double *v, double *p)
{
	double volume, pressure;

	volume = ml / 1000.0;
	if (mbar) {
		if (prefs.units.volume == CUFT) {
			volume = ml_to_cuft(ml);
			volume *= bar_to_atm(mbar / 1000.0);
		}

		if (prefs.units.pressure == PSI)
			pressure = mbar_to_PSI(mbar);
		else
			pressure = mbar / 1000.0;
		*p = pressure;
	} else {
		*p = 0;
	}
	*v = volume;
}

int convert_weight(int grams, double *m)
{
	int decimals = 1; /* not sure - do people do less than whole lbs/kg ? */
	double weight;

	if (prefs.units.weight == LBS)
		weight = grams_to_lbs(grams);
	else
		weight = grams / 1000.0;
	*m = weight;
	return decimals;
}

#if USE_GTK_UI
static void set_cylinder_description(struct cylinder_widget *cylinder, const char *desc)
{
	set_active_text(cylinder->description, desc);
}


static void set_cylinder_type_spinbuttons(struct cylinder_widget *cylinder, int ml, int mbar)
{
	double volume, pressure;

	convert_volume_pressure(ml, mbar, &volume, &pressure);
	gtk_spin_button_set_value(cylinder->size, volume);
	gtk_spin_button_set_value(cylinder->pressure, pressure);
}

static void set_cylinder_pressure_spinbuttons(struct cylinder_widget *cylinder, cylinder_t *cyl)
{
	int set;
	unsigned int start, end;
	double pressure;

	start = cyl->start.mbar;
	end = cyl->end.mbar;
	set = start || end;
	if (!set) {
		start = cyl->sample_start.mbar;
		end = cyl->sample_end.mbar;
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cylinder->pressure_button), set);
	gtk_widget_set_sensitive(cylinder->start, set);
	gtk_widget_set_sensitive(cylinder->end, set);

	convert_pressure(start, &pressure);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cylinder->start), pressure);
	convert_pressure(end, &pressure);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cylinder->end), pressure);
}

static void set_weight_description(struct ws_widget *ws_widget, const char *desc)
{
	set_active_text(ws_widget->description, desc);
}

static void set_weight_weight_spinbutton(struct ws_widget *ws_widget, int grams)
{
	double weight;

	convert_weight(grams, &weight);
	gtk_spin_button_set_value(ws_widget->weight, weight);
}

/*
 * The gtk_tree_model_foreach() interface is bad. It could have
 * returned whether the callback ever returned true
 */
static GtkTreeIter *found_match = NULL;
static GtkTreeIter match_iter;

static gboolean match_desc(GtkTreeModel *model,	GtkTreePath *path,
			GtkTreeIter *iter, gpointer data)
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

static int get_active_item(GtkComboBox *combo_box, GtkTreeIter *iter, GtkListStore *model)
{
	const char *desc;

	if (gtk_combo_box_get_active_iter(combo_box, iter))
		return TRUE;

	desc = get_active_text(combo_box);

	found_match = NULL;
	gtk_tree_model_foreach(GTK_TREE_MODEL(model), match_desc, (void *)desc);

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
	struct dive *dive;
	cylinder_t *cyl;

	if (cylinder->w_idx == W_IDX_PRIMARY)
		dive = current_dive;
	else
		dive = &edit_dive;
	cyl = dive->cylinder + cylinder->index;

	/* Did the user set it to some non-standard value? */
	if (!get_active_item(combo_box, &iter, cylinder_model)) {
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
		const char *desc = get_active_text(combo_box);

		same = !strcmp(desc, cyl->type.description);
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

static void weight_cb(GtkComboBox *combo_box, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo_box);
	int weight;
	struct ws_widget *ws_widget = data;
	struct dive *dive;
	weightsystem_t *ws;
	if (ws_widget->w_idx == W_IDX_PRIMARY)
		dive = current_dive;
	else
		dive = &edit_dive;
	ws = dive->weightsystem + ws_widget->index;

	/* Did the user set it to some non-standard value? */
	if (!get_active_item(combo_box, &iter, weightsystem_model)) {
		ws_widget->changed = 1;
		return;
	}

	/*
	 * We get "change" signal callbacks just because we set
	 * the description by hand. Whatever. So ignore them if
	 * they are no-ops.
	 */
	if (!ws_widget->changed && ws->description) {
		int same;
		const char *desc = get_active_text(combo_box);

		same = !strcmp(desc, ws->description);
		if (same)
			return;
	}
	ws_widget->changed = 1;

	gtk_tree_model_get(model, &iter,
		WS_WEIGHT, &weight,
		-1);

	set_weight_weight_spinbutton(ws_widget, weight);
}

static GtkTreeIter *add_cylinder_type(const char *desc, int ml, int mbar, GtkTreeIter *iter)
{
	GtkTreeModel *model;

	/* Don't even bother adding stuff without a size */
	if (!ml)
		return NULL;

	found_match = NULL;
	model = GTK_TREE_MODEL(cylinder_model);
	gtk_tree_model_foreach(model, match_desc, (void *)desc);

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

static GtkTreeIter *add_weightsystem_type(const char *desc, int weight, GtkTreeIter *iter)
{
	GtkTreeModel *model;

	found_match = NULL;
	model = GTK_TREE_MODEL(weightsystem_model);
	gtk_tree_model_foreach(model, match_desc, (void *)desc);

	if (found_match) {
		gtk_list_store_set(GTK_LIST_STORE(model), found_match,
				WS_WEIGHT, weight,
				-1);
	} else if (desc && desc[0]) {
		gtk_list_store_append(GTK_LIST_STORE(model), iter);
		gtk_list_store_set(GTK_LIST_STORE(model), iter,
			WS_DESC, desc,
			WS_WEIGHT, weight,
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
	GtkTreeIter iter;

	cylinder->name = desc;
	add_cylinder_type(desc, ml, mbar, &iter);
}

void add_weightsystem_description(weightsystem_t *weightsystem)
{
	GtkTreeIter iter;
	const char *desc;
	unsigned int weight;

	desc = weightsystem->description;
	if (!desc)
		return;
	weight = weightsystem->weight.grams;
	add_weightsystem_type(desc, weight, &iter);
}

static void add_weightsystem(struct ws_widget *ws_widget, const char *desc, int weight)
{
	GtkTreeIter iter;

	ws_widget->name = desc;
	add_weightsystem_type(desc, weight, &iter);
}

static void show_cylinder(cylinder_t *cyl, struct cylinder_widget *cylinder)
{
	const char *desc;
	int ml, mbar;
	int gasmix;
	double o2, he;

	/* Don't show uninitialized cylinder widgets */
	if (!cylinder->description)
		return;

	desc = cyl->type.description;
	if (!desc)
		desc = "";
	ml = cyl->type.size.mliter;
	mbar = cyl->type.workingpressure.mbar;
	add_cylinder(cylinder, desc, ml, mbar);

	set_cylinder_description(cylinder, desc);
	set_cylinder_type_spinbuttons(cylinder,
		cyl->type.size.mliter, cyl->type.workingpressure.mbar);
	set_cylinder_pressure_spinbuttons(cylinder, cyl);

	gasmix = cyl->gasmix.o2.permille || cyl->gasmix.he.permille;
	gtk_widget_set_sensitive(cylinder->o2, gasmix);
	gtk_widget_set_sensitive(cylinder->he, gasmix);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cylinder->gasmix_button), gasmix);

	o2 = cyl->gasmix.o2.permille / 10.0;
	he = cyl->gasmix.he.permille / 10.0;
	if (!o2)
		o2 = O2_IN_AIR / 10.0;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cylinder->o2), o2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(cylinder->he), he);
}

static void show_weightsystem(weightsystem_t *ws, struct ws_widget *weightsystem_widget)
{
	const char *desc;
	int grams;

	/* Don't show uninitialized widgets */
	if (!weightsystem_widget->description)
		return;

	desc = ws->description;
	if (!desc)
		desc = "";
	grams = ws->weight.grams;
	add_weightsystem(weightsystem_widget, desc, grams);

	set_weight_description(weightsystem_widget, desc);
	set_weight_weight_spinbutton(weightsystem_widget, ws->weight.grams);
}
#else
/* placeholders for a few functions that we need to redesign for the Qt UI */
void add_cylinder_description(cylinder_type_t *type)
{
	const char *desc;

	desc = type->description;
	if (!desc)
		return;
	/* now do something with it... */
}
void add_weightsystem_description(weightsystem_t *weightsystem)
{
	const char *desc;

	desc = weightsystem->description;
	if (!desc)
		return;
	/* now do something with it... */
}

#endif /* USE_GTK_UI */

gboolean cylinder_nodata(cylinder_t *cyl)
{
	return	!cyl->type.size.mliter &&
		!cyl->type.workingpressure.mbar &&
		!cyl->type.description &&
		!cyl->gasmix.o2.permille &&
		!cyl->gasmix.he.permille &&
		!cyl->start.mbar &&
		!cyl->end.mbar;
}

static gboolean cylinder_nosamples(cylinder_t *cyl)
{
	return	!cyl->sample_start.mbar &&
		!cyl->sample_end.mbar;
}

gboolean cylinder_none(void *_data)
{
	cylinder_t *cyl = _data;
	return cylinder_nodata(cyl) && cylinder_nosamples(cyl);
}

/* descriptions are equal if they are both NULL or both non-NULL
   and the same text */
static gboolean description_equal(const char *desc1, const char *desc2)
{
		return ((! desc1 && ! desc2) ||
			(desc1 && desc2 && strcmp(desc1, desc2) == 0));
}

static gboolean weightsystem_none(void *_data)
{
	weightsystem_t *ws = _data;
	return !ws->weight.grams && !ws->description;
}

gboolean no_weightsystems(weightsystem_t *ws)
{
	int i;

	for (i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		if (!weightsystem_none(ws + i))
			return FALSE;
	return TRUE;
}

static gboolean one_weightsystem_equal(weightsystem_t *ws1, weightsystem_t *ws2)
{
	return ws1->weight.grams == ws2->weight.grams &&
		description_equal(ws1->description, ws2->description);
}

gboolean weightsystems_equal(weightsystem_t *ws1, weightsystem_t *ws2)
{
	int i;

	for (i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		if (!one_weightsystem_equal(ws1 + i, ws2 + i))
			return FALSE;
	return TRUE;
}

#if USE_GTK_UI
static void set_one_cylinder(void *_data, GtkListStore *model, GtkTreeIter *iter)
{
	cylinder_t *cyl = _data;
	unsigned int start, end;

	start = cyl->start.mbar ? : cyl->sample_start.mbar;
	end = cyl->end.mbar ? : cyl->sample_end.mbar;
	gtk_list_store_set(model, iter,
		CYL_DESC, cyl->type.description ? : "",
		CYL_SIZE, cyl->type.size.mliter,
		CYL_WORKP, cyl->type.workingpressure.mbar,
		CYL_STARTP, start,
		CYL_ENDP, end,
		CYL_O2, cyl->gasmix.o2.permille,
		CYL_HE, cyl->gasmix.he.permille,
		-1);
}

static void set_one_weightsystem(void *_data, GtkListStore *model, GtkTreeIter *iter)
{
	weightsystem_t *ws = _data;

	gtk_list_store_set(model, iter,
		WS_DESC, ws->description ? : _("unspecified"),
		WS_WEIGHT, ws->weight.grams,
		-1);
}

static void *cyl_ptr(struct dive *dive, int idx)
{
	if (idx < 0 || idx >= MAX_CYLINDERS)
		return NULL;
	return &dive->cylinder[idx];
}

static void *ws_ptr(struct dive *dive, int idx)
{
	if (idx < 0 || idx >= MAX_WEIGHTSYSTEMS)
		return NULL;
	return &dive->weightsystem[idx];
}

static void show_equipment(struct dive *dive, int max,
			struct equipment_list *equipment_list,
			void*(*ptr_function)(struct dive*, int),
			gboolean(*none_function)(void *),
			void(*set_one_function)(void*, GtkListStore*, GtkTreeIter *))
{
	int i, used;
	void *data;
	GtkTreeIter iter;
	GtkListStore *model = equipment_list->model;

	if (! model)
		return;
	if (! dive) {
		gtk_widget_set_sensitive(equipment_list->edit, 0);
		gtk_widget_set_sensitive(equipment_list->del, 0);
		gtk_widget_set_sensitive(equipment_list->add, 0);
		clear_equipment_widgets();
		return;
	}
	gtk_list_store_clear(model);
	used = max;
	do {
		data = ptr_function(dive, used-1);
		if (!none_function(data))
			break;
	} while (--used);

	equipment_list->max_index = used;

	gtk_widget_set_sensitive(equipment_list->edit, 0);
	gtk_widget_set_sensitive(equipment_list->del, 0);
	gtk_widget_set_sensitive(equipment_list->add, used < max);

	for (i = 0; i < used; i++) {
		data = ptr_function(dive, i);
		gtk_list_store_append(model, &iter);
		set_one_function(data, model, &iter);
	}
}

void show_dive_equipment(struct dive *dive, int w_idx)
{
	show_equipment(dive, MAX_CYLINDERS, &cylinder_list[w_idx],
		&cyl_ptr, &cylinder_none, &set_one_cylinder);
	show_equipment(dive, MAX_WEIGHTSYSTEMS, &weightsystem_list[w_idx],
		&ws_ptr, &weightsystem_none, &set_one_weightsystem);
}

int select_cylinder(struct dive *dive, int when)
{
	GtkWidget *dialog, *vbox, *label;
	GtkWidget *buttons[MAX_CYLINDERS] = { NULL, };
	GSList *group = NULL;
	int i, success, nr, selected = -1;
	char buffer[256];

	nr = MAX_CYLINDERS - 1;
	while (nr >= 0 && cylinder_nodata(cyl_ptr(dive, nr)))
		nr--;

	if (nr == -1) {
		dialog = gtk_dialog_new_with_buttons(_("Cannot add gas change"),
				GTK_WINDOW(main_window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				NULL);
		vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		label = gtk_label_new(_("No cylinders listed for this dive."));
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
		gtk_widget_show_all(dialog);
		success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
		goto bail;
	}
	snprintf(buffer, sizeof(buffer), _("Add gaschange event at %d:%02u"), FRACTION(when, 60));
	dialog = gtk_dialog_new_with_buttons(buffer,
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	label = gtk_label_new(_("Available gases"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	for (i = 0; i <= nr; i++) {
		char gas_buf[80];
		cylinder_t *cyl = cyl_ptr(dive, i);
		get_gas_string(cyl->gasmix.o2.permille, cyl->gasmix.he.permille, gas_buf, sizeof(gas_buf));
		snprintf(buffer, sizeof(buffer), "%s: %s",
			dive->cylinder[i].type.description ?: _("unknown"), gas_buf);
		buttons[i] = gtk_radio_button_new_with_label(group, buffer);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(buttons[i]));
		gtk_box_pack_start(GTK_BOX(vbox), buttons[i], FALSE, FALSE, 0);
	}
	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		for (i = 0; i <= nr; i++)
			if (buttons[i] && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttons[i])))
				selected = i;
	}
	for (i = 0; i <= nr; i++)
		if (buttons[i])
			gtk_widget_destroy(buttons[i]);
bail:
	gtk_widget_destroy(dialog);

	return selected;

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
		double volume, double pressure, double start, double end, int o2, int he)
{
	int mbar, ml;

	if (prefs.units.pressure == PSI) {
		pressure = psi_to_bar(pressure);
		start = psi_to_bar(start);
		end = psi_to_bar(end);
	}

	mbar = pressure * 1000 + 0.5;
	if (mbar && prefs.units.volume == CUFT) {
		volume = cuft_to_l(volume);
		volume /= bar_to_atm(pressure);
	}

	ml = volume * 1000 + 0.5;

	/* Ignore obviously crazy He values */
	if (o2 + he > 1000)
		he = 0;

	/* We have a rule that normal air is all zeroes */
	if (!he && o2 > 208 && o2 < 211)
		o2 = 0;

	cyl->type.description = desc;
	cyl->type.size.mliter = ml;
	cyl->type.workingpressure.mbar = mbar;
	cyl->start.mbar = start * 1000 + 0.5;
	cyl->end.mbar = end * 1000 + 0.5;
	cyl->gasmix.o2.permille = o2;
	cyl->gasmix.he.permille = he;

	/*
	 * Also, insert it into the model if it doesn't already exist
	 */
	// WARNING: GTK-Specific Code.
	add_cylinder(cylinder, desc, ml, mbar);
}

static void record_cylinder_changes(cylinder_t *cyl, struct cylinder_widget *cylinder)
{
	const gchar *desc;
	GtkComboBox *box;
	double volume, pressure, start, end;
	int o2, he;

	/* Ignore uninitialized cylinder widgets */
	box = cylinder->description;
	if (!box)
		return;

	desc = strdup(get_active_text(box));
	volume = gtk_spin_button_get_value(cylinder->size);
	pressure = gtk_spin_button_get_value(cylinder->pressure);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cylinder->pressure_button))) {
		start = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cylinder->start));
		end = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cylinder->end));
	} else {
		start = end = 0;
	}
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cylinder->gasmix_button))) {
		o2 = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cylinder->o2))*10 + 0.5;
		he = gtk_spin_button_get_value(GTK_SPIN_BUTTON(cylinder->he))*10 + 0.5;
	} else {
		o2 = 0;
		he = 0;
	}
	fill_cylinder_info(cylinder, cyl, desc, volume, pressure, start, end, o2, he);
}

static void record_weightsystem_changes(weightsystem_t *ws, struct ws_widget *weightsystem_widget)
{
	const gchar *desc;
	GtkComboBox *box;
	int grams;
	double value;
	GtkTreeIter iter;

	/* Ignore uninitialized cylinder widgets */
	box = weightsystem_widget->description;
	if (!box)
		return;

	desc = strdup(get_active_text(box));
	value = gtk_spin_button_get_value(weightsystem_widget->weight);

	if (prefs.units.weight == LBS)
		grams = lbs_to_grams(value);
	else
		grams = value * 1000;
	ws->weight.grams = grams;
	ws->description = desc;
	add_weightsystem_type(desc, grams, &iter);
}
#endif /* USE_GTK_UI */
/*
 * We hardcode the most common standard cylinders,
 * we should pick up any other names from the dive
 * logs directly.
 */
struct tank_info tank_info[100] = {
	/* Need an empty entry for the no-cylinder case */
	{ "", },

	/* Size-only metric cylinders */
	{ "10.0 l", .ml = 10000 },
	{ "11.1 l", .ml = 11100 },

	/* Most common AL cylinders */
	{ "AL50",  .cuft =  50, .psi = 3000 },
	{ "AL63",  .cuft =  63, .psi = 3000 },
	{ "AL72",  .cuft =  72, .psi = 3000 },
	{ "AL80",  .cuft =  80, .psi = 3000 },
	{ "AL100", .cuft = 100, .psi = 3300 },

	/* Somewhat common LP steel cylinders */
	{ "LP85",  .cuft =  85, .psi = 2640 },
	{ "LP95",  .cuft =  95, .psi = 2640 },
	{ "LP108", .cuft = 108, .psi = 2640 },
	{ "LP121", .cuft = 121, .psi = 2640 },

	/* Somewhat common HP steel cylinders */
	{ "HP65",  .cuft =  65, .psi = 3442 },
	{ "HP80",  .cuft =  80, .psi = 3442 },
	{ "HP100", .cuft = 100, .psi = 3442 },
	{ "HP119", .cuft = 119, .psi = 3442 },
	{ "HP130", .cuft = 130, .psi = 3442 },

	/* Common European steel cylinders */
	{ "10L 300 bar",  .ml = 10000, .bar = 300 },
	{ "12L 200 bar",  .ml = 12000, .bar = 200 },
	{ "12L 232 bar",  .ml = 12000, .bar = 232 },
	{ "12L 300 bar",  .ml = 12000, .bar = 300 },
	{ "15L 200 bar",  .ml = 15000, .bar = 200 },
	{ "15L 232 bar",  .ml = 15000, .bar = 232 },
	{ "D7 300 bar",   .ml = 14000, .bar = 300 },
	{ "D8.5 232 bar", .ml = 17000, .bar = 232 },
	{ "D12 232 bar",  .ml = 24000, .bar = 232 },

	/* We'll fill in more from the dive log dynamically */
	{ NULL, }
};

#if USE_GTK_UI
static void fill_tank_list(GtkListStore *store)
{
	GtkTreeIter iter;
	struct tank_info *info = tank_info;

	for (info = tank_info ; info->name; info++) {
		int ml = info->ml;
		int cuft = info->cuft;
		int psi = info->psi;
		int mbar;
		double bar = info->bar;

		if (psi && bar)
			goto bad_tank_info;
		if (ml && cuft)
			goto bad_tank_info;
		if (cuft && !psi)
			goto bad_tank_info;

		/* Is it in cuft and psi? */
		if (psi)
			bar = psi_to_bar(psi);

		if (cuft) {
			double airvolume = cuft_to_l(cuft) * 1000.0;
			double atm = bar_to_atm(bar);

			ml = airvolume / atm + 0.5;
		}

		mbar = bar * 1000 + 0.5;

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, info->name,
			1, ml,
			2, mbar,
			-1);
		continue;

bad_tank_info:
		fprintf(stderr, "Bad tank info for '%s'\n", info->name);
	}
}

/*
 * We hardcode the most common weight system types
 * This is a bit odd as the weight system types don't usually encode weight
 */
static struct ws_info {
	const char *name;
	int grams;
} ws_info[100] = {
	{ N_("integrated"), 0 },
	{ N_("belt"), 0 },
	{ N_("ankle"), 0 },
	{ N_("backplate weight"), 0 },
	{ N_("clip-on"), 0 },
};

static void fill_ws_list(GtkListStore *store)
{
	GtkTreeIter iter;
	struct ws_info *info = ws_info;

	while (info->name) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, _(info->name),
			1, info->grams,
			-1);
		info++;
	}
}

static void gasmix_cb(GtkToggleButton *button, gpointer data)
{
	struct cylinder_widget *cylinder = data;
	int state;

	state = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(cylinder->o2, state);
	gtk_widget_set_sensitive(cylinder->he, state);
}

static void pressure_cb(GtkToggleButton *button, gpointer data)
{
	struct cylinder_widget *cylinder = data;
	int state;

	state = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(cylinder->start, state);
	gtk_widget_set_sensitive(cylinder->end, state);
}

static void cylinder_activate_cb(GtkComboBox *combo_box, gpointer data)
{
	struct cylinder_widget *cylinder = data;
	cylinder_cb(cylinder->description, data);
}

/* Return a frame containing a hbox inside a hbox */
static GtkWidget *frame_box(const char *title, GtkWidget *vbox)
{
	GtkWidget *hbox, *frame;

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);

	frame = gtk_frame_new(title);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	return hbox;
}

static GtkWidget *labeled_spinbutton(GtkWidget *box, const char *name, double min, double max, double incr)
{
	GtkWidget *hbox, *label, *button;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, TRUE, FALSE, 0);

	label = gtk_label_new(name);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);

	button = gtk_spin_button_new_with_range(min, max, incr);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);

	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(button), GTK_UPDATE_IF_VALID);

	return button;
}

static void cylinder_widget(GtkWidget *vbox, struct cylinder_widget *cylinder, GtkListStore *model)
{
	GtkWidget *frame, *hbox;
	GtkWidget *widget;

	/*
	 * Cylinder type: description, size and
	 * working pressure
	 */
	frame = gtk_frame_new(_("Cylinder"));

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	widget = combo_box_with_model_and_entry(model);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	cylinder->description = GTK_COMBO_BOX(widget);
	g_signal_connect(widget, "changed", G_CALLBACK(cylinder_cb), cylinder);

	g_signal_connect(get_entry(GTK_COMBO_BOX(widget)), "activate", G_CALLBACK(cylinder_activate_cb), cylinder);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	widget = create_spinbutton(hbox, _("Size"), 0, 300, 0.1);
	cylinder->size = GTK_SPIN_BUTTON(widget);

	widget = create_spinbutton(hbox, _("Pressure"), 0, 5000, 1);
	cylinder->pressure = GTK_SPIN_BUTTON(widget);

	/*
	 * Cylinder start/end pressures
	 */
	hbox = frame_box(_("Pressure"), vbox);

	widget = labeled_spinbutton(hbox, _("Start"), 0, 5000, 1);
	cylinder->start = widget;

	widget = labeled_spinbutton(hbox, _("End"), 0, 5000, 1);
	cylinder->end = widget;

	cylinder->pressure_button = gtk_check_button_new();
	gtk_box_pack_start(GTK_BOX(hbox), cylinder->pressure_button, FALSE, FALSE, 3);
	g_signal_connect(cylinder->pressure_button, "toggled", G_CALLBACK(pressure_cb), cylinder);

	/*
	 * Cylinder gas mix: Air, Nitrox or Trimix
	 */
	hbox = frame_box(_("Gasmix"), vbox);

	widget = labeled_spinbutton(hbox, "O"UTF8_SUBSCRIPT_2 "%", 1, 100, 0.1);
	cylinder->o2 = widget;
	widget = labeled_spinbutton(hbox, "He%", 0, 100, 0.1);
	cylinder->he = widget;
	cylinder->gasmix_button = gtk_check_button_new();
	gtk_box_pack_start(GTK_BOX(hbox), cylinder->gasmix_button, FALSE, FALSE, 3);
	g_signal_connect(cylinder->gasmix_button, "toggled", G_CALLBACK(gasmix_cb), cylinder);
}

static void weight_activate_cb(GtkComboBox *combo_box, gpointer data)
{
	struct ws_widget *ws_widget = data;
	weight_cb(ws_widget->description, data);
}

static void ws_widget(GtkWidget *vbox, struct ws_widget *ws_widget, GtkListStore *model)
{
	GtkWidget *frame, *hbox;
	GtkWidget *widget;
	GtkEntry *entry;

	/*
	 * weight_system: description and weight
	 */
	frame = gtk_frame_new(_("Weight"));

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	widget = combo_box_with_model_and_entry(model);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	ws_widget->description = GTK_COMBO_BOX(widget);
	g_signal_connect(widget, "changed", G_CALLBACK(weight_cb), ws_widget);

	entry = get_entry(GTK_COMBO_BOX(widget));
	g_signal_connect(entry, "activate", G_CALLBACK(weight_activate_cb), ws_widget);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	if (prefs.units.weight == KG)
		widget = create_spinbutton(hbox, _("kg"), 0, 50, 0.5);
	else
		widget = create_spinbutton(hbox, _("lbs"), 0, 110, 1);
	ws_widget->weight = GTK_SPIN_BUTTON(widget);
}

static int edit_cylinder_dialog(GtkWidget *w, int index, cylinder_t *cyl, int w_idx)
{
	int success;
	GtkWidget *dialog, *vbox, *parent;
	struct cylinder_widget cylinder;
	struct dive *dive;

	cylinder.index = index;
	cylinder.w_idx = w_idx;
	cylinder.changed = 0;

	if (w_idx == W_IDX_PRIMARY)
		dive = current_dive;
	else
		dive = &edit_dive;
	if (!dive)
		return 0;
	*cyl = dive->cylinder[index];

	dialog = gtk_dialog_new_with_buttons(_("Cylinder"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	parent = gtk_widget_get_ancestor(w, GTK_TYPE_DIALOG);
	if (parent) {
		gtk_widget_set_sensitive(parent, FALSE);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
	}

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	cylinder_widget(vbox, &cylinder, cylinder_model);

	show_cylinder(cyl, &cylinder);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		record_cylinder_changes(cyl, &cylinder);
		dive->cylinder[index] = *cyl;
		if (w_idx == W_IDX_PRIMARY) {
			mark_divelist_changed(TRUE);
			update_cylinder_related_info(dive);
			flush_divelist(dive);
		}
	}

	gtk_widget_destroy(dialog);
	if (parent)
		gtk_widget_set_sensitive(parent, TRUE);
	return success;
}

static int edit_weightsystem_dialog(GtkWidget *w, int index, weightsystem_t *ws, int w_idx)
{
	int success;
	GtkWidget *dialog, *vbox, *parent;
	struct ws_widget weightsystem_widget;
	struct dive *dive;

	weightsystem_widget.index = index;
	weightsystem_widget.w_idx = w_idx;
	weightsystem_widget.changed = 0;

	if (w_idx == W_IDX_PRIMARY)
		dive = current_dive;
	else
		dive = &edit_dive;
	if (!dive)
		return 0;
	*ws = dive->weightsystem[index];

	dialog = gtk_dialog_new_with_buttons(_("Weight System"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	parent = gtk_widget_get_ancestor(w, GTK_TYPE_DIALOG);
	if (parent) {
		gtk_widget_set_sensitive(parent, FALSE);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
	}

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	ws_widget(vbox, &weightsystem_widget, weightsystem_model);

	show_weightsystem(ws, &weightsystem_widget);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		record_weightsystem_changes(ws, &weightsystem_widget);
		dive->weightsystem[index] = *ws;
		if (w_idx == W_IDX_PRIMARY) {
			mark_divelist_changed(TRUE);
			flush_divelist(dive);
		}
	}

	gtk_widget_destroy(dialog);
	if (parent)
		gtk_widget_set_sensitive(parent, TRUE);
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

static void edit_cb(GtkWidget *w, int w_idx)
{
	int index;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list[w_idx].model;
	GtkTreeView *tree_view = cylinder_list[w_idx].tree_view;
	GtkTreeSelection *selection;
	cylinder_t cyl;

	selection = gtk_tree_view_get_selection(tree_view);

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);
	if (!edit_cylinder_dialog(w, index, &cyl, w_idx))
		return;

	set_one_cylinder(&cyl, model, &iter);
	repaint_dive();
}

static void add_cb(GtkWidget *w, int w_idx)
{
	int index = cylinder_list[w_idx].max_index;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list[w_idx].model;
	GtkTreeView *tree_view = cylinder_list[w_idx].tree_view;
	GtkTreeSelection *selection;
	cylinder_t cyl;

	if (!edit_cylinder_dialog(w, index, &cyl, w_idx))
		return;

	gtk_list_store_append(model, &iter);
	set_one_cylinder(&cyl, model, &iter);

	selection = gtk_tree_view_get_selection(tree_view);
	gtk_tree_selection_select_iter(selection, &iter);

	cylinder_list[w_idx].max_index++;
	gtk_widget_set_sensitive(cylinder_list[w_idx].add, cylinder_list[w_idx].max_index < MAX_CYLINDERS);
}

static void del_cb(GtkButton *button, int w_idx)
{
	int index, nr;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list[w_idx].model;
	GtkTreeView *tree_view = cylinder_list[w_idx].tree_view;
	GtkTreeSelection *selection;
	struct dive *dive;
	cylinder_t *cyl;

	selection = gtk_tree_view_get_selection(tree_view);

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);

	if (w_idx == W_IDX_PRIMARY)
		dive = current_dive;
	else
		dive = &edit_dive;
	if (!dive)
		return;
	cyl = dive->cylinder + index;
	nr = cylinder_list[w_idx].max_index - index - 1;

	gtk_list_store_remove(model, &iter);

	cylinder_list[w_idx].max_index--;
	memmove(cyl, cyl+1, nr*sizeof(*cyl));
	memset(cyl+nr, 0, sizeof(*cyl));

	mark_divelist_changed(TRUE);
	flush_divelist(dive);

	gtk_widget_set_sensitive(cylinder_list[w_idx].edit, 0);
	gtk_widget_set_sensitive(cylinder_list[w_idx].del, 0);
	gtk_widget_set_sensitive(cylinder_list[w_idx].add, 1);
}

static void ws_edit_cb(GtkWidget *w, int w_idx)
{
	int index;
	GtkTreeIter iter;
	GtkListStore *model = weightsystem_list[w_idx].model;
	GtkTreeView *tree_view = weightsystem_list[w_idx].tree_view;
	GtkTreeSelection *selection;
	weightsystem_t ws;

	selection = gtk_tree_view_get_selection(tree_view);

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);
	if (!edit_weightsystem_dialog(w, index, &ws, w_idx))
		return;

	set_one_weightsystem(&ws, model, &iter);
	repaint_dive();
}

static void ws_add_cb(GtkWidget *w, int w_idx)
{
	int index = weightsystem_list[w_idx].max_index;
	GtkTreeIter iter;
	GtkListStore *model = weightsystem_list[w_idx].model;
	GtkTreeView *tree_view = weightsystem_list[w_idx].tree_view;
	GtkTreeSelection *selection;
	weightsystem_t ws;

	if (!edit_weightsystem_dialog(w, index, &ws, w_idx))
		return;

	gtk_list_store_append(model, &iter);
	set_one_weightsystem(&ws, model, &iter);

	selection = gtk_tree_view_get_selection(tree_view);
	gtk_tree_selection_select_iter(selection, &iter);

	weightsystem_list[w_idx].max_index++;
	gtk_widget_set_sensitive(weightsystem_list[w_idx].add, weightsystem_list[w_idx].max_index < MAX_WEIGHTSYSTEMS);
}

static void ws_del_cb(GtkButton *button, int w_idx)
{
	int index, nr;
	GtkTreeIter iter;
	GtkListStore *model = weightsystem_list[w_idx].model;
	GtkTreeView *tree_view = weightsystem_list[w_idx].tree_view;
	GtkTreeSelection *selection;
	struct dive *dive;
	weightsystem_t *ws;

	selection = gtk_tree_view_get_selection(tree_view);

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);

	if (w_idx == W_IDX_PRIMARY)
		dive = current_dive;
	else
		dive = &edit_dive;
	if (!dive)
		return;
	ws = dive->weightsystem + index;
	nr = weightsystem_list[w_idx].max_index - index - 1;

	gtk_list_store_remove(model, &iter);

	weightsystem_list[w_idx].max_index--;
	memmove(ws, ws+1, nr*sizeof(*ws));
	memset(ws+nr, 0, sizeof(*ws));

	mark_divelist_changed(TRUE);
	flush_divelist(dive);

	gtk_widget_set_sensitive(weightsystem_list[w_idx].edit, 0);
	gtk_widget_set_sensitive(weightsystem_list[w_idx].del, 0);
	gtk_widget_set_sensitive(weightsystem_list[w_idx].add, 1);
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

static GtkListStore *create_weightsystem_model(void)
{
	GtkListStore *model;

	model = gtk_list_store_new(2,
		G_TYPE_STRING,		/* Weightsystem description */
		G_TYPE_INT,		/* Weight in grams */
		-1);

	fill_ws_list(model);
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
	char buffer[64];

	gtk_tree_model_get(model, iter, CYL_SIZE, &ml, CYL_WORKP, &mbar, -1);
	convert_volume_pressure(ml, mbar, &size, &pressure);
	if (size)
		snprintf(buffer, sizeof(buffer), "%.1f", size);
	else
		strcpy(buffer, _("unkn"));
	g_object_set(renderer, "text", buffer, NULL);
}

static void weight_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int idx = (long)data;
	int grams, decimals;
	double value;
	char buffer[64];

	gtk_tree_model_get(model, iter, idx, &grams, -1);
	decimals = convert_weight(grams, &value);
	if (grams)
		snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
	else
		strcpy(buffer, _("unkn"));
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

static void selection_cb(GtkTreeSelection *selection, struct equipment_list *list)
{
	GtkTreeIter iter;
	int selected;

	selected = gtk_tree_selection_get_selected(selection, NULL, &iter);
	gtk_widget_set_sensitive(list->edit, selected);
	gtk_widget_set_sensitive(list->del, selected);
}

static void row_activated_cb(GtkTreeView *tree_view,
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			int w_idx)
{
	edit_cb(GTK_WIDGET(tree_view), w_idx);
}

static void ws_row_activated_cb(GtkTreeView *tree_view,
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			int w_idx)
{
	ws_edit_cb(GTK_WIDGET(tree_view), w_idx);
}

static GtkWidget *cylinder_list_widget(int w_idx)
{
	GtkListStore *model = cylinder_list[w_idx].model;
	GtkWidget *tree_view;
	GtkTreeSelection *selection;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_widget_set_can_focus(tree_view, FALSE);

	g_signal_connect(tree_view, "row-activated", G_CALLBACK(row_activated_cb), GINT_TO_POINTER(w_idx));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_BROWSE);
	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), &cylinder_list[w_idx]);

	g_object_set(G_OBJECT(tree_view), "headers-visible", TRUE,
					  "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_BOTH,
					  NULL);

	tree_view_column(tree_view, CYL_DESC, _("Type"), NULL, ALIGN_LEFT | UNSORTABLE);
	tree_view_column(tree_view, CYL_SIZE, _("Size"), size_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_WORKP, _("MaxPress"), pressure_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_STARTP, _("Start"), pressure_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_ENDP, _("End"), pressure_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_O2, "O" UTF8_SUBSCRIPT_2 "%", percentage_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_HE, "He%", percentage_data_func, ALIGN_RIGHT | UNSORTABLE);
	return tree_view;
}

static GtkWidget *weightsystem_list_widget(int w_idx)
{
	GtkListStore *model = weightsystem_list[w_idx].model;
	GtkWidget *tree_view;
	GtkTreeSelection *selection;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_widget_set_can_focus(tree_view, FALSE);
	g_signal_connect(tree_view, "row-activated", G_CALLBACK(ws_row_activated_cb), GINT_TO_POINTER(w_idx));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_BROWSE);
	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), &weightsystem_list[w_idx]);

	g_object_set(G_OBJECT(tree_view), "headers-visible", TRUE,
					  "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_BOTH,
					  NULL);

	tree_view_column(tree_view, WS_DESC, _("Type"), NULL, ALIGN_LEFT | UNSORTABLE);
	tree_view_column(tree_view, WS_WEIGHT, _("weight"),
			weight_data_func, ALIGN_RIGHT | UNSORTABLE);

	return tree_view;
}

static GtkWidget *cylinder_list_create(int w_idx)
{
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
	cylinder_list[w_idx].model = model;
	return cylinder_list_widget(w_idx);
}

static GtkWidget *weightsystem_list_create(int w_idx)
{
	GtkListStore *model;

	model = gtk_list_store_new(WS_COLUMNS,
		G_TYPE_STRING,		/* WS_DESC: utf8 */
		G_TYPE_INT		/* WS_WEIGHT: grams */
		);
	weightsystem_list[w_idx].model = model;
	return weightsystem_list_widget(w_idx);
}

GtkWidget *equipment_widget(int w_idx)
{
	GtkWidget *vbox, *hbox, *frame, *framebox, *tree_view;
	GtkWidget *add, *del, *edit;

	vbox = gtk_vbox_new(FALSE, 3);

	/*
	 * We create the cylinder size (and weightsystem) models
	 * at startup for the primary cylinder / weightsystem widget,
	 * since we're going to share it across all cylinders and all
	 * dives. So if you add a new cylinder type or weightsystem in
	 * one dive, it will show up when you edit the cylinder types
	 * or weightsystems for another dive.
	 */
	if (w_idx == W_IDX_PRIMARY)
		cylinder_model = create_tank_size_model();
	tree_view = cylinder_list_create(w_idx);
	cylinder_list[w_idx].tree_view = GTK_TREE_VIEW(tree_view);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	frame = gtk_frame_new(_("Cylinders"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 3);

	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), framebox);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	gtk_box_pack_start(GTK_BOX(hbox), tree_view, TRUE, FALSE, 3);

	hbox = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	edit = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	add = gtk_button_new_from_stock(GTK_STOCK_ADD);
	del = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(edit, FALSE);
	gtk_widget_set_sensitive(add, FALSE);
	gtk_widget_set_sensitive(del, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), edit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), del, FALSE, FALSE, 0);

	cylinder_list[w_idx].edit = edit;
	cylinder_list[w_idx].add = add;
	cylinder_list[w_idx].del = del;

	g_signal_connect(edit, "clicked", G_CALLBACK(edit_cb), GINT_TO_POINTER(w_idx));
	g_signal_connect(add, "clicked", G_CALLBACK(add_cb), GINT_TO_POINTER(w_idx));
	g_signal_connect(del, "clicked", G_CALLBACK(del_cb), GINT_TO_POINTER(w_idx));

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	if (w_idx == W_IDX_PRIMARY)
		weightsystem_model = create_weightsystem_model();
	tree_view = weightsystem_list_create(w_idx);
	weightsystem_list[w_idx].tree_view = GTK_TREE_VIEW(tree_view);

	frame = gtk_frame_new(_("Weight"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 3);

	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), framebox);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	gtk_box_pack_start(GTK_BOX(hbox), tree_view, TRUE, FALSE, 3);

	hbox = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	edit = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	add = gtk_button_new_from_stock(GTK_STOCK_ADD);
	del = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_widget_set_sensitive(edit, FALSE);
	gtk_widget_set_sensitive(add, FALSE);
	gtk_widget_set_sensitive(del, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), edit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), del, FALSE, FALSE, 0);

	weightsystem_list[w_idx].edit = edit;
	weightsystem_list[w_idx].add = add;
	weightsystem_list[w_idx].del = del;

	g_signal_connect(edit, "clicked", G_CALLBACK(ws_edit_cb), GINT_TO_POINTER(w_idx));
	g_signal_connect(add, "clicked", G_CALLBACK(ws_add_cb), GINT_TO_POINTER(w_idx));
	g_signal_connect(del, "clicked", G_CALLBACK(ws_del_cb), GINT_TO_POINTER(w_idx));

	return vbox;
}

void clear_equipment_widgets()
{
	gtk_list_store_clear(cylinder_list[W_IDX_PRIMARY].model);
	gtk_list_store_clear(weightsystem_list[W_IDX_PRIMARY].model);
}
#endif /* USE_GTK_UI */
