// SPDX-License-Identifier: GPL-2.0
/* unix.c */
/* implements UNIX specific functions */
#include "ssrf.h"
#include "dive.h"
#include "file.h"
#include "subsurface-string.h"
#include "display.h"
#include "membuffer.h"
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <zip.h>

// the DE should provide us with a default font and font size...
const char unix_system_divelist_default_font[] = "Sans";
const char *system_divelist_default_font = unix_system_divelist_default_font;
double system_divelist_default_font_size = -1.0;

void subsurface_OS_pref_setup(void)
{
	// nothing
}

bool subsurface_ignore_font(const char *font)
{
	// there are no old default fonts to ignore
	UNUSED(font);
	return false;
}

static const char *system_default_path_append(const char *append)
{
	const char *home = getenv("HOME");
	if (!home)
		home = "~";
	const char *path = "/.subsurface";

	int len = strlen(home) + strlen(path) + 1;
	if (append)
		len += strlen(append) + 1;

	char *buffer = (char *)malloc(len);
	memset(buffer, 0, len);
	strcat(buffer, home);
	strcat(buffer, path);
	if (append) {
		strcat(buffer, "/");
		strcat(buffer, append);
	}

	return buffer;
}

const char *system_default_directory(void)
{
	static const char *path = NULL;
	if (!path)
		path = system_default_path_append(NULL);
	return path;
}

const char *system_default_filename(void)
{
	static const char *path = NULL;
	if (!path) {
		const char *user = getenv("LOGNAME");
		if (empty_string(user))
			user = "username";
		char *filename = calloc(strlen(user) + 5, 1);
		strcat(filename, user);
		strcat(filename, ".xml");
		path = system_default_path_append(filename);
		free(filename);
	}
	return path;
}

int enumerate_devices(device_callback_t callback, void *userdata, unsigned int transport)
{
	int index = -1, entries = 0;
	DIR *dp = NULL;
	struct dirent *ep = NULL;
	size_t i;
	FILE *file;
	char *line = NULL;
	char *fname;
	size_t len;
	if (transport & DC_TRANSPORT_SERIAL) {
		const char *dirname = "/dev";
#ifdef __OpenBSD__
		const char *patterns[] = {
			"ttyU*",
			"ttyC*",
			NULL
		};
#else
		const char *patterns[] = {
			"ttyUSB*",
			"ttyS*",
			"ttyACM*",
			"rfcomm*",
			NULL
		};
#endif

		dp = opendir(dirname);
		if (dp == NULL) {
			return -1;
		}

		while ((ep = readdir(dp)) != NULL) {
			for (i = 0; patterns[i] != NULL; ++i) {
				if (fnmatch(patterns[i], ep->d_name, 0) == 0) {
					char filename[1024];
					int n = snprintf(filename, sizeof(filename), "%s/%s", dirname, ep->d_name);
					if (n >= (int)sizeof(filename)) {
						closedir(dp);
						return -1;
					}
					callback(filename, userdata);
					if (is_default_dive_computer_device(filename))
						index = entries;
					entries++;
					break;
				}
			}
		}
		closedir(dp);
	}
#ifdef __linux__
	if (transport & DC_TRANSPORT_USBSTORAGE) {
		int num_uemis = 0;
		file = fopen("/proc/mounts", "r");
		if (file == NULL)
			return index;

		while ((getline(&line, &len, file)) != -1) {
			char *ptr = strstr(line, "UEMISSDA");
			if (!ptr)
				ptr = strstr(line, "GARMIN");
			if (ptr) {
				char *end = ptr, *start = ptr;
				while (start > line && *start != ' ')
					start--;
				if (*start == ' ')
					start++;
				while (*end != ' ' && *end != '\0')
					end++;

				*end = '\0';
				fname = strdup(start);

				callback(fname, userdata);

				if (is_default_dive_computer_device(fname))
					index = entries;
				entries++;
				num_uemis++;
				free((void *)fname);
			}
		}
		free(line);
		fclose(file);
		if (num_uemis == 1 && entries == 1) /* if we found only one and it's a mounted Uemis, pick it */
			index = 0;
	}
#endif
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

int subsurface_access(const char *path, int mode)
{
	return access(path, mode);
}

int subsurface_stat(const char* path, struct stat* buf)
{
	return stat(path, buf);
}

struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp)
{
	return zip_open(path, flags, errorp);
}

int subsurface_zip_close(struct zip *zip)
{
	return zip_close(zip);
}

/* win32 console */
void subsurface_console_init(void)
{
	/* NOP */
}

void subsurface_console_exit(void)
{
	/* NOP */
}

bool subsurface_user_is_root()
{
	return geteuid() == 0;
}
