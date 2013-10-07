/* macos.c */
/* implements Mac OS X specific functions */
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>
#include "dive.h"
#include "display.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <mach-o/dyld.h>
#include <sys/syslimits.h>

/* macos defines CFSTR to create a CFString object from a constant,
 * but no similar macros if a C string variable is supposed to be
 * the argument. We add this here (hardcoding the default allocator
 * and MacRoman encoding */
#define CFSTR_VAR(_var) CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,	\
					(_var), kCFStringEncodingMacRoman,	\
					kCFAllocatorNull)

#define SUBSURFACE_PREFERENCES CFSTR("org.hohndel.subsurface")
#define ICON_NAME "Subsurface.icns"
#define UI_FONT "Arial 12"

const char system_divelist_default_font[] = "Arial 10";

const char *system_default_filename(void)
{
	const char *home, *user;
	char *buffer;
	int len;

	home = getenv("HOME");
	user = getenv("LOGNAME");
	len = strlen(home) + strlen(user) + 45;
	buffer = malloc(len);
	snprintf(buffer, len, "%s/Library/Application Support/Subsurface/%s.xml", home, user);
	return buffer;
}

void subsurface_command_line_init(int *argc, char ***argv)
{
	/* this is a no-op */
}

void subsurface_command_line_exit(int *argc, char ***argv)
{
	/* this is a no-op */
}

bool subsurface_os_feature_available(os_feature_t f)
{
	return TRUE;
}

int enumerate_devices (device_callback_t callback, void *userdata)
{
	int index = -1;
	DIR *dp = NULL;
	struct dirent *ep = NULL;
	size_t i;
	const char *dirname = "/dev";
	const char *patterns[] = {
		"tty.*",
		"usbserial",
		NULL
	};

	dp = opendir (dirname);
	if (dp == NULL) {
		return -1;
	}

	while ((ep = readdir (dp)) != NULL) {
		for (i = 0; patterns[i] != NULL; ++i) {
			if (fnmatch (patterns[i], ep->d_name, 0) == 0) {
				char filename[1024];
				int n = snprintf (filename, sizeof (filename), "%s/%s", dirname, ep->d_name);
				if (n >= sizeof (filename)) {
					closedir (dp);
					return -1;
				}
				callback (filename, userdata);
				if (is_default_dive_computer_device(filename))
					index = i;
				break;
			}
		}
	}
	// TODO: list UEMIS mount point from /proc/mounts

	closedir (dp);
	return index;
}
