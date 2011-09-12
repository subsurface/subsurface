#include <stdio.h>
#include <gtk/gtk.h>

#include "dive.h"
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

static void error(const char *fmt, ...)
{
	va_list args;
	GError *error;

	va_start(args, fmt);
	error = g_error_new_valist(
		g_quark_from_string("divelog"),
		DIVE_ERROR_PARSE, fmt, args);
	va_end(args);
	report_error(error);
	g_error_free(error);
}

typedef struct device_data_t {
	device_type_t type;
	const char *name;
	device_devinfo_t devinfo;
	device_clock_t clock;
} device_data_t;

static parser_status_t create_parser(device_data_t *devdata, parser_t **parser)
{
	switch (devdata->type) {
	case DEVICE_TYPE_SUUNTO_SOLUTION:
		return suunto_solution_parser_create(parser);

	case DEVICE_TYPE_SUUNTO_EON:
		return suunto_eon_parser_create(parser, 0);

	case DEVICE_TYPE_SUUNTO_VYPER:
		if (devdata->devinfo.model == 0x01)
			return suunto_eon_parser_create(parser, 1);
		return suunto_vyper_parser_create(parser);

	case DEVICE_TYPE_SUUNTO_VYPER2:
	case DEVICE_TYPE_SUUNTO_D9:
		return suunto_d9_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_UWATEC_ALADIN:
	case DEVICE_TYPE_UWATEC_MEMOMOUSE:
		return uwatec_memomouse_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_UWATEC_SMART:
		return uwatec_smart_parser_create(parser, devdata->devinfo.model, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_REEFNET_SENSUS:
		return reefnet_sensus_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_REEFNET_SENSUSPRO:
		return reefnet_sensuspro_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_REEFNET_SENSUSULTRA:
		return reefnet_sensusultra_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_OCEANIC_VTPRO:
		return oceanic_vtpro_parser_create(parser);

	case DEVICE_TYPE_OCEANIC_VEO250:
		return oceanic_veo250_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_OCEANIC_ATOM2:
		return oceanic_atom2_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_MARES_NEMO:
	case DEVICE_TYPE_MARES_PUCK:
		return mares_nemo_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_MARES_ICONHD:
		return mares_iconhd_parser_create(parser);

	case DEVICE_TYPE_HW_OSTC:
		return hw_ostc_parser_create(parser);

	case DEVICE_TYPE_CRESSI_EDY:
	case DEVICE_TYPE_ZEAGLE_N2ITION3:
		return cressi_edy_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_ATOMICS_COBALT:
		return atomics_cobalt_parser_create(parser);

	default:
		return PARSER_STATUS_ERROR;
	}
}

static int dive_cb(const unsigned char *data, unsigned int size,
	const unsigned char *fingerprint, unsigned int fsize,
	void *userdata)
{
	int rc;
	parser_t *parser = NULL;
	device_data_t *devdata = userdata;
	dc_datetime_t dt = {0};

	rc = create_parser(devdata, &parser);
	if (rc != PARSER_STATUS_SUCCESS) {
		error("Unable to create parser for %s", devdata->name);
		return rc;
	}

	rc = parser_set_data(parser, data, size);
	if (rc != PARSER_STATUS_SUCCESS) {
		error("Error registering the data.");
		parser_destroy(parser);
		return rc;
	}

	rc = parser_get_datetime(parser, &dt);
	if (rc != PARSER_STATUS_SUCCESS && rc != PARSER_STATUS_UNSUPPORTED) {
		error("Error parsing the datetime.");
		parser_destroy (parser);
		return rc;
	}

	printf("<datetime>%04i-%02i-%02i %02i:%02i:%02i</datetime>\n",
		dt.year, dt.month, dt.day,
		dt.hour, dt.minute, dt.second);

	parser_destroy(parser);
	return PARSER_STATUS_SUCCESS;
}


static device_status_t import_device_data(device_t *device, device_data_t *devicedata)
{
	return device_foreach(device, dive_cb, devicedata);
}

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

static void
event_cb (device_t *device, device_event_t event, const void *data, void *userdata)
{
}

static int
cancel_cb (void *userdata)
{
	return 0;
}

static void do_import(const char *computer, device_type_t type)
{
	/* FIXME! Needs user input! */
	const char *devname = "/dev/ttyUSB0";
	device_t *device = NULL;
	device_status_t rc;
	device_data_t devicedata = {
		.type = type,
		.name = computer,
	};

	rc = device_open(devname, type, &device);
	if (rc != DEVICE_STATUS_SUCCESS) {
		error("Unable to open %s (%s)", computer, devname);
		return;
	}

	// Register the event handler.
	int events = DEVICE_EVENT_WAITING | DEVICE_EVENT_PROGRESS | DEVICE_EVENT_DEVINFO | DEVICE_EVENT_CLOCK;
	rc = device_set_events(device, events, event_cb, &devicedata);
	if (rc != DEVICE_STATUS_SUCCESS) {
		error("Error registering the event handler.");
		device_close(device);
		return;
	}

	// Register the cancellation handler.
	rc = device_set_cancel(device, cancel_cb, &devicedata);
	if (rc != DEVICE_STATUS_SUCCESS) {
		error("Error registering the cancellation handler.");
		device_close(device);
		return;
	}

	rc = import_device_data(device, &devicedata);
	if (rc != DEVICE_STATUS_SUCCESS) {
		error("Dive data import error");
		device_close(device);
		return;
	}

	device_close(device);
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

