/* windows.c */
/* implements Windows specific functions */
#include "dive.h"
#include "display.h"
#if USE_GTK_UI
#include "display-gtk.h"
#endif
#include <windows.h>
#include <shlobj.h>

const char system_divelist_default_font[] = "Sans 8";

static HKEY hkey;

void subsurface_open_conf(void)
{
	LONG success;

	success = RegCreateKeyEx(HKEY_CURRENT_USER, (LPCTSTR)TEXT("Software\\subsurface"),
	                         0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
	                         NULL, &hkey, NULL);
	if (success != ERROR_SUCCESS)
		printf("CreateKey Software\\subsurface failed %ld\n", success);
}

void subsurface_unset_conf(const char *name)
{
	RegDeleteValue(hkey, (LPCTSTR)name);
}

void subsurface_set_conf(const char *name, const char *value)
{
	/* since we are using the pointer 'value' as both an actual
	 * pointer to the string setting and as a way to pass the
	 * numbers 0 and 1 to this function for booleans, one of the
	 * calls to RegSetValueEx needs to pass &value (when we want
	 * to pass the boolean value), the other one passes value (the
	 * address of the string. */
	int wlen;
	wchar_t *wname, *wstring;

	wname = (wchar_t *)g_utf8_to_utf16(name, -1, NULL, NULL, NULL);
	if (!wname)
		return;

	wlen = g_utf8_strlen((char *)value, -1);
	wstring = (wchar_t *)g_utf8_to_utf16((char *)value, -1, NULL, NULL, NULL);
	if (!wstring || !wlen) {
		free(wname);
		return;
	}
	RegSetValueExW(hkey, (LPCWSTR)wname, 0, REG_SZ, (const BYTE *)wstring,
	               wlen * sizeof(wchar_t));
	free(wstring);
	free(wname);
}

void subsurface_set_conf_int(const char *name, int value)
{
	RegSetValueEx(hkey, (LPCTSTR)name, 0, REG_DWORD, (const BYTE *)&value, 4);
}

void subsurface_set_conf_bool(const char *name, int value)
{
	subsurface_set_conf_int(name, value);
}

const char *subsurface_get_conf(const char *name)
{
	const int csize = 64;
	int blen = 0;
	LONG ret = ERROR_MORE_DATA;
	wchar_t *wstring = NULL, *wname;
	char *utf8_string;

	wname = (wchar_t *)g_utf8_to_utf16(name, -1, NULL, NULL, NULL);
	if (!wname)
		return NULL;
	blen = 0;
	/* lest try to load the string in chunks of 'csize' bytes until it fits */
	while (ret == ERROR_MORE_DATA) {
		blen += csize;
		wstring = (wchar_t *)realloc(wstring, blen * sizeof(wchar_t));
		ret = RegQueryValueExW(hkey, (LPCWSTR)wname, NULL, NULL,
		                     (LPBYTE)wstring, (LPDWORD)&blen);
	}
	/* that's what happens the first time we start - just return NULL */
	if (ret != ERROR_SUCCESS) {
		free(wname);
		free(wstring);
		return NULL;
	}
	/* convert the returned string into utf-8 */
	utf8_string = g_utf16_to_utf8(wstring, -1, NULL, NULL, NULL);
	free(wstring);
	free(wname);
	if (!utf8_string)
		return NULL;
	if (!g_utf8_validate(utf8_string, -1, NULL)) {
		free(utf8_string);
		return NULL;
	}
	return utf8_string;
}

int subsurface_get_conf_int(const char *name)
{
	DWORD value = -1, len = 4;
	LONG ret = RegQueryValueEx(hkey, (LPCTSTR)TEXT(name), NULL, NULL,
	                         (LPBYTE)&value, (LPDWORD)&len);
	if (ret != ERROR_SUCCESS)
		return -1;
	return value;
}

int subsurface_get_conf_bool(const char *name)
{
	int ret = subsurface_get_conf_int(name);
	if (ret == -1)
		return ret;
	return ret != 0;
}

void subsurface_flush_conf(void)
{
	/* this is a no-op */
}

void subsurface_close_conf(void)
{
	RegCloseKey(hkey);
}

#if USE_GTK_UI
int subsurface_fill_device_list(GtkListStore *store)
{
	const int bufdef = 512;
	const char *dlabels[] = {"UEMISSDA", NULL};
	const char *devdef = "COM1";
	GtkTreeIter iter;
	int index = -1, nentries = 0, ret, i;
	char bufname[bufdef], bufval[bufdef], *p;
	DWORD nvalues, bufval_len, bufname_len;
	HKEY key;

	/* add serial ports */
	ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM",
	                    0, KEY_READ, &key);
	if (ret == ERROR_SUCCESS) {
		ret = RegQueryInfoKeyA(key,  NULL, NULL, NULL, NULL, NULL, NULL, &nvalues,
		                       NULL, NULL, NULL, NULL);
		if (ret == ERROR_SUCCESS)
			for (i = 0; i < nvalues; i++) {
				memset(bufval, 0, bufdef);
				memset(bufname, 0, bufdef);
				bufname_len = bufdef;
				bufval_len = bufdef;
				ret = RegEnumValueA(key, i, bufname, &bufname_len, NULL, NULL, bufval,
				                    &bufval_len);
				if (ret == ERROR_SUCCESS) {
					gtk_list_store_append(store, &iter);
					gtk_list_store_set(store, &iter, 0, bufval, -1);
					if (is_default_dive_computer_device(bufval))
						index = nentries;
					nentries++;
				}
			}
	}
	/* add drive letters that match labels */
	memset(bufname, 0, bufdef);
	bufname_len = bufdef;
	if (GetLogicalDriveStringsA(bufname_len, bufname)) {
		p = bufname;
		while (*p) {
			memset(bufval, 0, bufdef);
			if (GetVolumeInformationA(p, bufval, bufdef, NULL, NULL, NULL, NULL, 0)) {
				for (i = 0; dlabels[i] != NULL; i++)
					if (!strcmp(bufval, dlabels[i])) {
						char name[80];
						snprintf(name, sizeof(name), "%s (%s)", p, dlabels[i]);
						gtk_list_store_append(store, &iter);
						gtk_list_store_set(store, &iter, 0, name, -1);
						if (is_default_dive_computer_device(p))
							index = nentries;
						nentries++;
					}
			}
			p = &p[strlen(p) + 1];
		}
	}
	/* if we can't find anything, use the default */
	if (!nentries) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				0, devdef, -1);
		if (is_default_dive_computer_device(devdef))
			index = 0;
	}
	return index;
}
#endif /* USE_GTK_UI */

const char *subsurface_icon_name()
{
	return "subsurface.ico";
}

const char *system_default_filename(void)
{
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

const char *subsurface_gettext_domainpath(char *argv0)
{
	/* first hackishly make sure that the LANGUAGE information is correctly set up
	 * in the environment */
	char buffer[80];
	gchar *locale = g_win32_getlocale();
	snprintf(buffer, sizeof(buffer), "LANGUAGE=%s.UTF-8", locale);
	putenv(buffer);
	g_free(locale);
	/* always use translation directory relative to install location, regardless of argv0 */
	return "./share/locale";
}

#if USE_GTK_UI
void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager)
{
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
}
#endif /* USE_GTK_UI */

/* barely documented API */
extern int __wgetmainargs(int *, wchar_t ***, wchar_t ***, int, int *);

/* expand-convert the UTF-16 argument list to a list of UTF-8 strings */
void subsurface_command_line_init(gint *argc, gchar ***argv)
{
	wchar_t **wargv, **wenviron, *p, path[MAX_PATH] = {0};
	gchar **argv_new;
	gchar *s;
	/* for si we assume that a struct address will equal the address
	 * of its first and only int member */
	gint i, n, ret, si;

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

gboolean subsurface_launch_for_uri(const char* uri)
{
	gboolean ret = FALSE;
	wchar_t *wuri = (wchar_t *)g_utf8_to_utf16(uri, -1, NULL, NULL, NULL);
	if (wuri) {
		if ((INT_PTR)ShellExecuteW(NULL, L"open", wuri, NULL, NULL, SW_SHOWNORMAL) > 32)
			ret = TRUE;
		free(wuri);
	}
	if (!ret)
		g_message("ShellExecute failed for: %s", uri);
	return ret;
}

/* check if we are running a newer OS version */
gboolean subsurface_os_feature_available(os_feature_t f)
{
	switch (f) {
	case UTF8_FONT_WITH_STARS:
		if ((GetVersion() & 0xff) < 6)
			return FALSE; /* version less than Vista */
		else
			return TRUE;
		break;
	default:
		return TRUE;
	}
}
