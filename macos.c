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
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>

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

/* NOP wrappers to comform with windows.c */
int subsurface_rename(const char *path, const char *newpath)
{
	return rename(path, newpath);
}

int subsurface_open(const char *path, int oflags, mode_t mode)
{
	return open(path, oflags, mode);
}

FILE *subsurface_fopen(const char *path, const char *mode)
{
	return fopen(path, mode);
}

void *subsurface_opendir(const char *path)
{
	return (void *)opendir(path);
}

struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp)
{
	return zip_open(path, flags, errorp);
}

int subsurface_zip_close(struct zip *zip)
{
	return zip_close(zip);
}
