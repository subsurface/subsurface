/* macos.c */
/* implements Mac OS X specific functions */
#include "display-gtk.h"
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>
#include "gtkosxapplication.h"

static GtkOSXApplication *osx_app;

/* macos defines CFSTR to create a CFString object from a constant,
 * but no similar macros if a C string variable is supposed to be
 * the argument. We add this here (hardcoding the default allocator
 * and MacRoman encoding */
#define CFSTR_VAR(_var) CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,	\
					(_var), kCFStringEncodingMacRoman,	\
					kCFAllocatorNull)

#define SUBSURFACE_PREFERENCES CFSTR("org.hohndel.subsurface")
#define ICON_NAME "Subsurface.icns"
#define UI_FONT "Arial Unicode MS 12"
#define DIVELIST_MAC_DEFAULT_FONT "Arial Unicode MS 9"

void subsurface_open_conf(void)
{
	/* nothing at this time */
}

void subsurface_set_conf(char *name, pref_type_t type, const void *value)
{
	switch (type) {
	case PREF_BOOL:
		CFPreferencesSetAppValue(CFSTR_VAR(name),
			value == NULL ? kCFBooleanFalse : kCFBooleanTrue, SUBSURFACE_PREFERENCES);
		break;
	case PREF_STRING:
		CFPreferencesSetAppValue(CFSTR_VAR(name), CFSTR_VAR(value), SUBSURFACE_PREFERENCES);
	}
}

const void *subsurface_get_conf(char *name, pref_type_t type)
{
	Boolean boolpref;
	CFPropertyListRef strpref;

	switch (type) {
	case PREF_BOOL:
		boolpref = CFPreferencesGetAppBooleanValue(CFSTR_VAR(name), SUBSURFACE_PREFERENCES, FALSE);
		if (boolpref)
			return (void *) 1;
		else
			return NULL;
	case PREF_STRING:
		strpref = CFPreferencesCopyAppValue(CFSTR_VAR(name), SUBSURFACE_PREFERENCES);
		if (!strpref)
			return NULL;
		return CFStringGetCStringPtr(strpref, kCFStringEncodingMacRoman);
	}
	/* we shouldn't get here, but having this line makes the compiler happy */
	return NULL;
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

const char *subsurface_USB_name()
{
	return "/dev/tty.SLAB_USBtoUART";
}

const char *subsurface_icon_name()
{
	static char path[1024];

	snprintf(path, 1024, "%s/%s", quartz_application_get_resource_path(), ICON_NAME);

	return path;
}

void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager)
{
	GtkWidget *menu_item, *sep;

	if (!divelist_font)
		divelist_font = DIVELIST_MAC_DEFAULT_FONT;
	g_object_set(G_OBJECT(settings), "gtk-font-name", UI_FONT, NULL);

	osx_app = g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
	gtk_widget_hide (menubar);
	gtk_osxapplication_set_menu_bar(osx_app, GTK_MENU_SHELL(menubar));

	sep = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/FileMenu/Separator2");
	if (sep)
		gtk_widget_destroy(sep);

	menu_item = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/FileMenu/Quit");
	gtk_widget_hide (menu_item);
	menu_item = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/Help/About");
	gtk_osxapplication_insert_app_menu_item(osx_app, menu_item, 0);

	sep = gtk_separator_menu_item_new();
	g_object_ref(sep);
	gtk_osxapplication_insert_app_menu_item (osx_app, sep, 1);

	menu_item = gtk_ui_manager_get_widget(ui_manager, "/MainMenu/FileMenu/Preferences");
	gtk_osxapplication_insert_app_menu_item(osx_app, menu_item, 2);

	sep = gtk_separator_menu_item_new();
	g_object_ref(sep);
	gtk_osxapplication_insert_app_menu_item (osx_app, sep, 3);

	gtk_osxapplication_set_use_quartz_accelerators(osx_app, TRUE);
	gtk_osxapplication_ready(osx_app);
}
