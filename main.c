#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

GtkWidget *main_window;

static int sortfn(const void *_a, const void *_b)
{
	const struct dive *a = *(void **)_a;
	const struct dive *b = *(void **)_b;

	if (a->when < b->when)
		return -1;
	if (a->when > b->when)
		return 1;
	return 0;
}

static int alloc_samples;

/* Don't pick a zero for MERGE_MIN() */
#define MERGE_MAX(res, a, b, n) res->n = MAX(a->n, b->n)
#define MERGE_MIN(res, a, b, n) res->n = (a->n)?(b->n)?MIN(a->n, b->n):(a->n):(b->n)

static struct dive *add_sample(struct sample *sample, int time, struct dive *dive)
{
	int nr = dive->samples;
	struct sample *d;

	if (nr >= alloc_samples) {
		alloc_samples = (alloc_samples + 64) * 3 / 2;
		dive = realloc(dive, dive_size(alloc_samples));
		if (!dive)
			return NULL;
	}
	dive->samples = nr+1;
	d = dive->sample + nr;

	*d = *sample;
	d->time.seconds = time;
	return dive;
}

/*
 * Merge samples. Dive 'a' is "offset" seconds before Dive 'b'
 */
static struct dive *merge_samples(struct dive *res, struct dive *a, struct dive *b, int offset)
{
	int asamples = a->samples;
	int bsamples = b->samples;
	struct sample *as = a->sample;
	struct sample *bs = b->sample;

	for (;;) {
		int at, bt;
		struct sample sample;

		if (!res)
			return NULL;

		at = asamples ? as->time.seconds : -1;
		bt = bsamples ? bs->time.seconds + offset : -1;

		/* No samples? All done! */
		if (at < 0 && bt < 0)
			return res;

		/* Only samples from a? */
		if (bt < 0) {
add_sample_a:
			res = add_sample(as, at, res);
			as++;
			asamples--;
			continue;
		}

		/* Only samples from b? */
		if (at < 0) {
add_sample_b:
			res = add_sample(bs, bt, res);
			bs++;
			bsamples--;
			continue;
		}

		if (at < bt)
			goto add_sample_a;
		if (at > bt)
			goto add_sample_b;

		/* same-time sample: add a merged sample. Take the non-zero ones */
		sample = *bs;
		if (as->depth.mm)
			sample.depth = as->depth;
		if (as->temperature.mkelvin)
			sample.temperature = as->temperature;
		if (as->tankpressure.mbar)
			sample.tankpressure = as->tankpressure;
		if (as->tankindex)
			sample.tankindex = as->tankindex;

		res = add_sample(&sample, at, res);

		as++;
		bs++;
		asamples--;
		bsamples--;
	}
}

static char *merge_text(const char *a, const char *b)
{
	char *res;

	if (!a || !*a)
		return (char *)b;
	if (!b || !*b)
		return (char *)a;
	if (!strcmp(a,b))
		return (char *)a;
	res = malloc(strlen(a) + strlen(b) + 9);
	if (!res)
		return (char *)a;
	sprintf(res, "(%s) or (%s)", a, b);
	return res;
}

/*
 * This could do a lot more merging. Right now it really only
 * merges almost exact duplicates - something that happens easily
 * with overlapping dive downloads.
 */
static struct dive *try_to_merge(struct dive *a, struct dive *b)
{
	int i;
	struct dive *res;

	if (a->when != b->when)
		return NULL;

	alloc_samples = 5;
	res = malloc(dive_size(alloc_samples));
	if (!res)
		return NULL;
	memset(res, 0, dive_size(alloc_samples));

	res->when = a->when;
	res->name = merge_text(a->name, b->name);
	res->location = merge_text(a->location, b->location);
	res->notes = merge_text(a->notes, b->notes);
	MERGE_MAX(res, a, b, maxdepth.mm);
	MERGE_MAX(res, a, b, meandepth.mm);	/* recalc! */
	MERGE_MAX(res, a, b, duration.seconds);
	MERGE_MAX(res, a, b, surfacetime.seconds);
	MERGE_MAX(res, a, b, airtemp.mkelvin);
	MERGE_MIN(res, a, b, watertemp.mkelvin);
	MERGE_MAX(res, a, b, beginning_pressure.mbar);
	MERGE_MAX(res, a, b, end_pressure.mbar);
	for (i = 0; i < MAX_MIXES; i++) {
		if (a->gasmix[i].o2.permille) {
			res->gasmix[i] = a->gasmix[i];
			continue;
		}
		res->gasmix[i] = b->gasmix[i];
	}
	return merge_samples(res, a, b, 0);
}

/*
 * This doesn't really report anything at all. We just sort the
 * dives, the GUI does the reporting
 */
static void report_dives(void)
{
	int i;

	qsort(dive_table.dives, dive_table.nr, sizeof(struct dive *), sortfn);

	for (i = 1; i < dive_table.nr; i++) {
		struct dive **pp = &dive_table.dives[i-1];
		struct dive *prev = pp[0];
		struct dive *dive = pp[1];
		struct dive *merged;

		if (prev->when + prev->duration.seconds < dive->when)
			continue;

		merged = try_to_merge(prev, dive);
		if (!merged)
			continue;

		free(prev);
		free(dive);
		*pp = merged;
		dive_table.nr--;
		memmove(pp+1, pp+2, sizeof(*pp)*(dive_table.nr - i));

		/* Redo the new 'i'th dive */
		i--;
	}
}

static void parse_argument(const char *arg)
{
	const char *p = arg+1;

	do {
		switch (*p) {
		case 'v':
			verbose++;
			continue;
		default:
			fprintf(stderr, "Bad argument '%s'\n", arg);
			exit(1);
		}
	} while (*++p);
}

static void on_destroy(GtkWidget* w, gpointer data)
{
	gtk_main_quit();
}

static GtkWidget *dive_profile;

void repaint_dive(void)
{
	update_dive_info(current_dive);
	gtk_widget_queue_draw(dive_profile);
}

static char *existing_filename;

static void file_open(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Open File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		printf("Open: '%s'\n", filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static void file_save(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Save File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	if (!existing_filename) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "Untitled document");
	} else
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), existing_filename);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		save_dives(filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static GtkItemFactoryEntry menu_items[] = {
	{ "/_File",		NULL,		NULL,		0, "<Branch>" },
	{ "/File/_Open",	"<control>O",	file_open,	0, "<StockItem>", GTK_STOCK_OPEN },
	{ "/File/_Save",	"<control>S",	file_save,	0, "<StockItem>", GTK_STOCK_SAVE },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

/* This is just directly from the gtk menubar tutorial. */
static GtkWidget *get_menubar_menu(GtkWidget *window)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	return gtk_item_factory_get_widget(item_factory, "<main>");
}

int main(int argc, char **argv)
{
	int i;
	GtkWidget *win;
	GtkWidget *divelist;
	GtkWidget *table;
	GtkWidget *notebook;
	GtkWidget *frame;
	GtkWidget *menubar;
	GtkWidget *vbox;

	parse_xml_init();

	gtk_init(&argc, &argv);

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		parse_xml_file(a);
	}

	report_dives();

	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(win), "destroy",      G_CALLBACK(on_destroy), NULL);
	main_window = win;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(win), vbox);

	menubar = get_menubar_menu(win);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

	/* Table for the list of dives, cairo window, and dive info */
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 5);
	gtk_box_pack_end(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	/* Create the atual divelist */
	divelist = create_dive_list();
	gtk_table_attach(GTK_TABLE(table), divelist, 0, 1, 0, 2,
		0, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);

	/* Frame for minimal dive info */
	frame = dive_info_frame();
	gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 0, 1,
		 GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0, 0);

	/* Notebook for dive info vs profile vs .. */
	notebook = gtk_notebook_new();
	gtk_table_attach_defaults(GTK_TABLE(table), notebook, 1, 2, 1, 2);

	/* Frame for dive profile */
	frame = dive_profile_frame();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Dive Profile"));
	dive_profile = frame;

	/* Frame for extended dive info */
	frame = extended_dive_info_frame();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Extended dive Info"));

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	gtk_main();
	return 0;
}
