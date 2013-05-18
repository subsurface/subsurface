#include <libintl.h>
#include <glib/gi18n.h>
#include "dive.h"
#include "divelist.h"
#include "display.h"
#if USE_GTK_UI
#include "display-gtk.h"
#include "callbacks-gtk.h"
#endif
#include "libdivecomputer.h"

const char *default_dive_computer_vendor;
const char *default_dive_computer_product;
const char *default_dive_computer_device;

#if USE_GTK_UI
static gboolean force_download;
static gboolean prefer_downloaded;

OPTIONCALLBACK(force_toggle, force_download)
OPTIONCALLBACK(prefer_dl_toggle, prefer_downloaded)
#endif

struct product {
	const char *product;
	dc_descriptor_t *descriptor;
	struct product *next;
};

struct vendor {
	const char *vendor;
	struct product *productlist;
	struct vendor *next;
};

struct mydescriptor {
	const char *vendor;
	const char *product;
	dc_family_t type;
	unsigned int model;
};

struct vendor *dc_list;

#if USE_GTK_UI
static void render_dc_vendor(GtkCellLayout *cell,
		GtkCellRenderer *renderer,
		GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data)
{
	const char *vendor;

	gtk_tree_model_get(model, iter, 0, &vendor, -1);
	g_object_set(renderer, "text", vendor, NULL);
}

static void render_dc_product(GtkCellLayout *cell,
		GtkCellRenderer *renderer,
		GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data)
{
	dc_descriptor_t *descriptor = NULL;
	const char *product;

	gtk_tree_model_get(model, iter, 0, &descriptor, -1);
	product = dc_descriptor_get_product(descriptor);
	g_object_set(renderer, "text", product, NULL);
}
#endif

int is_default_dive_computer(const char *vendor, const char *product)
{
	return default_dive_computer_vendor && !strcmp(vendor, default_dive_computer_vendor) &&
		default_dive_computer_product && !strcmp(product, default_dive_computer_product);
}

int is_default_dive_computer_device(const char *name)
{
	return default_dive_computer_device && !strcmp(name, default_dive_computer_device);
}

void set_default_dive_computer(const char *vendor, const char *product)
{
	if (!vendor || !*vendor)
		return;
	if (!product || !*product)
		return;
	if (is_default_dive_computer(vendor, product))
		return;
	if (default_dive_computer_vendor)
		free((void *)default_dive_computer_vendor);
	if (default_dive_computer_product)
		free((void *)default_dive_computer_product);
	default_dive_computer_vendor = strdup(vendor);
	default_dive_computer_product = strdup(product);
	subsurface_set_conf("dive_computer_vendor", vendor);
	subsurface_set_conf("dive_computer_product", product);
}

void set_default_dive_computer_device(const char *name)
{
	if (!name || !*name)
		return;
	if (is_default_dive_computer_device(name))
		return;
	if (default_dive_computer_device)
		free((void *)default_dive_computer_device);
	default_dive_computer_device = strdup(name);
	subsurface_set_conf("dive_computer_device", name);
}

#if USE_GTK_UI
static void dive_computer_selector_changed(GtkWidget *combo, gpointer data)
{
	GtkWidget *import, *button;

#if DC_VERSION_CHECK(0, 4, 0)
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter)) {
		GtkTreeModel *model;
		dc_descriptor_t *descriptor;

		model = gtk_combo_box_get_model (GTK_COMBO_BOX(combo));
		gtk_tree_model_get(model, &iter, 0, &descriptor, -1);
		gtk_widget_set_sensitive(GTK_WIDGET(data), dc_descriptor_get_transport (descriptor) == DC_TRANSPORT_SERIAL);
	}
#endif

	import = gtk_widget_get_ancestor(combo, GTK_TYPE_DIALOG);
	button = gtk_dialog_get_widget_for_response(GTK_DIALOG(import), GTK_RESPONSE_ACCEPT);
	gtk_widget_set_sensitive(button, TRUE);
}

static GtkListStore **product_model;
static void dive_computer_vendor_changed(GtkComboBox *vendorcombo, GtkComboBox *productcombo)
{
	int vendor = gtk_combo_box_get_active(vendorcombo);
	gtk_combo_box_set_model(productcombo, GTK_TREE_MODEL(product_model[vendor + 1]));
	gtk_combo_box_set_active(productcombo, -1);
}

static GtkWidget *import_dive_computer(device_data_t *data, GtkDialog *dialog)
{
	GError *error;
	GtkWidget *vbox, *info, *container, *label, *button;

	/* HACK to simply include the Uemis Zurich in the list */
	if (! strcmp(data->vendor, "Uemis") && ! strcmp(data->product, "Zurich")) {
		error = uemis_download(data->devname, &data->progress, data->dialog, data->force_download);
	} else {
		error = do_import(data);
	}
	if (!error)
		return NULL;

	button = gtk_dialog_get_widget_for_response(dialog, GTK_RESPONSE_ACCEPT);
	gtk_button_set_use_stock(GTK_BUTTON(button), 0);
	gtk_button_set_label(GTK_BUTTON(button), _("Retry"));

	vbox = gtk_dialog_get_content_area(dialog);

	info = gtk_info_bar_new();
	container = gtk_info_bar_get_content_area(GTK_INFO_BAR(info));
	label = gtk_label_new(error->message);
	gtk_container_add(GTK_CONTAINER(container), label);
	gtk_box_pack_start(GTK_BOX(vbox), info, FALSE, FALSE, 0);
	return info;
}
#endif

/* create a list of lists and keep the elements sorted */
void add_dc(const char *vendor, const char *product, dc_descriptor_t *descriptor)
{
	struct vendor *dcl = dc_list;
	struct vendor **dclp = &dc_list;
	struct product *pl, **plp;

	if (!vendor || !product)
		return;
	while (dcl && strcmp(dcl->vendor, vendor) < 0) {
		dclp = &dcl->next;
		dcl = dcl->next;
	}
	if (!dcl || strcmp(dcl->vendor, vendor)) {
		dcl = calloc(sizeof(struct vendor), 1);
		dcl->next = *dclp;
		*dclp = dcl;
		dcl->vendor = strdup(vendor);
	}
	/* we now have a pointer to the requested vendor */
	plp = &dcl->productlist;
	pl = *plp;
	while (pl && strcmp(pl->product, product) < 0) {
		plp = &pl->next;
		pl = pl->next;
	}
	if (!pl || strcmp(pl->product, product)) {
		pl = calloc(sizeof(struct product), 1);
		pl->next = *plp;
		*plp = pl;
		pl->product = strdup(product);
	}
	/* one would assume that the vendor / product combinations are unique,
	 * but that is not the case. At the time of this writing, there are two
	 * flavors of the Oceanic OC1 - but looking at the code in libdivecomputer
	 * they are handled exactly the same, so we ignore this issue for now
	 *
	    if (pl->descriptor && memcmp(pl->descriptor, descriptor, sizeof(struct mydescriptor)))
		printf("duplicate entry with different descriptor for %s - %s\n", vendor, product);
	    else
	 */
	pl->descriptor = descriptor;
}

#if USE_GTK_UI
/* fill the vendors and create and fill the respective product stores; return the longest product name
 * and also the indices of the default vendor / product */
static int fill_computer_list(GtkListStore *vendorstore, GtkListStore ***productstore, int *vendor_index, int *product_index)
{
	int i, j, numvendor, width = 10;
	GtkTreeIter iter;
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;
	struct vendor *dcl;
	struct product *pl;
	GtkListStore **pstores;

	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next (iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
		add_dc(vendor, product, descriptor);
		if (product && strlen(product) > width)
			width = strlen(product);
	}
	dc_iterator_free(iterator);
	/* and add the Uemis Zurich which we are handling internally
	   THIS IS A HACK as we magically have a data structure here that
	   happens to match a data structure that is internal to libdivecomputer;
	   this WILL BREAK if libdivecomputer changes the dc_descriptor struct...
	   eventually the UEMIS code needs to move into libdivecomputer, I guess */
	mydescriptor = malloc(sizeof(struct mydescriptor));
	mydescriptor->vendor = "Uemis";
	mydescriptor->product = "Zurich";
	mydescriptor->type = DC_FAMILY_NULL;
	mydescriptor->model = 0;
	add_dc("Uemis", "Zurich", (dc_descriptor_t *)mydescriptor);
	dcl = dc_list;
	numvendor = 0;
	while (dcl) {
		numvendor++;
		dcl = dcl->next;
	}
	/* we need an extra vendor for the empty one */
	numvendor += 1;
	dcl = dc_list;
	i = 0;
	*vendor_index = *product_index = -1;
	if (*productstore)
		free(*productstore);
	pstores = *productstore = malloc(numvendor * sizeof(GtkListStore *));
	while (dcl) {
		gtk_list_store_append(vendorstore, &iter);
		gtk_list_store_set(vendorstore, &iter,
			0, dcl->vendor,
			-1);
		pl = dcl->productlist;
		pstores[i + 1] = gtk_list_store_new(1, G_TYPE_POINTER);
		j = 0;
		while (pl) {
			gtk_list_store_append(pstores[i + 1], &iter);
			gtk_list_store_set(pstores[i + 1], &iter,
				0, pl->descriptor,
				-1);
			if (is_default_dive_computer(dcl->vendor, pl->product)) {
				*vendor_index = i;
				*product_index = j;
			}
			j++;
			pl = pl->next;
		}
		i++;
		dcl = dcl->next;
	}
	/* now add the empty product list in case no vendor is selected */
	pstores[0] = gtk_list_store_new(1, G_TYPE_POINTER);

	return width;
}

static GtkComboBox *dive_computer_selector(GtkWidget *vbox)
{
	GtkWidget *hbox, *vendor_combo_box, *product_combo_box, *frame;
	GtkListStore *vendor_model;
	GtkCellRenderer *vendor_renderer, *product_renderer;
	int vendor_default_index, product_default_index, width;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	vendor_model = gtk_list_store_new(1, G_TYPE_POINTER);

	width = fill_computer_list(vendor_model, &product_model, &vendor_default_index, &product_default_index);

	frame = gtk_frame_new(_("Dive computer vendor and product"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vendor_combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(vendor_model));
	product_combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(product_model[vendor_default_index + 1]));

	g_signal_connect(G_OBJECT(vendor_combo_box), "changed", G_CALLBACK(dive_computer_vendor_changed), product_combo_box);
	gtk_box_pack_start(GTK_BOX(hbox), vendor_combo_box, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), product_combo_box, FALSE, FALSE, 3);

	vendor_renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(vendor_combo_box), vendor_renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(vendor_combo_box), vendor_renderer, render_dc_vendor, NULL, NULL);

	product_renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_fixed_size(product_renderer, 10 * width, -1);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(product_combo_box), product_renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(product_combo_box), product_renderer, render_dc_product, NULL, NULL);

	gtk_combo_box_set_active(GTK_COMBO_BOX(vendor_combo_box), vendor_default_index);
	gtk_combo_box_set_active(GTK_COMBO_BOX(product_combo_box), product_default_index);

	return GTK_COMBO_BOX(product_combo_box);
}

static GtkComboBox *dc_device_selector(GtkWidget *vbox)
{
	GtkWidget *hbox, *combo_box, *frame;
	GtkListStore *model;
	GtkCellRenderer *renderer;
	int default_index;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	model = gtk_list_store_new(1, G_TYPE_STRING);
	default_index = subsurface_fill_device_list(model);

	frame = gtk_frame_new(_("Device or mount point"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);

	combo_box = combo_box_with_model_and_entry(model);
	gtk_container_add(GTK_CONTAINER(frame), combo_box);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);

	if (default_index != -1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), default_index);
	else
		if (default_dive_computer_device)
			set_active_text(GTK_COMBO_BOX(combo_box), default_dive_computer_device);

	return GTK_COMBO_BOX(combo_box);
}

/* this prevents clicking the [x] button, while the import thread is still running */
static void download_dialog_delete(GtkWidget *w, gpointer data)
{
	/* a no-op */
}

void download_dialog(GtkWidget *w, gpointer data)
{
	int result;
	char *devname, *ns, *ne;
	GtkWidget *dialog, *button, *okbutton, *hbox, *vbox, *label, *info = NULL;
	GtkComboBox *computer, *device;
	GtkTreeIter iter;
	device_data_t devicedata = {
		.devname = NULL,
	};

	dialog = gtk_dialog_new_with_buttons(_("Download From Dive Computer"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(download_dialog_delete), NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	label = gtk_label_new(_(" Please select dive computer and device. "));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 3);
	computer = dive_computer_selector(vbox);
	device = dc_device_selector(vbox);
	g_signal_connect(G_OBJECT(computer), "changed", G_CALLBACK(dive_computer_selector_changed), device);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 3);
	devicedata.progress.bar = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(hbox), devicedata.progress.bar);

	force_download = FALSE;
	button = gtk_check_button_new_with_label(_("Force download of all dives"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(force_toggle), NULL);

	prefer_downloaded = FALSE;
	button = gtk_check_button_new_with_label(_("Always prefer downloaded dive"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(prefer_dl_toggle), NULL);

	okbutton = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	if (!gtk_combo_box_get_active_iter(computer, &iter))
		gtk_widget_set_sensitive(okbutton, FALSE);

repeat:
	gtk_widget_show_all(dialog);
	gtk_widget_set_sensitive(okbutton, TRUE);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
		dc_descriptor_t *descriptor;
		GtkTreeModel *model;

	case GTK_RESPONSE_ACCEPT:
		/* once the accept event is triggered the dialog becomes non-modal.
		 * lets re-set that */
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		if (info)
			gtk_widget_destroy(info);
		const char *vendor, *product;

		if (!gtk_combo_box_get_active_iter(computer, &iter))
			break;

		gtk_widget_set_sensitive(okbutton, FALSE);

		model = gtk_combo_box_get_model(computer);
		gtk_tree_model_get(model, &iter,
				0, &descriptor,
				-1);

		vendor = dc_descriptor_get_vendor(descriptor);
		product = dc_descriptor_get_product(descriptor);

		devicedata.descriptor = descriptor;
		devicedata.vendor = vendor;
		devicedata.product = product;
		set_default_dive_computer(vendor, product);

		/* get the device name from the combo box entry and set as default */
		devname = strdup(get_active_text(device));
		set_default_dive_computer_device(devname);
		/* clear leading and trailing white space from the device name and also
		 * everything after (and including) the first '(' char. */
		ns = devname;
		while (*ns == ' ' || *ns == '\t')
			ns++;
		ne = ns;
		while (*ne && *ne != '(')
			ne++;
		*ne = '\0';
		if (ne > ns)
			while (*(--ne) == ' ' || *ne == '\t')
				*ne = '\0';
		devicedata.devname = ns;
		devicedata.dialog = GTK_DIALOG(dialog);
		devicedata.force_download = force_download;
		force_download = FALSE; /* when retrying we don't want to restart */
		info = import_dive_computer(&devicedata, GTK_DIALOG(dialog));
		free((void *)devname);
		if (info)
			goto repeat;
		report_dives(TRUE, prefer_downloaded);
		break;
	default:
		/* it's possible that some dives were downloaded */
		report_dives(TRUE, prefer_downloaded);
		break;
	}
	gtk_widget_destroy(dialog);
}

void update_progressbar(progressbar_t *progress, double value)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->bar), value);
}

void update_progressbar_text(progressbar_t *progress, const char *text)
{
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->bar), text);
}
#endif
