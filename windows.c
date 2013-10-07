/* windows.c */
/* implements Windows specific functions */
#include "dive.h"
#include "display.h"
#include <windows.h>
#include <shlobj.h>

const char system_divelist_default_font[] = "Sans 8";

const char *system_default_filename(void)
{
	char datapath[MAX_PATH];
	const char *user;
	char *buffer;
	int len;

	/* I don't think this works on Windows */
	user = getenv("LOGNAME");
	if (! SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, datapath))) {
		datapath[0] = '.';
		datapath[1] = '\0';
	}
	len = strlen(datapath) + strlen(user) + 17;
	buffer = malloc(len);
	snprintf(buffer, len, "%s\\Subsurface\\%s.xml", datapath, user);
	return buffer;
}

/* barely documented API */
extern int __wgetmainargs(int *, wchar_t ***, wchar_t ***, int, int *);

/* expand-convert the UTF-16 argument list to a list of UTF-8 strings */
void subsurface_command_line_init(int *argc, char ***argv)
{
	wchar_t **wargv, **wenviron, *p, path[MAX_PATH] = {0};
	char **argv_new;
	char *s;
	/* for si we assume that a struct address will equal the address
	 * of its first and only int member */
	int i, n, ret, si;

	/* change the current process path to the module path, so that we can
	 * access relative folders such as ./share and ./xslt */
	GetModuleFileNameW(NULL, path, MAX_PATH - 1);
	p = wcsrchr(path, '\\');
	*(p + 1) = '\0';
	SetCurrentDirectoryW(path);

	/* memory leak tools may reports a potential issue here at a call
	 * to strcpy_s in msvcrt, wich should be a false positive. but even if there
	 * is some kind of a leak, it should be unique and have the same
	 * lifespan as the process heap. */
	ret = __wgetmainargs(&n, &wargv, &wenviron, TRUE, &si);
	if (ret < 0) {
		fprintf(stderr, "Cannot convert command line");
		return;
	}
	argv_new = malloc(sizeof(char *) * (n + 1));

#if MISSING_GLIB_FUNCTIONS
	for (i = 0; i < n; ++i) {
		s = g_utf16_to_utf8((gunichar2 *)wargv[i], -1, NULL, NULL, NULL);
		if (!s) {
			fprintf(stderr, "Cannot convert command line argument (%d) to UTF-8", (i + 1));
			s = "\0";
		} else if (!g_utf8_validate(s, -1, NULL)) {
			fprintf(stderr,"Cannot validate command line argument '%s' (%d)", s, (i + 1));
			free(s);
			s = "\0";
		}
		argv_new[i] = s;
	}
#endif
	argv_new[n] = NULL;

	/* update the argument list and count */
	if (argv && argc) {
		*argv = argv_new;
		*argc = n;
	}
}

/* once done, free the argument list */
void subsurface_command_line_exit(int *argc, char ***argv)
{
	int i;
	for (i = 0; i < *argc; i++)
		free((*argv)[i]);
	free(*argv);
}

int enumerate_devices (device_callback_t callback, void *userdata)
{
	// Open the registry key.
	HKEY hKey;
	int index = -1;
	LONG rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey);
	if (rc != ERROR_SUCCESS) {
		return -1;
	}

	// Get the number of values.
	DWORD count = 0;
	rc = RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL);
	if (rc != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return -1;
	}
	DWORD i;
	for (i = 0; i < count; ++i) {
		// Get the value name, data and type.
		char name[512], data[512];
		DWORD name_len = sizeof (name);
		DWORD data_len = sizeof (data);
		DWORD type = 0;
		rc = RegEnumValue (hKey, i, name, &name_len, NULL, &type, (LPBYTE) data, &data_len);
		if (rc != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return -1;
		}

		// Ignore non-string values.
		if (type != REG_SZ)
			continue;

		// Prevent a possible buffer overflow.
		if (data_len >= sizeof (data)) {
			RegCloseKey(hKey);
			return -1;
		}

		// Null terminate the string.
		data[data_len] = 0;

		callback (data, userdata);
		index++;
		if (is_default_dive_computer_device(name))
			index = i;
	}

	RegCloseKey(hKey);
	return index;
}
