/* windows.c */
/* implements Windows specific functions */
#include "display-gtk.h"
#include <windows.h>

static HKEY hkey;

static int get_from_registry(HKEY hkey, const char *key)
{
	DWORD value;
	DWORD len = 4;
	LONG success;

	success = RegQueryValueEx(hkey, TEXT(key), NULL, NULL,
				(LPBYTE) &value, &len );
	if (success != ERROR_SUCCESS)
		return FALSE; /* that's what happens the first time we start */
	return value;
}

void subsurface_open_conf(void)
{
	LONG success;

	success = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\subsurface"), 0,
			KEY_QUERY_VALUE, &hkey);
	if (success != ERROR_SUCCESS) {
		success = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\subsurface"),
					0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
					NULL, &hkey, NULL);
		if (success != ERROR_SUCCESS)
			printf("CreateKey Software\\subsurface failed %ld\n", success);
	}
}

void subsurface_set_conf(char *name, pref_type_t type, const void *value)
{
	switch (type) {
	case PREF_BOOL:
		/* we simply store the value as DWORD */
		RegSetValueEx(hkey, TEXT(name), 0, REG_DWORD, value, 4);
		break;
	case PREF_STRING:
		RegSetValueEx(hkey, TEXT(name), 0, REG_SZ, value, strlen(value));
	}
}

const void *subsurface_get_conf(char *name, pref_type_t type)
{
	LONG success;
	char *string;
	int len;

	switch (type) {
	case PREF_BOOL:
		return get_from_registry(hkey, name) ? (void *) 1 : NULL;
	case PREF_STRING:
		string = malloc(80);
		len = 80;
		success = RegQueryValueEx(hkey, TEXT(name), NULL, NULL,
					(LPBYTE) string, (LPDWORD)&len );
		if (success != ERROR_SUCCESS) {
			/* that's what happens the first time we start - just return NULL */
			free(string);
			return NULL;
		}
		return string;
	}
	/* we shouldn't get here */
	return NULL;
}

void subsurface_close_conf(void)
{
	if (RegFlushKey(hkey) != ERROR_SUCCESS)
		printf("RegFlushKey failed \n");
	RegCloseKey(hkey);
}
