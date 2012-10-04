/* windows.c */
/* implements Windows specific functions */
#include "dive.h"
#include "display-gtk.h"
#include <windows.h>
#include <shlobj.h>
#define DIVELIST_DEFAULT_FONT "Sans 8"

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

void subsurface_flush_conf(void)
{
	/* this is a no-op */
}

void subsurface_close_conf(void)
{
	RegCloseKey(hkey);
}

const char *subsurface_USB_name()
{
	return "COM3";
}

const char *subsurface_icon_name()
{
	return "subsurface.ico";
}

const char *subsurface_default_filename()
{
	if (default_filename) {
		return strdup(default_filename);
	} else {
		char datapath[MAX_PATH];
		const char *user;
		char *buffer;
		int len;

		user = g_get_user_name();
		if (! SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, datapath))) {
			datapath[0] = '.';
			datapath[1] = '\0';
		}
		len = strlen(datapath) + strlen(user) + 17;
		buffer = malloc(len);
		snprintf(buffer, len, "%s\\Subsurface\\%s.xml", datapath, user);
		return buffer;
	}
}

void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager)
{
	if (!divelist_font)
		divelist_font = DIVELIST_DEFAULT_FONT;
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
}

/* barely documented API */
extern int __wgetmainargs(int *, wchar_t ***, wchar_t ***, int, int *);

/* expand-convert the UTF-16 argument list to a list of UTF-8 strings */
void subsurface_command_line_init(gint *argc, gchar ***argv)
{
	wchar_t **wargv, **wenviron;
	gchar **argv_new;
	gchar *s;
	/* for si we assume that a struct address will equal the address
	 * of its first and only int member */
	gint i, n, ret, si;

	/* memory leak tools may reports a potential issue here at a call
	 * to strcpy_s in msvcrt, wich should be a false positive. but even if there
	 * is some kind of a leak, it should be unique and have the same
	 * lifespan as the process heap. */
	ret = __wgetmainargs(&n, &wargv, &wenviron, TRUE, &si);
	if (ret < 0) {
		g_warning("Cannot convert command line");
		return;
	}
	argv_new = g_malloc(sizeof(gchar *) * (n + 1));

	for (i = 0; i < n; ++i) {
		s = g_utf16_to_utf8((gunichar2 *)wargv[i], -1, NULL, NULL, NULL);
		if (!s) {
			g_warning("Cannot convert command line argument (%d) to UTF-8", (i + 1));
			s = "\0";
		}	else if (!g_utf8_validate(s, -1, NULL)) {
			g_warning("Cannot validate command line argument '%s' (%d)", s, (i + 1));
			g_free(s);
			s = "\0";
		}
		argv_new[i] = s;
	}
	argv_new[n] = NULL;

	/* update the argument list and count */
	if (argv && argc) {
		*argv = argv_new;
		*argc = n;
	}
}

/* once done, free the argument list */
void subsurface_command_line_exit(gint *argc, gchar ***argv)
{
	int i;
	for (i = 0; i < *argc; i++)
		g_free((*argv)[i]);
	g_free(*argv);
}
