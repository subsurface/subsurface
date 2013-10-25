/* linux.c */
/* implements Linux specific functions */
#include "dive.h"
#include "display.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <fnmatch.h>

const char system_divelist_default_font[] = "Sans 8";

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
	FILE *file;
	char *line = NULL;
	char *fname;
	size_t len;

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
	closedir (dp);

	file = fopen("/proc/mounts", "r");
	if (file == NULL)
		return index;

	while ((getline(&line, &len, file)) != -1) {
		char *ptr = strstr(line, "UEMISSDA");
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
				index = i;
			i++;
			free((void *)fname);
	   }
	}

	free(line);
	fclose(file);

	return index;
}
