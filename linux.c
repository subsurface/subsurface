/* linux.c */
/* implements Linux specific functions */
#include "dive.h"
#include "display-gtk.h"
#include <gconf/gconf-client.h>
#include <string.h>

#define DIVELIST_DEFAULT_FONT "Sans 8"

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

void subsurface_flush_conf(void)
{
	/* this is a no-op */
}

void subsurface_close_conf(void)
{
	/* this is a no-op */
}

const char *subsurface_USB_name()
{
	return "/dev/ttyUSB0";
}

const char *subsurface_icon_name()
{
	return "subsurface.svg";
}

const char *subsurface_default_filename()
{
	if (default_filename) {
		return strdup(default_filename);
	} else {
		const char *home, *user;
		char *buffer;
		int len;

		home = g_get_home_dir();
		user = g_get_user_name();
		len = strlen(home) + strlen(user) + 17;
		buffer = malloc(len);
		snprintf(buffer, len, "%s/subsurface/%s.xml", home, user);
		return buffer;
	}
}

const char *subsurface_gettext_domainpath(char *argv0)
{
	if (argv0[0] == '.') {
		/* we're starting a local copy */
		return "./share/locale";
	} else {
		/* subsurface is installed, so system dir should be fine */
		return NULL;
	}
}

void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager)
{
	if (!divelist_font)
		divelist_font = strdup(DIVELIST_DEFAULT_FONT);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
}

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
