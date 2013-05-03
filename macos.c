/* macos.c */
/* implements Mac OS X specific functions */
#include <stdlib.h>
#include "dive.h"
#include "display.h"
#if USE_GTK_UI
#include "display-gtk.h"
#endif /* USE_GTK_UI */
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <mach-o/dyld.h>
#include "gtkosxapplication.h"
#include <sys/syslimits.h>

static GtkosxApplication *osx_app;

/* macos defines CFSTR to create a CFString object from a constant,
 * but no similar macros if a C string variable is supposed to be
 * the argument. We add this here (hardcoding the default allocator
 * and MacRoman encoding */
#define CFSTR_VAR(_var) CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,	\
					(_var), kCFStringEncodingMacRoman,	\
					kCFAllocatorNull)

#define SUBSURFACE_PREFERENCES CFSTR("org.hohndel.subsurface")
#define ICON_NAME "Subsurface.icns"
#define UI_FONT "Arial 12"

const char system_divelist_default_font[] = "Arial 10";

void subsurface_open_conf(void)
{
	/* nothing at this time */
}

void subsurface_unset_conf(const char *name)
{
	CFPreferencesSetAppValue(CFSTR_VAR(name), NULL, SUBSURFACE_PREFERENCES);
}

void subsurface_set_conf(const char *name, const char *value)
{
	CFPreferencesSetAppValue(CFSTR_VAR(name), CFSTR_VAR(value), SUBSURFACE_PREFERENCES);
}

void subsurface_set_conf_bool(const char *name, int value)
{
	CFPreferencesSetAppValue(CFSTR_VAR(name),
		value ? kCFBooleanTrue : kCFBooleanFalse, SUBSURFACE_PREFERENCES);
}

void subsurface_set_conf_int(const char *name, int value)
{
	CFNumberRef numRef = CFNumberCreate(NULL, kCFNumberIntType, &value);
	CFPreferencesSetAppValue(CFSTR_VAR(name), numRef, SUBSURFACE_PREFERENCES);
}

const char *subsurface_get_conf(const char *name)
{
	CFPropertyListRef strpref;

	strpref = CFPreferencesCopyAppValue(CFSTR_VAR(name), SUBSURFACE_PREFERENCES);
	if (!strpref)
		return NULL;
	return strdup(CFStringGetCStringPtr(strpref, kCFStringEncodingMacRoman));
}

int subsurface_get_conf_bool(const char *name)
{
	Boolean boolpref, exists;

	boolpref = CFPreferencesGetAppBooleanValue(CFSTR_VAR(name), SUBSURFACE_PREFERENCES, &exists);
	if (!exists)
		return -1;
	return boolpref;
}

int subsurface_get_conf_int(const char *name)
{
	Boolean exists;
	CFIndex value;

	value = CFPreferencesGetAppIntegerValue(CFSTR_VAR(name), SUBSURFACE_PREFERENCES, &exists);
	if (!exists)
		return -1;
	return value;
}

void subsurface_flush_conf(void)
{
	int ok = CFPreferencesAppSynchronize(SUBSURFACE_PREFERENCES);
	if (!ok)
		fprintf(stderr,"Could not save preferences\n");
}

void subsurface_close_conf(void)
{
	/* Nothing */
}

int subsurface_fill_device_list(GtkListStore *store)
{
	int i = 0;
	int index = -1;
	GtkTreeIter iter;
	GDir *dev;
	const char *name;

	dev = g_dir_open("/dev", 0, NULL);
	while (dev && (name = g_dir_read_name(dev)) != NULL) {
		if (strstr(name, "usbserial") ||
		    (strstr(name, "SerialPort") && strstr(name, "cu"))) {
			int len = strlen(name) + 6;
			char *devicename = malloc(len);
			snprintf(devicename, len, "/dev/%s", name);
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
					0, devicename, -1);
			if (is_default_dive_computer_device(devicename))
				index = i;
			i++;
		}
	}
	if (dev)
		g_dir_close(dev);
	dev = g_dir_open("/Volumes", 0, NULL);
	while (dev && (name = g_dir_read_name(dev)) != NULL) {
		if (strstr(name, "UEMISSDA")) {
			int len = strlen(name) + 10;
			char *devicename = malloc(len);
			snprintf(devicename, len, "/Volumes/%s", name);
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
					0, devicename, -1);
			if (is_default_dive_computer_device(devicename))
				index = i;
			i++;
		}
	}
	if (dev)
		g_dir_close(dev);
	if (i == 0) {
		/* if we can't find anything, use the default */
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				0, "/dev/tty.SLAB_USBtoUART", -1);
		if (is_default_dive_computer_device("/dev/tty.SLAB_USBtoUART"))
			index = i;
	}
	return index;
}

const char *subsurface_icon_name()
{
	static char path[PATH_MAX];

#if USE_GTK_UI
	snprintf(path, sizeof(path), "%s/%s", gtkosx_application_get_resource_path(), ICON_NAME);
#else
	/* need Qt path */
#endif
	return path;
}

const char *system_default_filename(void)
{
	const char *home, *user;
	char *buffer;
	int len;

	home = g_get_home_dir();
	user = g_get_user_name();
	len = strlen(home) + strlen(user) + 45;
	buffer = malloc(len);
	snprintf(buffer, len, "%s/Library/Application Support/Subsurface/%s.xml", home, user);
	return buffer;
}

const char *subsurface_gettext_domainpath(char *argv0)
{
	/* on a Mac we ignore the argv0 argument and instead use the resource_path
	 * to figure out where to find the translation files */
#if USE_GTK_UI
	static char buffer[PATH_MAX];
	const char *resource_path = gtkosx_application_get_resource_path();
	if (resource_path) {
		snprintf(buffer, sizeof(buffer), "%s/share/locale", resource_path);
		return buffer;
	}
#endif /* USE_GTK_UI */
	return "./share/locale";
}

#if USE_GTK_UI
static void show_main_window(GtkWidget *w, gpointer data)
{
	gtk_widget_show(main_window);
	gtk_window_present(GTK_WINDOW(main_window));
}

void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager)
{
	GtkWidget *menu_item, *sep;
	static char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s/xslt", gtkosx_application_get_resource_path());
	setenv("SUBSURFACE_XSLT_PATH",  path, TRUE);

	g_object_set(G_OBJECT(settings), "gtk-font-name", UI_FONT, NULL);

	osx_app = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);
	gtk_widget_hide (menubar);
	gtkosx_application_set_menu_bar(osx_app, GTK_MENU_SHELL(menubar));

	sep = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/FileMenu/Separator3");
	if (sep)
		gtk_widget_destroy(sep);

	menu_item = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/FileMenu/Quit");
	gtk_widget_hide (menu_item);
	menu_item = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/Help/About");
	gtkosx_application_insert_app_menu_item(osx_app, menu_item, 0);

	sep = gtk_separator_menu_item_new();
	g_object_ref(sep);
	gtkosx_application_insert_app_menu_item (osx_app, sep, 1);

	menu_item = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/FileMenu/Preferences");
	gtkosx_application_insert_app_menu_item(osx_app, menu_item, 2);

	sep = gtk_separator_menu_item_new();
	g_object_ref(sep);
	gtkosx_application_insert_app_menu_item (osx_app, sep, 3);

	gtkosx_application_set_use_quartz_accelerators(osx_app, TRUE);
	g_signal_connect(osx_app,"NSApplicationDidBecomeActive",G_CALLBACK(show_main_window),NULL);
	g_signal_connect(osx_app, "NSApplicationBlockTermination", G_CALLBACK(on_delete), NULL);

	gtkosx_application_ready(osx_app);
}
#endif /* UES_GTK_UI */

void subsurface_command_line_init(gint *argc, gchar ***argv)
{
	/* this is a no-op */
}

void subsurface_command_line_exit(gint *argc, gchar ***argv)
{
	/* this is a no-op */
}

gboolean subsurface_os_feature_available(os_feature_t f)
{
	return TRUE;
}

gboolean subsurface_launch_for_uri(const char* uri)
{
	CFURLRef urlref = CFURLCreateWithBytes(NULL, uri, strlen(uri), kCFStringEncodingMacRoman, NULL);
	OSStatus status = LSOpenCFURLRef(urlref, NULL);
	if (status)
		return FALSE;
	return TRUE;
}
