/* linux.c */
/* implements Linux specific functions */
#include "dive.h"
#include "display.h"
#if USE_GTK_UI
#include "display-gtk.h"
#endif
#include <gconf/gconf-client.h>
#include <string.h>

const char system_divelist_default_font[] = "Sans 8";

GConfClient *gconf;

static char *gconf_name(const char *name)
{
	static char buf[255] = "/apps/subsurface/";

	snprintf(buf, 255, "/apps/subsurface/%s", name);

	return buf;
}

void subsurface_open_conf(void)
{
	gconf = gconf_client_get_default();
}

void subsurface_unset_conf(const char *name)
{
	gconf_client_unset(gconf, gconf_name(name), NULL);
}

void subsurface_set_conf(const char *name, const char *value)
{
	gconf_client_set_string(gconf, gconf_name(name), value, NULL);
}

void subsurface_set_conf_bool(const char *name, int value)
{
	gconf_client_set_bool(gconf, gconf_name(name), value > 0, NULL);
}

void subsurface_set_conf_int(const char *name, int value)
{
	gconf_client_set_int(gconf, gconf_name(name), value , NULL);
}

const char *subsurface_get_conf(const char *name)
{
	return gconf_client_get_string(gconf, gconf_name(name), NULL);
}

int subsurface_get_conf_bool(const char *name)
{
	GConfValue *val;
	gboolean ret;

	val = gconf_client_get(gconf, gconf_name(name), NULL);
	if (!val)
		return -1;
	ret = gconf_value_get_bool(val);
	gconf_value_free(val);
	return ret;
}

int subsurface_get_conf_int(const char *name)
{
	int val = gconf_client_get_int(gconf, gconf_name(name), NULL);
	if(!val)
		return -1;

	return val;
}

void subsurface_flush_conf(void)
{
	/* this is a no-op */
}

void subsurface_close_conf(void)
{
	/* this is a no-op */
}

#if USE_GTK_UI
int subsurface_fill_device_list(GtkListStore *store)
{
	int i = 0;
	int index = -1;
	GtkTreeIter iter;
	GDir *dev;
	const char *name;
	char *buffer;
	gsize length;

	dev = g_dir_open("/dev", 0, NULL);
	while (dev && (name = g_dir_read_name(dev)) != NULL) {
		if (strstr(name, "USB")) {
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
	if (g_file_get_contents("/proc/mounts", &buffer, &length, NULL) &&
		length > 0) {
		char *ptr = strstr(buffer, "UEMISSDA");
		if (ptr) {
			char *end = ptr, *start = ptr;
			while (start > buffer && *start != ' ')
				start--;
			if (*start == ' ')
				start++;
			while (*end != ' ' && *end != '\0')
				end++;
			*end = '\0';
			name = strdup(start);
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
					0, name, -1);
			if (is_default_dive_computer_device(name))
				index = i;
			i++;
			free((void *)name);
		}
		g_free(buffer);
	}
	if (i == 0) {
		/* if we can't find anything, use the default */
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				0, "/dev/ttyUSB0", -1);
		if (is_default_dive_computer_device("/dev/ttyUSB0"))
			index = i;
	}
	return index;
}
#endif /* USE_GTK_UI */

const char *subsurface_icon_name()
{
	return "subsurface-icon.svg";
}

const char *system_default_filename(void)
{
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

#if USE_GTK_UI
void subsurface_ui_setup(GtkSettings *settings, GtkWidget *menubar,
		GtkWidget *vbox, GtkUIManager *ui_manager)
{
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
}
#endif /* USE_GTK_UI */

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

#if USE_GTK_UI
gboolean subsurface_launch_for_uri(const char* uri)
{
	GError *err = NULL;
	gtk_show_uri(NULL, uri, gtk_get_current_event_time(), &err);
	if (err) {
		g_message("%s: %s", err->message, uri);
		g_error_free(err);
		return FALSE;
	}
	return TRUE;
}
#endif /* USE_GTK_UI */
