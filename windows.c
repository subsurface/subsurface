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
