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

	success = RegQueryValueEx(hkey, (LPCTSTR)TEXT(key), NULL, NULL,
				(LPBYTE) &value, (LPDWORD)&len );
	if (success != ERROR_SUCCESS)
		return FALSE; /* that's what happens the first time we start */
	return value;
}

void subsurface_open_conf(void)
{
	LONG success;

	success = RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)TEXT("Software\\subsurface"),
				0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
				NULL, &hkey, NULL);
	if (success != ERROR_SUCCESS)
		printf("CreateKey Software\\subsurface failed %ld\n", success);
}

void subsurface_set_conf(char *name, pref_type_t type, const void *value)
{
	/* since we are using the pointer 'value' as both an actual
	 * pointer to the string setting and as a way to pass the
	 * numbers 0 and 1 to this function for booleans, one of the
	 * calls to RegSetValueEx needs to pass &value (when we want
	 * to pass the boolean value), the other one passes value (the
	 * address of the string. */
	switch (type) {
	case PREF_BOOL:
		/* we simply store the value as DWORD */
		RegSetValueEx(hkey, (LPCTSTR)TEXT(name), 0, REG_DWORD, (const BYTE *)&value, 4);
		break;
	case PREF_STRING:
		RegSetValueEx(hkey, (LPCTSTR)TEXT(name), 0, REG_SZ, (const BYTE *)value, strlen(value));
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
		success = RegQueryValueEx(hkey, (LPCTSTR)TEXT(name), NULL, NULL,
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

const char *subsurface_USB_name()
{
	return("COM3");
}
