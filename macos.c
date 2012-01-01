/* macos.c */
/* implements Mac OS X specific functions */
#include "display-gtk.h"
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>
#include "gtkosxapplication.h"


static CFURLRef fileURL;
static CFPropertyListRef propertyList;
static CFMutableDictionaryRef dict = NULL;
static GtkOSXApplication *theApp;

/* macos defines CFSTR to create a CFString object from a constant,
 * but no similar macros if a C string variable is supposed to be
 * the argument. We add this here (hardcoding the default allocator
 * and MacRoman encoding */
#define CFSTR_VAR(_var) CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,	\
					(_var), kCFStringEncodingMacRoman,	\
					kCFAllocatorNull)

#define SUBSURFACE_PREFERENCES "~/Library/Preferences/org.hohndel.subsurface.plist"
#define REL_ICON_PATH "Resources/Subsurface.icns"
#define UI_FONT "Arial Unicode MS 12"
#define DIVELIST_MAC_DEFAULT_FONT "Arial Unicode MS 9"

static void show_error(SInt32 errorCode, char *action)
{
	char *errortext;

	switch(errorCode) {
	case -11: errortext = "kCFURLUnknownSchemeError";
		break;
	case -12: errortext = "kCFURLResourceNotFoundError";
		break;
	case -13: errortext = "kCFURLResourceAccessViolationError";
		break;
	case -14: errortext = "kCFURLRemoteHostUnavailableError";
		break;
	case -15: errortext = "kCFURLImproperArgumentsError";
		break;
	case -16: errortext = "kCFURLUnknownPropertyKeyError";
		break;
	case -17: errortext = "kCFURLPropertyKeyUnavailableError";
		break;
	case -18: errortext = "kCFURLTimeoutError";
		break;
	default: errortext = "kCFURLUnknownError";
	};
	fprintf(stderr, "Error %s preferences: %s\n", action, errortext);
}

void subsurface_open_conf(void)
{
	CFStringRef errorString;
	CFDataRef resourceData;
	Boolean status;
	SInt32 errorCode;

	fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
						CFSTR(SUBSURFACE_PREFERENCES),// file path name
						kCFURLPOSIXPathStyle,    // interpret as POSIX path
						false );                 // is it a directory?

	status = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault,
							fileURL, &resourceData,
							NULL, NULL, &errorCode);
	if (status) {
		propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault,
							resourceData, kCFPropertyListImmutable,
							&errorString);
		CFRelease(resourceData);
	} else {
		show_error(errorCode, "reading");
	}
}

void subsurface_set_conf(char *name, pref_type_t type, const void *value)
{
	if (!dict)
		dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
						&kCFTypeDictionaryKeyCallBacks,
						&kCFTypeDictionaryValueCallBacks);
	switch (type) {
	case PREF_BOOL:
		CFDictionarySetValue(dict, CFSTR_VAR(name), value == NULL ? CFSTR("0") : CFSTR("1"));
		break;
	case PREF_STRING:
		CFDictionarySetValue(dict, CFSTR_VAR(name), CFSTR_VAR(value));
	}
}
const void *subsurface_get_conf(char *name, pref_type_t type)
{
	CFStringRef dict_entry;

	/* if no settings exist, we return the value for FALSE */
	if (!propertyList)
		return NULL;

	switch (type) {
	case PREF_BOOL:
		dict_entry = CFDictionaryGetValue(propertyList, CFSTR_VAR(name));
		if (dict_entry && ! CFStringCompare(CFSTR("1"), dict_entry, 0))
			return (void *) 1;
		else
			return NULL;
	case PREF_STRING:
		return CFStringGetCStringPtr(CFDictionaryGetValue(propertyList,
						CFSTR_VAR(name)), kCFStringEncodingMacRoman);
	}
	/* we shouldn't get here, but having this line makes the compiler happy */
	return NULL;
}

void subsurface_close_conf(void)
{
	Boolean status;
	SInt32 errorCode;
	CFDataRef xmlData;

	propertyList = dict;
	dict = NULL;
	xmlData = CFPropertyListCreateXMLData(kCFAllocatorDefault, propertyList);
	status = CFURLWriteDataAndPropertiesToResource (fileURL, xmlData, NULL, &errorCode);
	if (!status) {
		show_error(errorCode, "writing");
	}
	CFRelease(xmlData);
	CFRelease(propertyList);
}

const char *subsurface_USB_name()
{
	return "/dev/tty.SLAB_USBtoUART";
}

const char *subsurface_icon_name()
{
	static char path[1024];
	char *ptr1, *ptr2;
	uint32_t size = sizeof(path); /* need extra space to copy icon path */
	if (_NSGetExecutablePath(path, &size) == 0) {
		ptr1 = strcasestr(path,"MacOS/subsurface");
		ptr2 = strcasestr(path,"Contents");
		if (ptr1 && ptr2) {
			/* we are running as installed app from a bundle */
			if (ptr1 - path < size - strlen(REL_ICON_PATH)) {
				strcpy(ptr1,REL_ICON_PATH);
				return path;
			}
		}
	}
	return "packaging/macosx/Subsurface.icns";
}

void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox)
{
	if (!divelist_font)
		divelist_font = DIVELIST_MAC_DEFAULT_FONT;
	g_object_set(G_OBJECT(settings), "gtk-font-name", UI_FONT, NULL);

	theApp = g_object_new(GTK_TYPE_OSX_APPLICATION, NULL);
	gtk_widget_hide (menubar);
	gtk_osxapplication_set_menu_bar(theApp, GTK_MENU_SHELL(menubar));
	gtk_osxapplication_set_use_quartz_accelerators(theApp, TRUE);
	gtk_osxapplication_ready(theApp);

}
