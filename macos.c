/* macos.c */
/* implements Mac OS X specific functions */
#include "display-gtk.h"
#include <CoreFoundation/CoreFoundation.h>

static CFURLRef fileURL;
static CFPropertyListRef propertyList;
static CFMutableDictionaryRef dict = NULL;

/* macos defines CFSTR to create a CFString object from a constant,
 * but no similar macros if a C string variable is supposed to be
 * the argument. We add this here (hardcoding the default allocator
 * and MacRoman encoding */
#define CFSTR_VAR(_var) CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,	\
					(_var), kCFStringEncodingMacRoman,	\
					kCFAllocatorNull)

void subsurface_open_conf(void)
{
	CFStringRef errorString;
	CFDataRef resourceData;
	Boolean status;
	SInt32 errorCode;

	fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
						CFSTR("subsurface.pref"),// file path name
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
	// some error handling - but really, what can we do?
	CFRelease(xmlData);
	CFRelease(propertyList);
}

const char *subsurface_USB_name()
{
	return("/dev/tty.SLAB_USBtoUART");
}
