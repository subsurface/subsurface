#include <stdio.h>
#include <gtk/gtk.h>

#include "display.h"

/* libdivecomputer */
#include <device.h>
#include <suunto.h>
#include <reefnet.h>
#include <uwatec.h>
#include <oceanic.h>
#include <mares.h>
#include <hw.h>
#include <cressi.h>
#include <zeagle.h>
#include <atomics.h>
#include <utils.h>

static device_status_t device_open(const char *devname,
	device_type_t type,
	device_t **device)
{
	switch (type) {
	case DEVICE_TYPE_SUUNTO_SOLUTION:
		return suunto_solution_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_EON:
		return suunto_eon_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_VYPER:
		return suunto_vyper_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_VYPER2:
		return suunto_vyper2_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_D9:
		return suunto_d9_device_open(device, devname);

	case DEVICE_TYPE_UWATEC_ALADIN:
		return uwatec_aladin_device_open(device, devname);

	case DEVICE_TYPE_UWATEC_MEMOMOUSE:
		return uwatec_memomouse_device_open(device, devname);

	case DEVICE_TYPE_UWATEC_SMART:
		return uwatec_smart_device_open(device);

	case DEVICE_TYPE_REEFNET_SENSUS:
		return reefnet_sensus_device_open(device, devname);

	case DEVICE_TYPE_REEFNET_SENSUSPRO:
		return reefnet_sensuspro_device_open(device, devname);

	case DEVICE_TYPE_REEFNET_SENSUSULTRA:
		return reefnet_sensusultra_device_open(device, devname);

	case DEVICE_TYPE_OCEANIC_VTPRO:
		return oceanic_vtpro_device_open(device, devname);

	case DEVICE_TYPE_OCEANIC_VEO250:
		return oceanic_veo250_device_open(device, devname);

	case DEVICE_TYPE_OCEANIC_ATOM2:
		return oceanic_atom2_device_open(device, devname);

	case DEVICE_TYPE_MARES_NEMO:
		return mares_nemo_device_open(device, devname);

	case DEVICE_TYPE_MARES_PUCK:
		return mares_puck_device_open(device, devname);

	case DEVICE_TYPE_MARES_ICONHD:
		return mares_iconhd_device_open(device, devname);

	case DEVICE_TYPE_HW_OSTC:
		return hw_ostc_device_open(device, devname);

	case DEVICE_TYPE_CRESSI_EDY:
		return cressi_edy_device_open(device, devname);

	case DEVICE_TYPE_ZEAGLE_N2ITION3:
		return zeagle_n2ition3_device_open(device, devname);

	case DEVICE_TYPE_ATOMICS_COBALT:
		return atomics_cobalt_device_open(device);

	default:
		return DEVICE_STATUS_ERROR;
	}
}

static void do_import(const char *computer, device_type_t type)
{
	/* FIXME! Needs user input! */
	const char *devname = "/dev/ttyUSB0";
	device_t *device = NULL;
	device_status_t rc;

	rc = device_open(devname, type, &device);
	printf("rc=%d\n", rc);
	if (rc != DEVICE_STATUS_SUCCESS)
		return;
}

/*
 * Taken from 'example.c' in libdivecomputer.
 *
 * I really wish there was some way to just have
 * libdivecomputer tell us what devices it supports,
 * rather than have the application have to know..
 */
struct device_list {
	const char *name;
	device_type_t type;
} device_list[] = {
	{ "Suunto Solution",	DEVICE_TYPE_SUUNTO_SOLUTION },
	{ "Suunto Eon",		DEVICE_TYPE_SUUNTO_EON },
	{ "Suunto Vyper",	DEVICE_TYPE_SUUNTO_VYPER },
	{ "Suunto Vyper Air",	DEVICE_TYPE_SUUNTO_VYPER2 },
	{ "Suunto D9",		DEVICE_TYPE_SUUNTO_D9 },
	{ "Uwatec Aladin",	DEVICE_TYPE_UWATEC_ALADIN },
	{ "Uwatec Memo Mouse",	DEVICE_TYPE_UWATEC_MEMOMOUSE },
	{ "Uwatec Smart",	DEVICE_TYPE_UWATEC_SMART },
	{ "ReefNet Sensus",	DEVICE_TYPE_REEFNET_SENSUS },
	{ "ReefNet Sensus Pro",	DEVICE_TYPE_REEFNET_SENSUSPRO },
	{ "ReefNet Sensus Ultra",DEVICE_TYPE_REEFNET_SENSUSULTRA },
	{ "Oceanic VT Pro",	DEVICE_TYPE_OCEANIC_VTPRO },
	{ "Oceanic Veo250",	DEVICE_TYPE_OCEANIC_VEO250 },
	{ "Oceanic Atom 2",	DEVICE_TYPE_OCEANIC_ATOM2 },
	{ "Mares Nemo",		DEVICE_TYPE_MARES_NEMO },
	{ "Mares Puck",		DEVICE_TYPE_MARES_PUCK },
	{ "Mares Icon HD",	DEVICE_TYPE_MARES_ICONHD },
	{ "OSTC",		DEVICE_TYPE_HW_OSTC },
	{ "Cressi Edy",		DEVICE_TYPE_CRESSI_EDY },
	{ "Zeagle N2iTiON 3",	DEVICE_TYPE_ZEAGLE_N2ITION3 },
	{ "Atomics Cobalt",	DEVICE_TYPE_ATOMICS_COBALT },
	{ NULL }
};

static void fill_computer_list(GtkListStore *store)
{
	GtkTreeIter iter;
	struct device_list *list = device_list;

	for (list = device_list ; list->name ; list++) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, list->name,
			1, list->type,
			-1);
	}
}

static GtkComboBox *dive_computer_selector(GtkWidget *dialog)
{
	GtkWidget *hbox, *combo_box;
	GtkListStore *model;
	GtkCellRenderer *renderer;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

	model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	fill_computer_list(model);

	combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
	gtk_box_pack_start(GTK_BOX(hbox), combo_box, FALSE, TRUE, 3);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer, "text", 0, NULL);

	return GTK_COMBO_BOX(combo_box);
}

void import_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog;
	GtkComboBox *computer;

	dialog = gtk_dialog_new_with_buttons("Import from dive computer",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	computer = dive_computer_selector(dialog);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
		int type;
		GtkTreeIter iter;
		GtkTreeModel *model;
		const char *comp;
	case GTK_RESPONSE_ACCEPT:
		if (!gtk_combo_box_get_active_iter(computer, &iter))
			break;
		model = gtk_combo_box_get_model(computer);
		gtk_tree_model_get(model, &iter,
			0, &comp,
			1, &type,
			-1);
		do_import(comp, type);
		break;
	default:
		break;
	}
	gtk_widget_destroy(dialog);
}

