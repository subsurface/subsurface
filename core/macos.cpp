// SPDX-License-Identifier: GPL-2.0
/* macos.c */
/* implements Mac OS X specific functions */
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>
#include "device.h"
#include "subsurfacestartup.h"
#include "subsurface-string.h"
#include "libdivecomputer.h"
#include <CoreFoundation/CoreFoundation.h>
#if !defined(__IPHONE_5_0)
#include <CoreServices/CoreServices.h>
#endif
#include <mach-o/dyld.h>
#include <sys/syslimits.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <zip.h>
#include <sys/stat.h>
#include <string>

/* macos defines CFSTR to create a CFString object from a constant,
 * but no similar macros if a C string variable is supposed to be
 * the argument. We add this here (hardcoding the default allocator
 * and MacRoman encoding */
#define CFSTR_VAR(_var) CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,               \
							(_var), kCFStringEncodingMacRoman, \
							kCFAllocatorNull)

#define SUBSURFACE_PREFERENCES CFSTR("org.hohndel.subsurface")
#define ICON_NAME "Subsurface.icns"
#define UI_FONT "Arial 12"

static std::string system_default_path()
{
	std::string home(getenv("HOME"));
	return home + "/Library/Application Support/Subsurface";
}

static std::string make_default_filename()
{
	std::string user = getenv("LOGNAME");
	if (user.empty())
		user = "username";
	return system_default_path() + "/" + user + ".xml";
}

using namespace std::string_literals;
std::string system_divelist_default_font = "Arial"s;
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
	return path.c_str();
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
	if (transport & DC_TRANSPORT_SERIAL) {
		// on Mac we always support FTDI now
		callback("FTDI", userdata);

		const char *dirname = "/dev";
		const char *patterns[] = {
			"tty.*",
			"usbserial"
		};

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
	if (transport & DC_TRANSPORT_USBSTORAGE) {
		const char *dirname = "/Volumes";
		int num_uemis = 0;
		dp = opendir(dirname);
		if (dp == NULL) {
			return -1;
		}

		while ((ep = readdir(dp)) != NULL) {
			if (fnmatch("UEMISSDA", ep->d_name, 0) == 0 ||
			    fnmatch("GARMIN", ep->d_name, 0) == 0) {
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
				num_uemis++;
				break;
			}
		}
		closedir(dp);
		if (num_uemis == 1 && entries == 1) /* if we find exactly one entry and that's a Uemis, select it */
			index = 0;
	}
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
