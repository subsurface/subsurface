#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"

void show_dive_equipment(struct dive *dive)
{
}

void flush_dive_equipment_changes(struct dive *dive)
{
}

static struct tank_info {
	const char *name;
	int size;	/* cuft or mliter depending on psi */
	int psi;	/* If zero, size is in mliter */
} tank_info[] = {
	{ "None", },
	{ "10.0 l", 10000 },
	{ "11.1 l", 11100 },
	{ "AL72", 72, 3000 },
	{ "AL80", 80, 3000 },
	{ "LP85", 85, 2640 },
	{ "LP95", 95, 2640 },
	{ "HP100", 100, 3442 },
	{ "HP119", 119, 3442 },
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
			double bar = 0.0689475729 * psi;
			double airvolume = 28316.8466 * size;
			double atm = bar / 1.01325;

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

static void cylinder_widget(GtkWidget *box, int nr, GtkListStore *model)
{
	GtkWidget *frame, *hbox, *size;
	char buffer[80];

	snprintf(buffer, sizeof(buffer), "Cylinder %d", nr);
	frame = gtk_frame_new(buffer);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(TRUE, 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	size = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(model), 0);
	gtk_box_pack_start(GTK_BOX(hbox), size, FALSE, FALSE, 0);
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

GtkWidget *equipment_widget(void)
{
	GtkWidget *vbox;
	GtkListStore *model;

	vbox = gtk_vbox_new(TRUE, 3);

	model = create_tank_size_model();
	cylinder_widget(vbox, 0, model);

	return vbox;
}
