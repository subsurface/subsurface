#include "subsurfacestartup.h"
#include "ssrf-version.h"
#include <stdbool.h>
#include <string.h>
#include "gettext.h"
struct preferences prefs;
struct preferences default_prefs = {
	.units = SI_UNITS,
	.unit_system = METRIC,
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
	.calc_ndl_tts = FALSE,
	.gflow = 30,
	.gfhigh = 75,
	.font_size = 14.0,
	.show_invalid = FALSE,
	.show_sac = FALSE,
};

struct units *get_units()
{
	return &prefs.units;
}

/* random helper functions, used here or elsewhere */
static int sortfn(const void *_a, const void *_b)
{
	const struct dive *a = (const struct dive*) *(void **)_a;
	const struct dive *b = (const struct dive*) *(void **)_b;

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
		QT_TRANSLATE_NOOP("gettextFromC","Sun"), QT_TRANSLATE_NOOP("gettextFromC","Mon"), QT_TRANSLATE_NOOP("gettextFromC","Tue"), QT_TRANSLATE_NOOP("gettextFromC","Wed"), QT_TRANSLATE_NOOP("gettextFromC","Thu"), QT_TRANSLATE_NOOP("gettextFromC","Fri"), QT_TRANSLATE_NOOP("gettextFromC","Sat")
	};
	return translate("gettextFromC",wday_array[wday]);
}

const char *monthname(int mon)
{
	static const char month_array[12][7] = {
		/*++GETTEXT: these are three letter months - we allow up to six code bytes*/
		QT_TRANSLATE_NOOP("gettextFromC","Jan"), QT_TRANSLATE_NOOP("gettextFromC","Feb"), QT_TRANSLATE_NOOP("gettextFromC","Mar"), QT_TRANSLATE_NOOP("gettextFromC","Apr"), QT_TRANSLATE_NOOP("gettextFromC","May"), QT_TRANSLATE_NOOP("gettextFromC","Jun"),
		QT_TRANSLATE_NOOP("gettextFromC","Jul"), QT_TRANSLATE_NOOP("gettextFromC","Aug"), QT_TRANSLATE_NOOP("gettextFromC","Sep"), QT_TRANSLATE_NOOP("gettextFromC","Oct"), QT_TRANSLATE_NOOP("gettextFromC","Nov"), QT_TRANSLATE_NOOP("gettextFromC","Dec"),
	};
	return translate("gettextFromC",month_array[mon]);
}

/*
 * track whether we switched to importing dives
 */
bool imported = FALSE;

static void print_version() {
	printf("Subsurface v%s, ", VERSION_STRING);
	printf("built with libdivecomputer v%s\n", dc_version(NULL));
}

static void print_help() {
	print_version();
	printf("\nUsage: subsurface [options] [logfile ...] [--import logfile ...]");
	printf("\n\noptions include:");
	printf("\n --help|-h             This help text");
	printf("\n --import logfile ...  Logs before this option is treated as base, everything after is imported");
	printf("\n --verbose|-v          Verbose debug (repeat to increase verbosity)");
	printf("\n --version             Prints current version\n\n");
}

void parse_argument(const char *arg)
{
	const char *p = arg+1;

	do {
		switch (*p) {
		case 'h':
			print_help();
			exit(0);
		case 'v':
			verbose++;
			continue;
		case 'q':
			quit++;
			continue;
		case '-':
			/* long options with -- */
			if (strcmp(arg, "--help") == 0) {
				print_help();
				exit(0);
			}
			if (strcmp(arg, "--import") == 0) {
				imported = TRUE; /* mark the dives so far as the base, * everything after is imported */
				return;
			}
			if (strcmp(arg, "--verbose") == 0) {
				verbose++;
				return;
			}
			if (strcmp(arg, "--version") == 0) {
				print_version();
				exit(0);
			}
			/* fallthrough */
		case 'p':
			/* ignore process serial number argument when run as native macosx app */
			if (strncmp(arg, "-psQT_TR_NOOP(", 5) == 0) {
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
void setup_system_prefs(void)
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
