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
	gtk_grab_remove(widget);
	gtk_widget_destroy(widget);
	*window = NULL;
}

struct maplocation {
	OsmGpsMap *map;
	double x,y;
	void (* callback)(float, float);
	GtkWidget *mapwindow;
};

static void add_location_cb(GtkWidget *widget, gpointer data)
{
	struct maplocation *maplocation = data;
	OsmGpsMap *map = maplocation->map;
	OsmGpsMapPoint *pt = osm_gps_map_point_new_degrees(0.0, 0.0);
	float mark_lat, mark_lon;

	osm_gps_map_convert_screen_to_geographic(map, maplocation->x, maplocation->y, pt);
	osm_gps_map_point_get_degrees(pt, &mark_lat, &mark_lon);
	maplocation->callback(mark_lat, mark_lon);
	gtk_widget_destroy(gtk_widget_get_parent(maplocation->mapwindow));
}

static void map_popup_menu_cb(GtkWidget *w, gpointer data)
{
	GtkWidget *menu, *menuitem, *image;

	menu = gtk_menu_new();
	menuitem = gtk_image_menu_item_new_with_label(_("Mark location here"));
	image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect(menuitem, "activate", G_CALLBACK(add_location_cb), data);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		3, gtk_get_current_event_time());
}

static gboolean button_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	static struct maplocation maplocation = {};
	if (data && event->type == GDK_BUTTON_PRESS && event->button == 3) {
		maplocation.map = (OsmGpsMap *)widget;
		maplocation.x = event->x;
		maplocation.y = event->y;
		maplocation.callback = data;
		maplocation.mapwindow = widget;
		map_popup_menu_cb(widget, &maplocation);
	}
	return FALSE;
}

/* the osm gps map default is to scroll-and-recenter around the mouse position
 * that is BAT SHIT CRAZY.
 * So let's do our own scroll handling instead */
static gboolean scroll_cb(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	OsmGpsMap *map = (OsmGpsMap *)widget;
	OsmGpsMapPoint *pt, *lt, *rb, *target;
	float target_lat, target_lon;
	gint ltx, lty, rbx, rby, target_x, target_y;
	gint zoom, max_zoom, min_zoom;

	g_object_get(widget, "max-zoom", &max_zoom, "zoom", &zoom, "min-zoom", &min_zoom, NULL);

	pt = osm_gps_map_point_new_degrees(0.0, 0.0);
	lt = osm_gps_map_point_new_degrees(0.0, 0.0);
	rb = osm_gps_map_point_new_degrees(0.0, 0.0);
	target = osm_gps_map_point_new_degrees(0.0, 0.0);

	osm_gps_map_convert_screen_to_geographic(map, event->x, event->y, pt);

	osm_gps_map_get_bbox(map, lt, rb);
	osm_gps_map_convert_geographic_to_screen(map, lt, &ltx, &lty);
	osm_gps_map_convert_geographic_to_screen(map, rb, &rbx, &rby);

	if (event->direction == GDK_SCROLL_UP) {
	        if (zoom == max_zoom)
		      return TRUE;

		target_x = event->x + ((ltx + rbx) / 2.0 - (gint)(event->x)) / 2;
		target_y = event->y + ((lty + rby) / 2.0 - (gint)(event->y)) / 2;

		osm_gps_map_convert_screen_to_geographic(map, target_x, target_y, target);
		osm_gps_map_point_get_degrees(target, &target_lat, &target_lon);

		osm_gps_map_zoom_in(map);
		osm_gps_map_set_center(map, target_lat, target_lon);
	} else if (event->direction == GDK_SCROLL_DOWN) {
	        if (zoom == min_zoom)
		      return TRUE;

		target_x = event->x + ((ltx + rbx) / 2.0 - (gint)(event->x)) * 2;
		target_y = event->y + ((lty + rby) / 2.0 - (gint)(event->y)) * 2;

		osm_gps_map_convert_screen_to_geographic(map, target_x, target_y, target);
		osm_gps_map_point_get_degrees(target, &target_lat, &target_lon);

		osm_gps_map_zoom_out(map);
		osm_gps_map_set_center(map, target_lat, target_lon);
	}
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

void show_map(OsmGpsMap *map, GtkWidget **window, struct dive *dive, void (*callback)(float, float))
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
		gtk_window_set_title(GTK_WINDOW(*window), _("Dive locations"));
		gtk_container_set_border_width(GTK_CONTAINER(*window), 5);
		gtk_window_set_resizable(GTK_WINDOW(*window), TRUE);
		gtk_container_add(GTK_CONTAINER(*window), GTK_WIDGET(map));
		gtk_window_set_transient_for(GTK_WINDOW(*window), GTK_WINDOW(main_window));
		gtk_window_set_modal(GTK_WINDOW(*window), TRUE);
		g_signal_connect(*window, "destroy", G_CALLBACK(on_close), (gpointer)window);
		g_signal_connect(G_OBJECT(map), "scroll-event", G_CALLBACK(scroll_cb), NULL);
	}
	if (callback)
		g_signal_connect(G_OBJECT(map), "button-press-event", G_CALLBACK(button_cb), callback);
	gtk_widget_show_all(*window);
	if (callback)
		gtk_grab_add(*window);
}

void show_gps_location(struct dive *dive, void (*callback)(float, float))
{
	static GtkWidget *window = NULL;
	static OsmGpsMap *map = NULL;
	GdkPixbuf *picture;
	GError *gerror = NULL;

	double lat = dive->latitude.udeg / 1000000.0;
	double lng = dive->longitude.udeg / 1000000.0;

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
	show_map(map, &window, dive, callback);
}

void show_gps_locations()
{
	static OsmGpsMap *map = NULL;
	static GtkWidget *window = NULL;
	struct dive *dive;
	int idx;

	if (!window || !map)
		map = init_map();

	for_each_dive(idx, dive) {
		if (dive_has_location(dive)) {
			add_gps_point(map, dive->latitude.udeg / 1000000.0,
				dive->longitude.udeg / 1000000.0);
		}
	}
	osm_gps_map_set_center_and_zoom(map, 0, 0, 2);

	show_map(map, &window, NULL, NULL);
}
