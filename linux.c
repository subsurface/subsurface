/* linux.c */
/* implements Linux specific functions */
#include "display-gtk.h"
#include <gconf/gconf-client.h>

GConfClient *gconf;

static char *gconf_name(char *name)
{
	static char buf[255] = "/apps/subsurface/";

	snprintf(buf, 255, "/apps/subsurface/%s", name);

	return buf;
}

void subsurface_open_conf(void)
{
	gconf = gconf_client_get_default();
}

void subsurface_set_conf(char *name, pref_type_t type, const void *value)
{
	switch (type) {
	case PREF_BOOL:
		gconf_client_set_bool(gconf, gconf_name(name), value != NULL, NULL);
		break;
	case PREF_STRING:
		gconf_client_set_string(gconf, gconf_name(name), value, NULL);
	}
}

const void *subsurface_get_conf(char *name, pref_type_t type)
{
	switch (type) {
	case PREF_BOOL:
		return gconf_client_get_bool(gconf, gconf_name(name), NULL) ? (void *) 1 : NULL;
	case PREF_STRING:
		return gconf_client_get_string(gconf, gconf_name(name), NULL);
	}
	/* we shouldn't get here */
	return NULL;
}

void subsurface_close_conf(void)
{
	/* this is a no-op */
}

const char *subsurface_USB_name()
{
	return("/dev/ttyUSB0");
}
