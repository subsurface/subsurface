// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dive.h"
#include "membuffer.h"
#include "divesite.h"
#include "errorhelper.h"
#include "file.h"
#include "save-html.h"
#include "format.h"
#include "worldmap-save.h"
#include "worldmap-options.h"
#include "gettext.h"

static const char *getGoogleApi()
{
	/* google maps api auth*/
	return "https://maps.googleapis.com/maps/api/js?";
}

static void writeMarkers(struct membuffer *b, bool selected_only)
{
	int i, dive_no = 0;
	struct dive *dive;
	std::string pre, post;

	for_each_dive (i, dive) {
		if (selected_only) {
			if (!dive->selected)
				continue;
		}
		struct dive_site *ds = get_dive_site_for_dive(dive);
		if (!dive_site_has_gps_location(ds))
			continue;
		put_degrees(b, ds->location.lat, "temp = new google.maps.Marker({position: new google.maps.LatLng(", "");
		put_degrees(b, ds->location.lon, ",", ")});\n");
		put_string(b, "markers.push(temp);\ntempinfowindow = new google.maps.InfoWindow({content: '<div id=\"content\">'+'<div id=\"siteNotice\">'+'</div>'+'<div id=\"bodyContent\">");
		pre = format_string_std("<p>%s ", translate("gettextFromC", "Date:"));
		put_HTML_date(b, dive, pre.c_str(), "</p>");
		pre = format_string_std("<p>%s ", translate("gettextFromC", "Time:"));
		put_HTML_time(b, dive, pre.c_str(), "</p>");
		pre = format_string_std("<p>%s ", translate("gettextFromC", "Duration:"));
		post = format_string_std(" %s</p>", translate("gettextFromC", "min"));
		put_duration(b, dive->duration, pre.c_str(), post.c_str());
		put_string(b, "<p> ");
		put_HTML_quoted(b, translate("gettextFromC", "Max. depth:"));
		put_HTML_depth(b, dive, " ", "</p>");
		put_string(b, "<p> ");
		put_HTML_quoted(b, translate("gettextFromC", "Air temp.:"));
		put_HTML_airtemp(b, dive, " ", "</p>");
		put_string(b, "<p> ");
		put_HTML_quoted(b, translate("gettextFromC", "Water temp.:"));
		put_HTML_watertemp(b, dive, " ", "</p>");
		pre = format_string_std("<p>%s <b>", translate("gettextFromC", "Location:"));
		put_string(b, pre.c_str());
		put_HTML_quoted(b, get_dive_location(dive));
		put_string(b, "</b></p>");
		pre = format_string_std("<p> %s ", translate("gettextFromC", "Notes:"));
		put_HTML_notes(b, dive, pre.c_str(), " </p>");
		put_string(b, "</p>'+'</div>'+'</div>'});\ninfowindows.push(tempinfowindow);\n");
		put_format(b, "google.maps.event.addListener(markers[%d], 'mouseover', function() {\ninfowindows[%d].open(map,markers[%d]);}", dive_no, dive_no, dive_no);
		put_format(b, ");google.maps.event.addListener(markers[%d], 'mouseout', function() {\ninfowindows[%d].close();});\n", dive_no, dive_no);
		dive_no++;
	}
}

static void insert_html_header(struct membuffer *b)
{
	put_string(b, "<!DOCTYPE html>\n<html>\n<head>\n");
	put_string(b, "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\" />\n<title>World Map</title>\n");
	put_string(b, "<meta charset=\"UTF-8\">");
}

static void insert_css(struct membuffer *b)
{
	put_format(b, "<style type=\"text/css\">%s</style>\n", css);
}

static void insert_javascript(struct membuffer *b, const bool selected_only)
{
	put_string(b, "<script type=\"text/javascript\" src=\"");
	put_string(b, getGoogleApi());
	put_string(b, "&amp;sensor=false\">\n</script>\n<script type=\"text/javascript\">\nvar map;\n");
	put_format(b, "function initialize() {\nvar mapOptions = {\n\t%s,", map_options);
	put_string(b, "rotateControl: false,\n\tstreetViewControl: false,\n\tmapTypeControl: false\n};\n");
	put_string(b, "map = new google.maps.Map(document.getElementById(\"map-canvas\"),mapOptions);\nvar markers = new Array();");
	put_string(b, "\nvar infowindows = new Array();\nvar temp;\nvar tempinfowindow;\n");
	writeMarkers(b, selected_only);
	put_string(b, "\nfor(var i=0;i<markers.length;i++)\n\tmarkers[i].setMap(map);\n}\n");
	put_string(b, "google.maps.event.addDomListener(window, 'load', initialize);</script>\n");
}

static void export_doit(struct membuffer *b, const bool selected_only)
{
	insert_html_header(b);
	insert_css(b);
	insert_javascript(b, selected_only);
	put_string(b, "\t</head>\n<body>\n<div id=\"map-canvas\"></div>\n</body>\n</html>");
}

extern "C" void export_worldmap_HTML(const char *file_name, const bool selected_only)
{
	FILE *f;

	struct membufferpp buf;
	export_doit(&buf, selected_only);

	f = subsurface_fopen(file_name, "w+");
	if (!f) {
		report_error(translate("gettextFromC", "Can't open file %s"), file_name);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
}
