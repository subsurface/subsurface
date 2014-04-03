#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "dive.h"
#include "membuffer.h"
#include "worldmap-save.h"
#include "worldmap-options.h"

char *getGoogleApi()
{
	/* google maps api auth*/
	return "https://maps.googleapis.com/maps/api/js?key=AIzaSyDzo9PWsqYDDSddVswg_13rpD9oH_dLuoQ";
}

void put_HTML_date(struct membuffer *b, struct dive *dive)
{
	struct tm tm;
	utc_mkdate(dive->when, &tm);
	put_format(b, "<p>date=%04u-%02u-%02u</p>", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	put_format(b, "<p>time=%02u:%02u:%02u</p>", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void put_HTML_temp(struct membuffer *b, struct dive *dive)
{
	put_temperature(b, dive->airtemp, "<p>Air Temp: ", " C\\'</p>");
	put_temperature(b, dive->watertemp, "<p>Water Temp: ", " C\\'</p>");
}

char *replace_char(char *str, char replace, char *replace_by)
{
	/*
		this fumction can't replace a character with a substring
		where the substring contains the character, infinte loop.
	*/

	if (!str)
		return 0;

	int i = 0, char_count = 0, new_size;

	while (str[i] != '\0') {
		if (str[i] == replace)
			char_count++;
		i++;
	}

	new_size = strlen(str) + char_count * strlen(replace_by) + 1;
	char *result = malloc(new_size);
	char *temp = strdup(str);
	char *p0, *p1;
	if (!result || !temp)
		return 0;
	result[0] = '\0';
	p0 = temp;
	p1 = strchr(temp, replace);
	while (p1) {
		*p1 = '\0';
		strcat(result, p0);
		strcat(result, replace_by);
		p0 = p1 + 1;
		p1 = strchr(p0, replace);
	}
	strcat(result, p0); /*concat the rest of the string*/
	free(temp);
	return result;
}

char *quote(char *string)
{
	char *new_line_removed = replace_char(string, '\n', "<br>");
	char *single_quotes_removed = replace_char(new_line_removed, '\'', "&#39;");
	free(new_line_removed);
	return single_quotes_removed;
}

void put_HTML_notes(struct membuffer *b, struct dive *dive)
{
	if (dive->notes) {
		char *notes = quote(dive->notes);
		put_format(b, "<p>Notes : %s </p>", notes);
		free(notes);
	}
}

void writeMarkers(struct membuffer *b)
{
	int i;
	struct dive *dive;

	for_each_dive(i, dive) {
		/*export selected dives only ?*/

		if (dive->latitude.udeg == 0 && dive->longitude.udeg == 0)
			continue;

		put_format(b, "temp = new google.maps.Marker({position: new google.maps.LatLng(%f,%f)});\n",
			   dive->latitude.udeg / 1000000.0, dive->longitude.udeg / 1000000.0);
		put_string(b, "markers.push(temp);\ntempinfowindow = new google.maps.InfoWindow({content: '<div id=\"content\">'+'<div id=\"siteNotice\">'+'</div>'+'<div id=\"bodyContent\">");
		put_HTML_date(b, dive);
		put_duration(b, dive->duration, "<p>duration=", " min</p>");
		put_HTML_temp(b, dive);
		put_HTML_notes(b, dive);
		put_string(b, "</p>'+'</div>'+'</div>'});\ninfowindows.push(tempinfowindow);\n");
		put_format(b, "google.maps.event.addListener(markers[%d], 'mouseover', function() {\ninfowindows[%d].open(map,markers[%d]);}", i, i, i);
		put_format(b, ");google.maps.event.addListener(markers[%d], 'mouseout', function() {\ninfowindows[%d].close();});\n", i, i);
	}
}

void insert_html_header(struct membuffer *b)
{
	put_string(b, "<!DOCTYPE html>\n<html>\n<head>\n");
	put_string(b, "<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\" />\n<title>World Map</title>\n");
}

void insert_css(struct membuffer *b)
{
	put_format(b, "<style type=\"text/css\">%s</style>\n", css);
}

void insert_javascript(struct membuffer *b)
{
	put_string(b, "<script type=\"text/javascript\"src=\"");
	put_string(b, getGoogleApi());
	put_string(b, "&sensor=false\">\n</script>\n<script type=\"text/javascript\">\nvar map;\n");
	put_format(b, "function initialize() {\nvar mapOptions = {\n\t%s,", map_options);
	put_string(b, "rotateControl: false,\n\tstreetViewControl: false,\n\tmapTypeControl: false\n};\n");
	put_string(b, "map = new google.maps.Map(document.getElementById(\"map-canvas\"),mapOptions);\nvar markers = new Array();");
	put_string(b, "\nvar infowindows = new Array();\nvar temp;\nvar tempinfowindow;\n");
	writeMarkers(b);
	put_string(b, "\nfor(var i=0;i<markers.length;i++)\n\tmarkers[i].setMap(map);\n}\n");
	put_string(b, "google.maps.event.addDomListener(window, 'load', initialize);</script>\n");
}

void export(struct membuffer *b)
{
	insert_html_header(b);
	insert_css(b);
	insert_javascript(b);
	put_string(b, "\t</head>\n<body>\n<div id=\"map-canvas\"/>\n</body>\n</html>");
}

void export_worldmap_HTML(const char *file_name)
{
	FILE *f;

	struct membuffer buf = { 0 };
	export(&buf);

	f = fopen(file_name, "w+");
	if (!f)
		printf("error"); /*report error*/

	flush_buffer(&buf, f); /*check for writing errors? */
	free_buffer(&buf);
	fclose(f);
}
