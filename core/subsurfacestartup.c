// SPDX-License-Identifier: GPL-2.0
#include "subsurfacestartup.h"
#include "subsurface-string.h"
#include "version.h"
#include "errorhelper.h"
#include "gettext.h"
#include "qthelper.h"
#include "git-access.h"
#include "pref.h"
#include "libdivecomputer/version.h"

#include <stdbool.h>
#include <string.h>

extern void show_computer_list();

int quit, force_root, ignore_bt;
#ifdef SUBSURFACE_MOBILE_DESKTOP
char *testqml = NULL;
#endif

/*
 * track whether we switched to importing dives
 */
bool imported = false;

void print_version()
{
	static bool version_printed = false;
	if (version_printed)
		return;
#if defined(SUBSURFACE_DOWNLOADER)
	printf("Subsurface-downloader v%s,\n", subsurface_git_version());
#else
	printf("Subsurface v%s,\n", subsurface_git_version());
#endif
	printf("built with libdivecomputer v%s\n", dc_version(NULL));
	print_qt_versions();
	int git_maj, git_min, git_rev;
	git_libgit2_version(&git_maj, &git_min, &git_rev);
	printf("built with libgit2 %d.%d.%d\n", git_maj, git_min, git_rev);
	version_printed = true;
}

void print_files()
{
	const char *branch = 0;
	const char *remote = 0;
	const char *filename, *local_git;

	printf("\nFile locations:\n\n");
	printf("Cloud email:%s\n", prefs.cloud_storage_email);
	if (!empty_string(prefs.cloud_storage_email) && !empty_string(prefs.cloud_storage_password)) {
		filename = cloud_url();

		is_git_repository(filename, &branch, &remote, true);
	} else {
		/* strdup so the free below works in either case */
		filename = strdup("No valid cloud credentials set.\n");
	}
	if (branch && remote) {
		local_git = get_local_dir(remote, branch);
		printf("Local git storage: %s\n", local_git);
	} else {
		printf("Unable to get local git directory\n");
	}
	printf("Cloud URL: %s\n", filename);
	free((void *)filename);
	char *tmp = hashfile_name_string();
	printf("Image filename table: %s\n", tmp);
	free(tmp);
	tmp = picturedir_string();
	printf("Local picture directory: %s\n\n", tmp);
	free(tmp);
}

static void print_help()
{
	print_version();
	printf("\nUsage: subsurface [options] [logfile ...] [--import logfile ...]");
	printf("\n\noptions include:");
	printf("\n --help|-h             This help text");
	printf("\n --ignore-bt           Don't enable Bluetooth support");
	printf("\n --import logfile ...  Logs before this option is treated as base, everything after is imported");
	printf("\n --verbose|-v          Verbose debug (repeat to increase verbosity)");
	printf("\n --version             Prints current version");
	printf("\n --user=<test>         Choose configuration space for user <test>");
#ifdef SUBSURFACE_MOBILE_DESKTOP
	printf("\n --testqml=<dir>       Use QML files from <dir> instead of QML resources");
#elif SUBSURFACE_DOWNLOADER
	printf("\n --dc-vendor=vendor    Set the dive computer to download from");
	printf("\n --dc-product=product  Set the dive computer to download from");
	printf("\n --device=device       Set the device to download from");
#endif
	printf("\n --cloud-timeout=<nr>  Set timeout for cloud connection (0 < timeout < 60)\n\n");
}

void parse_argument(const char *arg)
{
	const char *p = arg + 1;

	do {
		switch (*p) {
		case 'h':
			print_help();
			exit(0);
		case 'v':
			print_version();
			verbose++;
			continue;
		case 'q':
			quit++;
			continue;
		case '-':
			/* long options with -- */
			/* first test for --user=bla which allows the use of user specific settings */
			if (strncmp(arg, "--user=", sizeof("--user=") - 1) == 0) {
				settings_suffix = strdup(arg + sizeof("--user=") - 1);
				return;
			}
			if (strncmp(arg, "--cloud-timeout=", sizeof("--cloud-timeout=") - 1) == 0) {
				const char *timeout = arg + sizeof("--cloud-timeout=") - 1;
				int to = strtol(timeout, NULL, 10);
				if (0 < to && to < 60)
					default_prefs.cloud_timeout = to;
				return;
			}
			if (strcmp(arg, "--help") == 0) {
				print_help();
				exit(0);
			}
			if (strcmp(arg, "--ignore-bt") == 0) {
				ignore_bt = true;
				return;
			}
			if (strcmp(arg, "--import") == 0) {
				imported = true; /* mark the dives so far as the base, * everything after is imported */
				return;
			}
			if (strcmp(arg, "--verbose") == 0) {
				print_version();
				verbose++;
				return;
			}
			if (strcmp(arg, "--version") == 0) {
				print_version();
				exit(0);
			}
			if (strcmp(arg, "--allow_run_as_root") == 0) {
				++force_root;
				return;
			}
#if SUBSURFACE_DOWNLOADER
			if (strncmp(arg, "--dc-vendor=", sizeof("--dc-vendor=") - 1) == 0) {
				prefs.dive_computer.vendor = strdup(arg + sizeof("--dc-vendor=") - 1);
				return;
			}
			if (strncmp(arg, "--dc-product=", sizeof("--dc-product=") - 1) == 0) {
				prefs.dive_computer.product = strdup(arg + sizeof("--dc-product=") - 1);
				return;
			}
			if (strncmp(arg, "--device=", sizeof("--device=") - 1) == 0) {
				prefs.dive_computer.device = strdup(arg + sizeof("--device=") - 1);
				return;
			}
			if (strncmp(arg, "--list-dc", sizeof("--list-dc") - 1) == 0) {
				show_computer_list();
				exit(0);
			}
#elif SUBSURFACE_MOBILE_DESKTOP
			if (strncmp(arg, "--testqml=", sizeof("--testqml=") - 1) == 0) {
				testqml = malloc(strlen(arg) - sizeof("--testqml=") + 1);
				strcpy(testqml, arg + sizeof("--testqml=") - 1);
				return;
			}
#endif
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

	subsurface_OS_pref_setup();
	default_prefs.divelist_font = strdup(system_divelist_default_font);
	default_prefs.font_size = system_divelist_default_font_size;
	default_prefs.ffmpeg_executable = strdup("ffmpeg");

#if !defined(SUBSURFACE_MOBILE)
	default_prefs.default_filename = copy_string(system_default_filename());
#endif
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
