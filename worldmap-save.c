#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dive.h"
#include "membuffer.h"
#include "save-html.h"
#include "worldmap-save.h"
#include "worldmap-options.h"
#include "gettext.h"

char *getGoogleApi()
{
	/* google maps api auth*/
	return "https://maps.googleapis.com/maps/api/js?key=AIzaSyDzo9PWsqYDDSddVswg_13rpD9oH_dLuoQ";
}

void writeMarkers(struct membuffer *b, const bool selected_only)
{
	int i, dive_no = 0;
	struct dive *dive;
	char pre[1000], post[1000];

	for_each_dive (i, dive) {
		if (selected_only) {
			if (!dive->selected)
				continue;
		}
		if (dive->latitude.udeg == 0 && dive->longitude.udeg == 0)
			continue;

		put_degrees(b, dive->latitude, "temp = new google.maps.Marker({position: new google.maps.LatLng(", "");
		put_degrees(b, dive->longitude, ",", ")});\n");
		put_string(b, "markers.push(temp);\ntempinfowindow = new google.maps.InfoWindow({content: '<div id=\"content\">'+'<div id=\"siteNotice\">'+'</div>'+'<div id=\"bodyContent\">");
		snprintf(pre, sizeof(pre), "<p>%s ", translate("gettextFromC", "Date:"));
		put_HTML_date(b, dive, pre, "</p>");
		snprintf(pre, sizeof(pre), "<p>%s ", translate("gettextFromC", "Time:"));
		put_HTML_time(b, dive, pre, "</p>");
		snprintf(pre, sizeof(pre), "<p>%s ", translate("gettextFromC", "Duration:"));
		snprintf(post, sizeof(post), " %s</p>", translate("gettextFromC", "min"));
		put_duration(b, dive->duration, pre, post);
		snprintf(pre, sizeof(pre), "<p>%s ", translate("gettextFromC", "Max depth:"));
		snprintf(post, sizeof(post), " %s</p>", translate("gettextFromC", "m"));
		put_depth(b, dive->maxdepth, pre, post);
		snprintf(pre, sizeof(pre), "<p>%s ", translate("gettextFromC", "Air temp:"));
		put_HTML_airtemp(b, dive, pre, "</p>");
		snprintf(pre, sizeof(pre), "<p>%s ", translate("gettextFromC", "Water temp:"));
		put_HTML_watertemp(b, dive, pre, "</p>");
		snprintf(pre, sizeof(pre), "<p>%s <b>", translate("gettextFromC", "Location:"));
		put_string(b, pre);
		put_HTML_quoted(b, dive->location);
		put_string(b, "</b></p>");
		snprintf(pre, sizeof(pre), "<p> %s ", translate("gettextFromC", "Notes:"));
		put_HTML_notes(b, dive, pre, " </p>");
		put_string(b, "</p>'+'</div>'+'</div>'});\ninfowindows.push(tempinfowindow);\n");
		put_format(b, "google.maps.event.addListener(markers[%d], 'mouseover', function() {\ninfowindows[%d].open(map,markers[%d]);}", dive_no, dive_no, dive_no);
		put_format(b, ");google.maps.event.addListener(markers[%d], 'mouseout', function() {\ninfowindows[%d].close();});\n", dive_no, dive_no);
		dive_no++;
	}
}

void insert_html_header(struct membuffer *b)
{
	put_string(b, "<!DOCTYPE html>\n<html>\n<head>\n");
	put_string(b, "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\" />\n<title>World Map</title>\n");
	put_string(b, "<meta charset=\"UTF-8\">");
}

void insert_css(struct membuffer *b)
{
	put_format(b, "<style type=\"text/css\">%s</style>\n", css);
}

void insert_javascript(struct membuffer *b, const bool selected_only)
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

void export(struct membuffer *b, const bool selected_only)
{
	insert_html_header(b);
	insert_css(b);
	insert_javascript(b, selected_only);
	put_string(b, "\t</head>\n<body>\n<div id=\"map-canvas\"></div>\n</body>\n</html>");
}

void export_worldmap_HTML(const char *file_name, const bool selected_only)
{
	FILE *f;

	struct membuffer buf = { 0 };
	export(&buf, selected_only);

	f = subsurface_fopen(file_name, "w+");
	if (!f) {
		report_error(translate("gettextFromC", "Can't open file %s"), file_name);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
	free_buffer(&buf);
}
