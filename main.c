/* main.c */
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <glib/gi18n.h>

#include "dive.h"
#include "divelist.h"
#include "libdivecomputer.h"
#include "version.h"

struct preferences prefs;
struct preferences default_prefs = {
	.units = SI_UNITS,
	.visible_cols = { TRUE, FALSE, },
	.pp_graphs = {
		.po2 = FALSE,
		.pn2 = FALSE,
		.phe = FALSE,
		.po2_threshold =  1.6,
		.pn2_threshold =  4.0,
		.phe_threshold = 13.0,
	},
	.mod  = FALSE,
	.mod_ppO2  = 1.6,
	.ead  = FALSE,
	.profile_dc_ceiling = TRUE,
	.profile_red_ceiling  = FALSE,
	.profile_calc_ceiling = FALSE,
	.calc_ceiling_3m_incr = FALSE,
	.gflow = 30,
	.gfhigh = 75,
	.font_size = 14.0,
	.show_invalid = FALSE,
#ifdef USE_GTK_UI
	.map_provider = OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_HYBRID,
#endif
};

struct units *get_units()
{
	return &prefs.units;
}

/* random helper functions, used here or elsewhere */
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

void sort_table(struct dive_table *table)
{
	qsort(table->dives, table->nr, sizeof(struct dive *), sortfn);
}

const char *weekday(int wday)
{
	static const char wday_array[7][7] = {
		/*++GETTEXT: these are three letter days - we allow up to six code bytes */
		N_("Sun"), N_("Mon"), N_("Tue"), N_("Wed"), N_("Thu"), N_("Fri"), N_("Sat")
	};
	return _(wday_array[wday]);
}

const char *monthname(int mon)
{
	static const char month_array[12][7] = {
		/*++GETTEXT: these are three letter months - we allow up to six code bytes*/
		N_("Jan"), N_("Feb"), N_("Mar"), N_("Apr"), N_("May"), N_("Jun"),
		N_("Jul"), N_("Aug"), N_("Sep"), N_("Oct"), N_("Nov"), N_("Dec"),
	};
	return _(month_array[mon]);
}

/*
 * track whether we switched to importing dives
 */
static gboolean imported = FALSE;

static void parse_argument(const char *arg)
{
	const char *p = arg+1;

	do {
		switch (*p) {
		case 'v':
			verbose++;
			continue;
		case '-':
			/* long options with -- */
			if (strcmp(arg,"--import") == 0) {
				/* mark the dives so far as the base,
				 * everything after is imported */
				process_dives(FALSE, FALSE);
				imported = TRUE;
				return;
			}
			if (strcmp(arg, "--version") == 0) {
				printf("Subsurface v%s, ", VERSION_STRING);
				printf("built with libdivecomputer v%s\n", dc_version(NULL));
				exit(0);
			}
			/* fallthrough */
		case 'p':
			/* ignore process serial number argument when run as native macosx app */
			if (strncmp(arg, "-psn_", 5) == 0) {
				return;
			}
			/* fallthrough */
		default:
			fprintf(stderr, "Bad argument '%s'\n", arg);
			exit(1);
		}
	} while (*++p);
}

void renumber_dives(int nr)
{
	int i;

	for (i = 0; i < dive_table.nr; i++) {
		struct dive *dive = dive_table.dives[i];
		dive->number = nr + i;
	}
	mark_divelist_changed(TRUE);
}

/*
 * Under a POSIX setup, the locale string should have a format
 * like [language[_territory][.codeset][@modifier]].
 *
 * So search for the underscore, and see if the "territory" is
 * US, and turn on imperial units by default.
 *
 * I guess Burma and Liberia should trigger this too. I'm too
 * lazy to look up the territory names, though.
 */
static void setup_system_prefs(void)
{
	const char *env;

	default_prefs.divelist_font = strdup(system_divelist_default_font);
	default_prefs.default_filename = system_default_filename();

	env = getenv("LC_MEASUREMENT");
	if (!env)
		env = getenv("LC_ALL");
	if (!env)
		env = getenv("LANG");
	if (!env)
		return;
	env = strchr(env, '_');
	if (!env)
		return;
	env++;
	if (strncmp(env, "US", 2))
		return;

	default_prefs.units = IMPERIAL_units;
}

int main(int argc, char **argv)
{
	int i;
	gboolean no_filenames = TRUE;
	const char *path;
	char *error_message = NULL;

	/* set up l18n - the search directory needs to change
	 * so that it uses the correct system directory when
	 * subsurface isn't run from the local directory */
	path = subsurface_gettext_domainpath(argv[0]);
	setlocale(LC_ALL, "");
	bindtextdomain("subsurface", path);
	bind_textdomain_codeset("subsurface", "utf-8");
	textdomain("subsurface");

	setup_system_prefs();
	prefs = default_prefs;

	subsurface_command_line_init(&argc, &argv);
	parse_xml_init();

	init_ui(&argc, &argv);

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];
		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		set_filename(NULL, TRUE);

		parse_file(a, &error_message);
		if (no_filenames)
		{
			set_filename(a, TRUE);
			no_filenames = FALSE;
		}
	}
	if (no_filenames) {
		const char *filename = prefs.default_filename;
		parse_file(filename, NULL);
		/* don't report errors - this file may not exist, but make
		   sure we remember this as the filename in use */
		set_filename(filename, FALSE);
	}
	process_dives(imported, FALSE);
	parse_xml_exit();
	subsurface_command_line_exit(&argc, &argv);

	init_qt_ui(&argc, &argv, error_message); /* qt bit delayed until dives are parsed */
	run_ui();
	exit_ui();
	return 0;
}
