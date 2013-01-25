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

#ifdef DEBUGFILE
char *debugfilename;
FILE *debugfile;
#endif

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
	.profile_red_ceiling  = FALSE,
	.profile_calc_ceiling = FALSE,
	.calc_ceiling_3m_incr = FALSE,
	.gflow = 0.30,
	.gfhigh = 0.75,
};

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
 * When adding dives to the dive table, we try to renumber
 * the new dives based on any old dives in the dive table.
 *
 * But we only do it if:
 *
 *  - there are no dives in the dive table
 *
 *  OR
 *
 *  - the last dive in the old dive table was numbered
 *
 *  - all the new dives are strictly at the end (so the
 *    "last dive" is at the same location in the dive table
 *    after re-sorting the dives.
 *
 *  - none of the new dives have any numbers
 *
 * This catches the common case of importing new dives from
 * a dive computer, and gives them proper numbers based on
 * your old dive list. But it tries to be very conservative
 * and not give numbers if there is *any* question about
 * what the numbers should be - in which case you need to do
 * a manual re-numbering.
 */
static void try_to_renumber(struct dive *last, int preexisting)
{
	int i, nr;

	/*
	 * If the new dives aren't all strictly at the end,
	 * we're going to expect the user to do a manual
	 * renumbering.
	 */
	if (preexisting && get_dive(preexisting-1) != last)
		return;

	/*
	 * If any of the new dives already had a number,
	 * we'll have to do a manual renumbering.
	 */
	for (i = preexisting; i < dive_table.nr; i++) {
		struct dive *dive = get_dive(i);
		if (dive->number)
			return;
	}

	/*
	 * Ok, renumber..
	 */
	if (last)
		nr = last->number;
	else
		nr = 0;
	for (i = preexisting; i < dive_table.nr; i++) {
		struct dive *dive = get_dive(i);
		dive->number = ++nr;
	}
}

/*
 * track whether we switched to importing dives
 */
static gboolean imported = FALSE;

/*
 * This doesn't really report anything at all. We just sort the
 * dives, the GUI does the reporting
 */
void report_dives(gboolean is_imported, gboolean prefer_imported)
{
	int i;
	int preexisting = dive_table.preexisting;
	struct dive *last;

	/* check if we need a nickname for the divecomputer for newly downloaded dives;
	 * since we know they all came from the same divecomputer we just check for the
	 * first one */
	if (preexisting < dive_table.nr && dive_table.dives[preexisting]->downloaded)
		set_dc_nickname(dive_table.dives[preexisting]);
	else
		/* they aren't downloaded, so record / check all new ones */
		for (i = preexisting; i < dive_table.nr; i++)
			set_dc_nickname(dive_table.dives[i]);

	/* This does the right thing for -1: NULL */
	last = get_dive(preexisting-1);

	qsort(dive_table.dives, dive_table.nr, sizeof(struct dive *), sortfn);

	for (i = 1; i < dive_table.nr; i++) {
		struct dive **pp = &dive_table.dives[i-1];
		struct dive *prev = pp[0];
		struct dive *dive = pp[1];
		struct dive *merged;

		/* only try to merge overlapping dives - or if one of the dives has
		 * zero duration (that might be a gps marker from the webservice) */
		if (prev->dc.duration.seconds && dive->dc.duration.seconds &&
		    prev->when + prev->dc.duration.seconds < dive->when)
			continue;

		merged = try_to_merge(prev, dive, prefer_imported);
		if (!merged)
			continue;

		/* careful - we might free the dive that last points to. Oops... */
		if (last == prev || last == dive)
			last = merged;

		/* Redo the new 'i'th dive */
		i--;
		add_single_dive(i, merged);
		delete_single_dive(i+1);
		delete_single_dive(i+1);
	}
	/* make sure no dives are still marked as downloaded */
	for (i = 1; i < dive_table.nr; i++)
		dive_table.dives[i]->downloaded = FALSE;

	if (is_imported) {
		/* If there are dives in the table, are they numbered */
		if (!last || last->number)
			try_to_renumber(last, preexisting);

		/* did we add dives to the dive table? */
		if (preexisting != dive_table.nr)
			mark_divelist_changed(TRUE);
	}
	dive_list_update_dives();
}

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
				report_dives(FALSE, FALSE);
				imported = TRUE;
				return;
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

void update_dive(struct dive *new_dive)
{
	static struct dive *buffered_dive;
	struct dive *old_dive = buffered_dive;

	if (old_dive) {
		flush_divelist(old_dive);
	}
	show_dive_info(new_dive);
	if (new_dive) {
		show_dive_equipment(new_dive, W_IDX_PRIMARY);
		show_dive_stats(new_dive);
	}
	buffered_dive = new_dive;
}

void renumber_dives(int nr)
{
	int i;

	for (i = 0; i < dive_table.nr; i++) {
		struct dive *dive = dive_table.dives[i];
		dive->number = nr + i;
		flush_divelist(dive);
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
	default_prefs.default_filename = strdup(system_default_filename());

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

	/* set up l18n - the search directory needs to change
	 * so that it uses the correct system directory when
	 * subsurface isn't run from the local directory */
	path = subsurface_gettext_domainpath(argv[0]);
	setlocale( LC_ALL, "" );
	bindtextdomain("subsurface", path);
	bind_textdomain_codeset("subsurface", "utf-8");
	textdomain("subsurface");

	setup_system_prefs();
	prefs = default_prefs;

#if DEBUGFILE > 1
	debugfile = stderr;
#elif defined(DEBUGFILE)
	debugfilename = strdup(prefs.default_filename);
	strncpy(debugfilename + strlen(debugfilename) - 3, "log", 3);
	if (g_mkdir_with_parents(g_path_get_dirname(debugfilename), 0664) != 0 ||
	    (debugfile = g_fopen(debugfilename, "w")) == NULL)
		printf("oh boy, can't create debugfile");
#endif

	subsurface_command_line_init(&argc, &argv);
	parse_xml_init();

	init_ui(&argc, &argv);

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		no_filenames = FALSE;
		GError *error = NULL;
		parse_file(a, &error, TRUE);

		if (error != NULL)
		{
			report_error(error);
			g_error_free(error);
			error = NULL;
		}
	}
	if (no_filenames) {
		GError *error = NULL;
		const char *filename = prefs.default_filename;
		parse_file(filename, &error, TRUE);
		/* don't report errors - this file may not exist, but make
		   sure we remember this as the filename in use */
		set_filename(filename, FALSE);
	}
	report_dives(imported, FALSE);
	if (dive_table.nr == 0)
		show_dive_info(NULL);
	run_ui();
	exit_ui();

	parse_xml_exit();
	subsurface_command_line_exit(&argc, &argv);

#ifdef DEBUGFILE
	if (debugfile)
		fclose(debugfile);
#endif
	return 0;
}
