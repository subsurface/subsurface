#include "save-html.h"
#include "gettext.h"

void put_HTML_date(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	struct tm tm;
	utc_mkdate(dive->when, &tm);
	put_format(b, "%s%04u-%02u-%02u%s", pre, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, post);
}

char *replace_char(char *str, char replace, char *replace_by)
{
	/*
		this function can't replace a character with a substring
		where the substring contains the character, infinite loop.
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

void put_HTML_notes(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	if (dive->notes) {
		char *notes = quote(dive->notes);
		put_format(b, "%s: %s %s", pre, notes, post);
		free(notes);
	}
}

void put_HTML_time(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	struct tm tm;
	utc_mkdate(dive->when, &tm);
	put_format(b, "%s%02u:%02u:%02u%s", pre, tm.tm_hour, tm.tm_min, tm.tm_sec, post);
}

void put_HTML_airtemp(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	const char *unit;
	double value;

	if (!dive->airtemp.mkelvin) {
		put_format(b, "%s--%s", pre, post);
		return;
	}
	value = get_temp_units(dive->airtemp.mkelvin, &unit);
	put_format(b, "%s%.1f %s%s", pre, value, unit, post);
}

void put_HTML_watertemp(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	const char *unit;
	double value;

	if (!dive->watertemp.mkelvin) {
		put_format(b, "%s--%s", pre, post);
		return;
	}
	value = get_temp_units(dive->watertemp.mkelvin, &unit);
	put_format(b, "%s%.1f %s%s", pre, value, unit, post);
}

void put_HTML_tags(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	put_string(b, pre);
	struct tag_entry *tag = dive->tag_list;
	while(tag){
		put_format(b, "%s ", tag->tag->name);
		tag = tag->next;
	}
	put_string(b, post);
}

void write_one_dive(struct membuffer *b, struct dive *dive, int *dive_no)
{
	put_string(b, "{");
	put_format(b, "\"number\":%d,", *dive_no);
	put_format(b, "\"subsurface_number\":%d,", dive->number);
	put_HTML_date(b, dive, "\"date\":\"", "\",");
	put_HTML_time(b, dive, "\"time\":\"", "\",");
	put_format(b, "\"location\":\"%s\",", dive->location);
	put_format(b, "\"rating\":%d,", dive->rating);
	put_format(b, "\"visibility\":%d,", dive->visibility);
	put_string(b, "\"temperature\":{");
	put_HTML_airtemp(b, dive, "\"air\":\"", "\",");
	put_HTML_watertemp(b, dive, "\"water\":\"", "\",");
	put_string(b, "	},");
	put_format(b, "\"buddy\":\"%s\",", dive->buddy);
	put_format(b, "\"divemaster\":\"%s\",", dive->divemaster);
	put_format(b, "\"suit\":\"%s\",", dive->suit);
	put_HTML_tags(b, dive, "\"tags\":\"", "\",");
	put_HTML_notes(b, dive ,"\"notes\":\"" ,"\",");
	put_string(b, "},\n");
	(*dive_no)++;
}

void write_no_trip (struct membuffer *b, int *dive_no)
{
	int i;
	struct dive *dive;

	put_format(b, "{");
	put_format(b, "\"name\":\"Other\",");
	put_format(b, "\"dives\":[");

	for_each_dive(i, dive) {
		if (!dive->divetrip)
			write_one_dive(b, dive, dive_no);
	}
	put_format(b, "]},\n\n");
}

void write_trip (struct membuffer *b, dive_trip_t *trip, int *dive_no)
{
	int i;
	struct dive *dive;

	put_format(b, "{");
	put_format(b, "\"name\":\"%s\",",trip->location);
	put_format(b, "\"dives\":[");

	for (dive = trip->dives; dive != NULL; dive = dive->next){
		write_one_dive(b, dive, dive_no);
	}

	put_format(b, "]},\n\n");
}

void write_trips(struct membuffer *b,bool selected_only)
{
	int i, dive_no = 0;
	struct dive *dive;
	dive_trip_t *trip;

	for (trip = dive_trip_list; trip != NULL; trip = trip->next)
		trip->index = 0;

	if (selected_only) {
		put_format(b, "{");
		put_format(b, "\"name\":\"Other\",");
		put_format(b, "\"dives\":[");

		for_each_dive(i, dive) {
			if (!dive->selected)
				continue;
			write_one_dive(b, dive, &dive_no);
		}
		put_format(b, "]},\n\n");
	} else {

		for_each_dive(i, dive) {

			trip = dive->divetrip;

			/*Continue if the dive have no trips or we have seen this trip before*/
			if (!trip || trip->index)
				continue;

			/* We haven't seen this trip before - save it and all dives */
			trip->index = 1;
			write_trip(b, trip, &dive_no);
		}

		/*Save all remaining trips into Others*/
		write_no_trip(b, &dive_no);
	}
}

void export_list(struct membuffer *b,bool selected_only){
	put_string(b, "trips=[");
	write_trips(b, selected_only);
	put_string(b, "]");
}

void export_HTML(const char *file_name, const bool selected_only)
{
	FILE *f;

	struct membuffer buf = { 0 };
	export_list(&buf, selected_only);

	f = fopen(file_name, "w+");
	if (!f)
		printf("error"); /*report error*/

	flush_buffer(&buf, f); /*check for writing errors? */
	free_buffer(&buf);
	fclose(f);
}
