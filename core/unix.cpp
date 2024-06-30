// SPDX-License-Identifier: GPL-2.0
/* unix.c */
/* implements UNIX specific functions */
#include "file.h"
#include "subsurfacestartup.h"
#include "subsurface-string.h"
#include "device.h"
#include "libdivecomputer.h"
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
#include <string>

static std::string system_default_path()
{
	std::string home(getenv("HOME"));
	if (home.empty())
		home = "~";
	return home + "/.subsurface";
}

static std::string make_default_filename()
{
	std::string user = getenv("LOGNAME");
	if (user.empty())
		user = "username";
	return system_default_path() + "/" + user + ".xml";
}

// the DE should provide us with a default font and font size...
using namespace std::string_literals;
std::string system_divelist_default_font = "Sans"s;
double system_divelist_default_font_size = -1.0;

void subsurface_OS_pref_setup()
{
	// nothing
}

bool subsurface_ignore_font(const std::string &)
{
	// there are no old default fonts to ignore
	return false;
}

std::string system_default_directory()
{
	static const std::string path = system_default_path();
	return path;
}

std::string system_default_filename()
{
	static const std::string fn = make_default_filename();
	return fn;
}

int enumerate_devices(device_callback_t callback, void *userdata, unsigned int transport)
{
	int index = -1, entries = 0;
	DIR *dp = NULL;
	struct dirent *ep = NULL;
	FILE *file;
	char *line = NULL;
	size_t len;
	if (transport & DC_TRANSPORT_SERIAL) {
		const char *dirname = "/dev";
#ifdef __OpenBSD__
		const char *patterns[] = {
			"ttyU*",
			"ttyC*"
		};
#else
		const char *patterns[] = {
			"ttyUSB*",
			"ttyS*",
			"ttyACM*",
			"rfcomm*"
		};
#endif

		dp = opendir(dirname);
		if (dp == NULL) {
			return -1;
		}

		while ((ep = readdir(dp)) != NULL) {
			for (const char *pattern: patterns) {
				if (fnmatch(pattern, ep->d_name, 0) == 0) {
					std::string filename = std::string(dirname) + "/" + ep->d_name;
					callback(filename.c_str(), userdata);
					if (is_default_dive_computer_device(filename.c_str()))
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

				callback(start, userdata);

				if (is_default_dive_computer_device(start))
					index = entries;
				entries++;
				num_uemis++;
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
void subsurface_console_init()
{
	/* NOP */
}

void subsurface_console_exit()
{
	/* NOP */
}

bool subsurface_user_is_root()
{
	return geteuid() == 0;
}
