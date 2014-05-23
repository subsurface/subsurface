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

	value = get_temp_units(dive->airtemp.mkelvin, &unit);
	put_format(b, "%s%.1f %s%s",pre, value, unit, post);
}

void put_HTML_watertemp(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	const char *unit;
	double value;

	value = get_temp_units(dive->watertemp.mkelvin, &unit);
	put_format(b, "%s%.1f %s%s",pre, value, unit, post);
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

void write_dives(struct membuffer *b,bool selected_only)
{
	int i, dive_no = 0;
	struct dive *dive;

	for_each_dive(i, dive) {
		if (selected_only) {
			if (!dive->selected)
				continue;
		}
		put_string(b, "{");
		put_format(b, "\"number\":%d,", dive_no);
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
		dive_no++;
	}
}

void export_dives(struct membuffer *b,bool selected_only){
	put_string(b, "items=[");
	write_dives(b, selected_only);
	put_string(b, "]");
}

void export_HTML(const char *file_name, const bool selected_only)
{
	FILE *f;

	struct membuffer buf = { 0 };
	export_dives(&buf, selected_only);

	f = fopen(file_name, "w+");
	if (!f)
		printf("error"); /*report error*/

	flush_buffer(&buf, f); /*check for writing errors? */
	free_buffer(&buf);
	fclose(f);
}
