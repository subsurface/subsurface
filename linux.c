/* linux.c */
/* implements Linux specific functions */
#include "dive.h"
#include "display.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>

const char system_divelist_default_font[] = "Sans 8";

void subsurface_flush_conf(void)
{
	/* this is a no-op */
}

void subsurface_close_conf(void)
{
	/* this is a no-op */
}

const char *subsurface_icon_name()
{
	return "subsurface-icon.svg";
}

const char *system_default_filename(void)
{
	const char *home, *user;
	char *buffer;
	int len;

	home = getenv("HOME");
	user = getenv("LOGNAME");
	len = strlen(home) + strlen(user) + 17;
	buffer = malloc(len);
	snprintf(buffer, len, "%s/subsurface/%s.xml", home, user);
	return buffer;
}

const char *subsurface_gettext_domainpath(char *argv0)
{
	if (argv0[0] == '.') {
		/* we're starting a local copy */
		return "./share/locale";
	} else {
		/* subsurface is installed, so system dir should be fine */
		return NULL;
	}
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
		"ttyUSB*",
		"ttyS*",
		"ttyACM*",
		"rfcomm*",
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
