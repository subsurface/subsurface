/* gps.c */
/* Creates the UI displaying the dives locations on a map.
 */
#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "osm-gps-map.h"

#include "dive.h"
#include "display.h"
#include "display-gtk.h"
#include "divelist.h"

/* Several map providers are available, such as OSM_GPS_MAP_SOURCE_OPENSTREETMAP
   and OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_SATELLITE. We should make more of
   them available from e.g. a pull-down menu */
static OsmGpsMapSource_t opt_map_provider = OSM_GPS_MAP_SOURCE_GOOGLE_STREET;


static void on_close(GtkWidget *widget, gpointer user_data)
{
	GtkWidget **window = user_data;
	gtk_widget_destroy(widget);
	*window = NULL;
}

/* the osm gps map default is to scroll-and-recenter around the mouse position
 * that is BAT SHIT CRAZY.
 * So let's do our own scroll handling instead */
static gboolean scroll_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	OsmGpsMap *map = (OsmGpsMap *)widget;
	if (event->direction == GDK_SCROLL_UP)
		osm_gps_map_zoom_in(map);
	else if (event->direction == GDK_SCROLL_DOWN)
		osm_gps_map_zoom_out(map);
	/* don't allow the insane default handler to get its hands on this event */
	return TRUE;
}

static void add_gps_point(OsmGpsMap *map, float latitude, float longitude)
{
	OsmGpsMapTrack * track = osm_gps_map_track_new();
	OsmGpsMapPoint * point = osm_gps_map_point_new_degrees(latitude, longitude);
	osm_gps_map_track_add_point(track, point);
	osm_gps_map_track_add(map, track);
}


OsmGpsMap *init_map(void)
{
	OsmGpsMap *map;
	OsmGpsMapLayer *osd;
	char *cachedir, *cachebasedir;

	cachebasedir = osm_gps_map_get_default_cache_directory();
	cachedir = g_strdup(OSM_GPS_MAP_CACHE_AUTO);

	map = g_object_new(OSM_TYPE_GPS_MAP,
				"map-source", opt_map_provider,
				"tile-cache", cachedir,
				"tile-cache-base", cachebasedir,
				"proxy-uri", g_getenv("http_proxy"),
				NULL);
	osd = g_object_new(OSM_TYPE_GPS_MAP_OSD,
				"show-scale", TRUE,
				"show-coordinates", TRUE,
				"show-crosshair", TRUE,
				"show-dpad", TRUE,
				"show-zoom", TRUE,
				"show-gps-in-dpad", TRUE,
				"show-gps-in-zoom", FALSE,
				"dpad-radius", 30,
				NULL);

	osm_gps_map_layer_add(OSM_GPS_MAP(map), osd);
	g_object_unref(G_OBJECT(osd));
	return map;
}

void show_map(OsmGpsMap *map, GtkWidget **window)
{
	if (!*window) {
		/* Enable keyboard navigation */
		osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_FULLSCREEN, GDK_F11);
		osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_UP, GDK_Up);
		osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_DOWN, GDK_Down);
		osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_LEFT, GDK_Left);
		osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_RIGHT, GDK_Right);

		*window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_position(GTK_WINDOW(*window), GTK_WIN_POS_MOUSE);
		gtk_window_set_default_size(GTK_WINDOW(*window), 640, 480);
		gtk_window_set_title(GTK_WINDOW(*window), _("Dives locations"));
		gtk_container_set_border_width(GTK_CONTAINER(*window), 5);
		GTK_WINDOW(*window)->allow_shrink = TRUE;
		gtk_container_add(GTK_CONTAINER(*window), GTK_WIDGET(map));
		g_signal_connect(*window, "destroy", G_CALLBACK(on_close), (gpointer)window);
		g_signal_connect(G_OBJECT(map), "scroll-event", G_CALLBACK(scroll_cb), NULL);
	}
	gtk_widget_show_all(*window);
	gtk_window_present(GTK_WINDOW(*window));
}

void show_gps_location(struct dive *dp)
{
	static GtkWidget *window = NULL;
	static OsmGpsMap *map = NULL;
	GdkPixbuf *picture;
	GError *gerror = NULL;

	double lat = dp->latitude.udeg / 1000000.0;
	double lng = dp->longitude.udeg / 1000000.0;

	if (!map || !window)
		map = init_map();
	if (lat != 0 || lng != 0) {
		add_gps_point(map, lat, lng);
		osm_gps_map_set_center_and_zoom(map, lat, lng, 8);
		picture = gdk_pixbuf_new_from_file("./flag.png", &gerror);
		if (picture) {
			osm_gps_map_image_add_with_alignment(map, lat, lng, picture, 0, 1);
		} else {
			printf("error message: %s\n", gerror->message);
		}
	} else {
		osm_gps_map_set_center_and_zoom(map, 0, 0, 2);
	}
	show_map(map, &window);
}

void show_gps_locations()
{
	static OsmGpsMap *map = NULL;
	static GtkWidget *window = NULL;
	struct dive *dp;
	int idx;

	if (!window || !map)
		map = init_map();

	for (idx = 0; idx < dive_table.nr; idx++) {
		dp = dive_table.dives[idx];
		if (dp->latitude.udeg != 0 || dp->longitude.udeg != 0){
			add_gps_point(map, dp->latitude.udeg / 1000000.0,
				dp->longitude.udeg / 1000000.0);
		}
	}
	osm_gps_map_set_center_and_zoom(map, 0, 0, 2);

	show_map(map, &window);
}
