/* gps.c */
/* Creates the UI displaying the dives locations on a map.
 */
#include <glib/gi18n.h>

#include "osm-gps-map.h"

#include "dive.h"
#include "display.h"
#include "display-gtk.h"
#include "divelist.h"

static OsmGpsMapSource_t opt_map_provider = OSM_GPS_MAP_SOURCE_OPENSTREETMAP;


static void on_close (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_destroy(widget);
}

static void add_gps_point(OsmGpsMap *map, float latitude, float longitude)
{
	OsmGpsMapTrack * track = osm_gps_map_track_new ();
	OsmGpsMapPoint * point = osm_gps_map_point_new_degrees (latitude, longitude);
	osm_gps_map_track_add_point(track, point);
	osm_gps_map_track_add (map, track);
}


void show_gps_locations()
{
	GtkWidget *window;

	OsmGpsMap *map;
	OsmGpsMapLayer *osd;
	char *cachedir, *cachebasedir;
	int idx;
	struct dive *dp;

	cachebasedir = osm_gps_map_get_default_cache_directory();
	cachedir = g_strdup(OSM_GPS_MAP_CACHE_AUTO);

	map = g_object_new (OSM_TYPE_GPS_MAP,
				"map-source",opt_map_provider,
				"tile-cache",cachedir,
				"tile-cache-base", cachebasedir,
				"proxy-uri",g_getenv("http_proxy"),
				NULL);
	osd = g_object_new (OSM_TYPE_GPS_MAP_OSD,
				"show-scale",TRUE,
				"show-coordinates",TRUE,
				"show-crosshair",TRUE,
				"show-dpad",TRUE,
				"show-zoom",TRUE,
				"show-gps-in-dpad",TRUE,
				"show-gps-in-zoom",FALSE,
				"dpad-radius", 30,
				NULL);

	osm_gps_map_layer_add(OSM_GPS_MAP(map), osd);
	g_object_unref(G_OBJECT(osd));
	for (idx = 0; idx < dive_table.nr; idx++) {
		dp = dive_table.dives[idx];
		if (dp->latitude.udeg != 0 && dp->longitude.udeg != 0){
			add_gps_point(map, dp->latitude.udeg / 1000000.0,
				dp->longitude.udeg / 1000000.0);
		}
	}
	osm_gps_map_set_center_and_zoom(map, 0, 0, 0);

	/* Enable keyboard navigation */
	osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_FULLSCREEN, GDK_F11);
	osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_UP, GDK_Up);
	osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_DOWN, GDK_Down);
	osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_LEFT, GDK_Left);
	osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_RIGHT, GDK_Right);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	gtk_window_set_title(GTK_WINDOW(window), _("Dives locations"));
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	GTK_WINDOW(window)->allow_shrink = TRUE;

	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET(map));

	g_signal_connect (window, "destroy", G_CALLBACK (on_close), (gpointer) map);

	gtk_widget_show_all (window);
}
