/* equipment.c */
/* creates the UI for the equipment page -
 * controlled through the following interfaces:
 *
 * void show_dive_equipment(struct dive *dive)
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
	GtkWidget *edit, *add, *del;
};

static struct equipment_list cylinder_list, weightsystem_list;


struct cylinder_widget {
	int index, changed;
	const char *name;
	GtkWidget *hbox;
	GtkComboBox *description;
	GtkSpinButton *size, *pressure;
	GtkWidget *start, *end, *pressure_button;
	GtkWidget *o2, *he, *gasmix_button;
};

struct ws_widget {
	int index, changed;
	const char *name;
	GtkWidget *hbox;
	GtkComboBox *description;
	GtkSpinButton *weight;
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

static void convert_volume_pressure(int ml, int mbar, double *v, double *p)
{
	double volume, pressure;

	volume = ml / 1000.0;
	if (mbar) {
		if (output_units.volume == CUFT) {
			volume = ml_to_cuft(ml);
			volume *= bar_to_atm(mbar / 1000.0);
		}

		if (output_units.pressure == PSI) {
			pressure = mbar_to_PSI(mbar);
		} else
			pressure = mbar / 1000.0;
	}
	*v = volume;
	*p = pressure;
}

static int convert_weight(int grams, double *m)
{
	int decimals = 1; /* not sure - do people do less than whole lbs/kg ? */
	double weight;

	if (output_units.weight == LBS)
		weight = grams_to_lbs(grams);
	else
		weight = grams / 1000.0;
	*m = weight;
	return decimals;
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
	char *desc;

	if (gtk_combo_box_get_active_iter(combo_box, iter))
		return TRUE;

	desc = gtk_combo_box_get_active_text(combo_box);

	found_match = NULL;
	gtk_tree_model_foreach(GTK_TREE_MODEL(model), match_desc, (void *)desc);

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

static void weight_cb(GtkComboBox *combo_box, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(combo_box);
	int weight;
	struct ws_widget *ws_widget = data;
	weightsystem_t *ws = current_dive->weightsystem + ws_widget->index;

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
		char *desc = gtk_combo_box_get_active_text(combo_box);

		same = !strcmp(desc, ws->description);
		g_free(desc);
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
	GtkTreeIter iter, *match;

	cylinder->name = desc;
	match = add_cylinder_type(desc, ml, mbar, &iter);
	if (match)
		gtk_combo_box_set_active_iter(cylinder->description, match);
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
	GtkTreeIter iter, *match;

	ws_widget->name = desc;
	match = add_weightsystem_type(desc, weight, &iter);
	if (match)
		gtk_combo_box_set_active_iter(ws_widget->description, match);
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
		o2 = AIR_PERMILLE / 10.0;
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

	set_weight_weight_spinbutton(weightsystem_widget, ws->weight.grams);
}

int cylinder_none(void *_data)
{
	cylinder_t *cyl = _data;
	return	!cyl->type.size.mliter &&
		!cyl->type.workingpressure.mbar &&
		!cyl->type.description &&
		!cyl->gasmix.o2.permille &&
		!cyl->gasmix.he.permille &&
		!cyl->sample_start.mbar &&
		!cyl->sample_end.mbar &&
		!cyl->start.mbar &&
		!cyl->end.mbar;
}

int weightsystem_none(void *_data)
{
	weightsystem_t *ws = _data;
	return !ws->weight.grams && !ws->description;
}

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
		WS_DESC, ws->description ? : "unspecified",
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
			int(*none_function)(void *),
			void(*set_one_function)(void*, GtkListStore*, GtkTreeIter *))
{
	int i, used;
	void *data;
	GtkTreeIter iter;
	GtkListStore *model = equipment_list->model;

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

void show_dive_equipment(struct dive *dive)
{
	show_equipment(dive, MAX_CYLINDERS, &cylinder_list,
		&cyl_ptr, &cylinder_none, &set_one_cylinder);
	show_equipment(dive, MAX_WEIGHTSYSTEMS, &weightsystem_list,
		&ws_ptr, &weightsystem_none, &set_one_weightsystem);
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

	desc = gtk_combo_box_get_active_text(box);
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

	desc = gtk_combo_box_get_active_text(box);
	value = gtk_spin_button_get_value(weightsystem_widget->weight);

	if (output_units.weight == LBS)
		grams = lbs_to_grams(value);
	else
		grams = value * 1000;
	ws->weight.grams = grams;
	ws->description = desc;
	add_weightsystem_type(desc, grams, &iter);
}

/*
 * We hardcode the most common standard cylinders,
 * we should pick up any other names from the dive
 * logs directly.
 */
static struct tank_info {
	const char *name;
	int cuft, ml, psi, bar;
} tank_info[100] = {
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
	{ "integrated", 0 },
	{ "belt", 0 },
	{ "ankle", 0 },
	{ "bar", 0 },
	{ "clip-on", 0 },
};

static void fill_ws_list(GtkListStore *store)
{
	GtkTreeIter iter;
	struct ws_info *info = ws_info;

	while (info->name) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, info->name,
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
	hbox = frame_box("Pressure", vbox);

	widget = labeled_spinbutton(hbox, "Start", 0, 5000, 1);
	cylinder->start = widget;

	widget = labeled_spinbutton(hbox, "End", 0, 5000, 1);
	cylinder->end = widget;

	cylinder->pressure_button = gtk_check_button_new();
	gtk_box_pack_start(GTK_BOX(hbox), cylinder->pressure_button, FALSE, FALSE, 3);
	g_signal_connect(cylinder->pressure_button, "toggled", G_CALLBACK(pressure_cb), cylinder);

	/*
	 * Cylinder gas mix: Air, Nitrox or Trimix
	 */
	hbox = frame_box("Gasmix", vbox);

	widget = labeled_spinbutton(hbox, "O"UTF8_SUBSCRIPT_2 "%", 1, 100, 0.1);
	cylinder->o2 = widget;
	widget = labeled_spinbutton(hbox, "He%", 0, 100, 0.1);
	cylinder->he = widget;
	cylinder->gasmix_button = gtk_check_button_new();
	gtk_box_pack_start(GTK_BOX(hbox), cylinder->gasmix_button, FALSE, FALSE, 3);
	g_signal_connect(cylinder->gasmix_button, "toggled", G_CALLBACK(gasmix_cb), cylinder);
}

static gboolean weight_completion_cb(GtkEntryCompletion *widget, GtkTreeModel *model, GtkTreeIter *iter, struct ws_widget *ws_widget)
{
	const char *desc;
	unsigned int weight;

	gtk_tree_model_get(model, iter, WS_DESC, &desc, WS_WEIGHT, &weight, -1);
	add_weightsystem(ws_widget, desc, weight);
	return TRUE;
}

static void weight_activate_cb(GtkComboBox *combo_box, gpointer data)
{
	struct ws_widget *ws_widget = data;
	weight_cb(ws_widget->description, data);
}

static void ws_widget(GtkWidget *vbox, struct ws_widget *ws_widget, GtkListStore *model)
{
	GtkWidget *frame, *hbox;
	GtkEntryCompletion *completion;
	GtkWidget *widget;
	GtkEntry *entry;

	/*
	 * weight_system: description and weight
	 */
	frame = gtk_frame_new("Weight");

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	widget = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(model), 0);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	ws_widget->description = GTK_COMBO_BOX(widget);
	g_signal_connect(widget, "changed", G_CALLBACK(weight_cb), ws_widget);

	entry = GTK_ENTRY(GTK_BIN(widget)->child);
	g_signal_connect(entry, "activate", G_CALLBACK(weight_activate_cb), ws_widget);

	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(model));
	g_signal_connect(completion, "match-selected", G_CALLBACK(weight_completion_cb), ws_widget);
	gtk_entry_set_completion(entry, completion);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);

	if ( output_units.weight == KG)
		widget = create_spinbutton(hbox, "kg", 0, 50, 0.5);
	else
		widget = create_spinbutton(hbox, "lbs", 0, 110, 1);
	ws_widget->weight = GTK_SPIN_BUTTON(widget);
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
		update_cylinder_related_info(dive);
		flush_divelist(dive);
	}

	gtk_widget_destroy(dialog);

	return success;
}

static int edit_weightsystem_dialog(int index, weightsystem_t *ws)
{
	int success;
	GtkWidget *dialog, *vbox;
	struct ws_widget weightsystem_widget;
	struct dive *dive;

	weightsystem_widget.index = index;
	weightsystem_widget.changed = 0;

	dive = current_dive;
	if (!dive)
		return 0;
	*ws = dive->weightsystem[index];

	dialog = gtk_dialog_new_with_buttons("Weight System",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	ws_widget(vbox, &weightsystem_widget, weightsystem_model);

	show_weightsystem(ws, &weightsystem_widget);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		record_weightsystem_changes(ws, &weightsystem_widget);
		dive->weightsystem[index] = *ws;
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

static void edit_cb(GtkButton *button, GtkTreeView *tree_view)
{
	int index;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list.model;
	GtkTreeSelection *selection;
	cylinder_t cyl;

	selection = gtk_tree_view_get_selection(tree_view);

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);
	if (!edit_cylinder_dialog(index, &cyl))
		return;

	set_one_cylinder(&cyl, model, &iter);
	repaint_dive();
}

static void add_cb(GtkButton *button, GtkTreeView *tree_view)
{
	int index = cylinder_list.max_index;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list.model;
	GtkTreeSelection *selection;
	cylinder_t cyl;

	if (!edit_cylinder_dialog(index, &cyl))
		return;

	gtk_list_store_append(model, &iter);
	set_one_cylinder(&cyl, model, &iter);

	selection = gtk_tree_view_get_selection(tree_view);
	gtk_tree_selection_select_iter(selection, &iter);

	cylinder_list.max_index++;
	gtk_widget_set_sensitive(cylinder_list.add, cylinder_list.max_index < MAX_CYLINDERS);
}

static void del_cb(GtkButton *button, GtkTreeView *tree_view)
{
	int index, nr;
	GtkTreeIter iter;
	GtkListStore *model = cylinder_list.model;
	GtkTreeSelection *selection;
	struct dive *dive;
	cylinder_t *cyl;

	selection = gtk_tree_view_get_selection(tree_view);

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

static void ws_edit_cb(GtkButton *button, GtkTreeView *tree_view)
{
	int index;
	GtkTreeIter iter;
	GtkListStore *model = weightsystem_list.model;
	GtkTreeSelection *selection;
	weightsystem_t ws;

	selection = gtk_tree_view_get_selection(tree_view);

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);
	if (!edit_weightsystem_dialog(index, &ws))
		return;

	set_one_weightsystem(&ws, model, &iter);
	repaint_dive();
}

static void ws_add_cb(GtkButton *button, GtkTreeView *tree_view)
{
	int index = weightsystem_list.max_index;
	GtkTreeIter iter;
	GtkListStore *model = weightsystem_list.model;
	GtkTreeSelection *selection;
	weightsystem_t ws;

	if (!edit_weightsystem_dialog(index, &ws))
		return;

	gtk_list_store_append(model, &iter);
	set_one_weightsystem(&ws, model, &iter);

	selection = gtk_tree_view_get_selection(tree_view);
	gtk_tree_selection_select_iter(selection, &iter);

	weightsystem_list.max_index++;
	gtk_widget_set_sensitive(weightsystem_list.add, weightsystem_list.max_index < MAX_WEIGHTSYSTEMS);
}

static void ws_del_cb(GtkButton *button, GtkTreeView *tree_view)
{
	int index, nr;
	GtkTreeIter iter;
	GtkListStore *model = weightsystem_list.model;
	GtkTreeSelection *selection;
	struct dive *dive;
	weightsystem_t *ws;

	selection = gtk_tree_view_get_selection(tree_view);

	/* Nothing selected? This shouldn't happen, since the button should be inactive */
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	index = get_model_index(model, &iter);

	dive = current_dive;
	if (!dive)
		return;
	ws = dive->weightsystem + index;
	nr = weightsystem_list.max_index - index - 1;

	gtk_list_store_remove(model, &iter);

	weightsystem_list.max_index--;
	memmove(ws, ws+1, nr*sizeof(*ws));
	memset(ws+nr, 0, sizeof(*ws));

	mark_divelist_changed(TRUE);
	flush_divelist(dive);

	gtk_widget_set_sensitive(weightsystem_list.edit, 0);
	gtk_widget_set_sensitive(weightsystem_list.del, 0);
	gtk_widget_set_sensitive(weightsystem_list.add, 1);
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
	char buffer[10];

	gtk_tree_model_get(model, iter, CYL_SIZE, &ml, CYL_WORKP, &mbar, -1);
	convert_volume_pressure(ml, mbar, &size, &pressure);
	if (size)
		snprintf(buffer, sizeof(buffer), "%.1f", size);
	else
		strcpy(buffer, "unkn");
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
	char buffer[10];

	gtk_tree_model_get(model, iter, idx, &grams, -1);
	decimals = convert_weight(grams, &value);
	if (grams)
		snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
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
			GtkTreeModel *model)
{
	edit_cb(NULL, tree_view);
}

static void ws_row_activated_cb(GtkTreeView *tree_view,
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			GtkTreeModel *model)
{
	ws_edit_cb(NULL, tree_view);
}

GtkWidget *cylinder_list_widget(void)
{
	GtkListStore *model = cylinder_list.model;
	GtkWidget *tree_view;
	GtkTreeSelection *selection;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_widget_set_can_focus(tree_view, FALSE);

	g_signal_connect(tree_view, "row-activated", G_CALLBACK(row_activated_cb), model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_BROWSE);
	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), &cylinder_list);

	g_object_set(G_OBJECT(tree_view), "headers-visible", TRUE,
					  "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_BOTH,
					  NULL);

	tree_view_column(tree_view, CYL_DESC, "Type", NULL, ALIGN_LEFT | UNSORTABLE);
	tree_view_column(tree_view, CYL_SIZE, "Size", size_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_WORKP, "MaxPress", pressure_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_STARTP, "Start", pressure_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_ENDP, "End", pressure_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_O2, "O" UTF8_SUBSCRIPT_2 "%", percentage_data_func, ALIGN_RIGHT | UNSORTABLE);
	tree_view_column(tree_view, CYL_HE, "He%", percentage_data_func, ALIGN_RIGHT | UNSORTABLE);
	return tree_view;
}

GtkWidget *weightsystem_list_widget(void)
{
	GtkListStore *model = weightsystem_list.model;
	GtkWidget *tree_view;
	GtkTreeSelection *selection;

	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_widget_set_can_focus(tree_view, FALSE);
	g_signal_connect(tree_view, "row-activated", G_CALLBACK(ws_row_activated_cb), model);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_BROWSE);
	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), &weightsystem_list);

	g_object_set(G_OBJECT(tree_view), "headers-visible", TRUE,
					  "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_BOTH,
					  NULL);

	tree_view_column(tree_view, WS_DESC, "Type", NULL, ALIGN_LEFT | UNSORTABLE);
	tree_view_column(tree_view, WS_WEIGHT, "weight",
			weight_data_func, ALIGN_RIGHT | UNSORTABLE);

	return tree_view;
}

static GtkWidget *cylinder_list_create(void)
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
	cylinder_list.model = model;
	return cylinder_list_widget();
}

static GtkWidget *weightsystem_list_create(void)
{
	GtkListStore *model;

	model = gtk_list_store_new(WS_COLUMNS,
		G_TYPE_STRING,		/* WS_DESC: utf8 */
		G_TYPE_INT		/* WS_WEIGHT: grams */
		);
	weightsystem_list.model = model;
	return weightsystem_list_widget();
}

GtkWidget *equipment_widget(void)
{
	GtkWidget *vbox, *hbox, *frame, *framebox, *tree_view;
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
	tree_view = cylinder_list_create();

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	frame = gtk_frame_new("Cylinders");
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
	gtk_box_pack_start(GTK_BOX(hbox), edit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), del, FALSE, FALSE, 0);

	cylinder_list.edit = edit;
	cylinder_list.add = add;
	cylinder_list.del = del;

	g_signal_connect(edit, "clicked", G_CALLBACK(edit_cb), tree_view);
	g_signal_connect(add, "clicked", G_CALLBACK(add_cb), tree_view);
	g_signal_connect(del, "clicked", G_CALLBACK(del_cb), tree_view);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	weightsystem_model = create_weightsystem_model();
	tree_view = weightsystem_list_create();

	frame = gtk_frame_new("Weight");
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
	gtk_box_pack_start(GTK_BOX(hbox), edit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), add, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), del, FALSE, FALSE, 0);

	weightsystem_list.edit = edit;
	weightsystem_list.add = add;
	weightsystem_list.del = del;

	g_signal_connect(edit, "clicked", G_CALLBACK(ws_edit_cb), tree_view);
	g_signal_connect(add, "clicked", G_CALLBACK(ws_add_cb), tree_view);
	g_signal_connect(del, "clicked", G_CALLBACK(ws_del_cb), tree_view);

	return vbox;
}
