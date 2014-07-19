#include "save-html.h"
#include "gettext.h"
#include "stdio.h"

void write_attribute(struct membuffer *b, const char *att_name, const char *value)
{
	if (!value)
		value = "--";
	put_format(b, "\"%s\":\"", att_name);
	put_HTML_quoted(b, value);
	put_string(b, "\",");
}

void save_photos(struct membuffer *b, const char *photos_dir, struct dive *dive)
{
	struct picture *pic = dive->picture_list;
	put_string(b, "\"photos\":[");
	while (pic) {
		put_format(b, "{\"filename\":\"%s\"},", get_file_name(pic->filename));
		copy_image_and_overwrite(pic->filename, photos_dir);
		pic = pic->next;
	}
	put_string(b, "],");
}

void write_dive_status(struct membuffer *b, struct dive *dive)
{
	put_format(b, "\"sac\":\"%d\",", dive->sac);
	put_format(b, "\"otu\":\"%d\",", dive->otu);
	put_format(b, "\"cns\":\"%d\",", dive->cns);
}

void put_HTML_bookmarks(struct membuffer *b, struct dive *dive)
{
	struct event *ev = dive->dc.events;
	put_string(b, "\"events\":[");
	while (ev) {
		put_format(b, "{\"name\":\"%s\",", ev->name);
		put_format(b, "\"value\":\"%d\",", ev->value);
		put_format(b, "\"time\":\"%d\",},", ev->time.seconds);
		ev = ev->next;
	}
	put_string(b, "],");
}

static void put_cylinder_HTML(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_cylinders(dive);

	put_string(b, "\"Cylinders\":[");

	for (i = 0; i < nr; i++) {
		cylinder_t *cylinder = dive->cylinder + i;
		put_string(b, "{");
		write_attribute(b, "Type", cylinder->type.description);
		if (cylinder->type.size.mliter) {
			put_milli(b, "\"Size\":\"", cylinder->type.size.mliter, " l\",");
		} else {
			write_attribute(b, "Size", "--");
		}
		put_pressure(b, cylinder->type.workingpressure, "\"WPressure\":\"", " bar\",");

		if (cylinder->start.mbar) {
			put_milli(b, "\"SPressure\":\"", cylinder->start.mbar, " bar\",");
		} else {
			write_attribute(b, "SPressure", "--");
		}

		if (cylinder->end.mbar) {
			put_milli(b, "\"EPressure\":\"", cylinder->end.mbar, " bar\",");
		} else {
			write_attribute(b, "EPressure", "--");
		}

		if (cylinder->gasmix.o2.permille) {
			put_format(b, "\"O2\":\"%u.%u%%\",", FRACTION(cylinder->gasmix.o2.permille, 10));
		} else {
			write_attribute(b, "O2", "--");
		}
		put_string(b, "},");
	}

	put_string(b, "],");
}


void put_HTML_samples(struct membuffer *b, struct dive *dive)
{
	int i;
	put_format(b, "\"maxdepth\":%d,", dive->dc.maxdepth.mm);
	put_format(b, "\"duration\":%d,", dive->dc.duration.seconds);
	put_string(b, "\"samples\":\[");
	struct sample *s = dive->dc.sample;
	for (i = 0; i < dive->dc.samples; i++) {
		put_format(b, "[%d,%d,%d,%d],", s->time.seconds, s->depth.mm, s->cylinderpressure.mbar, s->temperature.mkelvin);
		s++;
	}
	put_string(b, "],");
}

void put_HTML_date(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	struct tm tm;
	utc_mkdate(dive->when, &tm);
	put_format(b, "%s%04u-%02u-%02u%s", pre, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, post);
}

void put_HTML_quoted(struct membuffer *b, const char *text)
{
	int is_html = 1, is_attribute = 1;
	put_quoted(b, text, is_attribute, is_html);
}

void put_HTML_notes(struct membuffer *b, struct dive *dive, const char *pre, const char *post)
{
	put_string(b, pre);
	if (dive->notes) {
		put_HTML_quoted(b, dive->notes);
	} else {
		put_string(b, "--");
	}
	put_string(b, post);
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
	put_string(b, "[");
	struct tag_entry *tag = dive->tag_list;

	if (!tag)
		put_string(b, "\"--\",");

	while (tag) {
		put_string(b, "\"");
		put_HTML_quoted(b, tag->tag->name);
		put_string(b, "\",");
		tag = tag->next;
	}
	put_string(b, "]");
	put_string(b, post);
}

/* if exporting list_only mode, we neglect exporting the samples, bookmarks and cylinders */
void write_one_dive(struct membuffer *b, struct dive *dive, const char *photos_dir, int *dive_no, const bool list_only)
{
	put_string(b, "{");
	put_format(b, "\"number\":%d,", *dive_no);
	put_format(b, "\"subsurface_number\":%d,", dive->number);
	put_HTML_date(b, dive, "\"date\":\"", "\",");
	put_HTML_time(b, dive, "\"time\":\"", "\",");
	write_attribute(b, "location", dive->location);
	put_format(b, "\"rating\":%d,", dive->rating);
	put_format(b, "\"visibility\":%d,", dive->visibility);
	put_string(b, "\"temperature\":{");
	put_HTML_airtemp(b, dive, "\"air\":\"", "\",");
	put_HTML_watertemp(b, dive, "\"water\":\"", "\",");
	put_string(b, "	},");
	write_attribute(b, "buddy", dive->buddy);
	write_attribute(b, "divemaster", dive->divemaster);
	write_attribute(b, "suit", dive->suit);
	put_HTML_tags(b, dive, "\"tags\":", ",");
	put_HTML_notes(b, dive, "\"notes\":\"", "\",");
	if (!list_only) {
		put_cylinder_HTML(b, dive);
		put_HTML_samples(b, dive);
		put_HTML_bookmarks(b, dive);
		write_dive_status(b, dive);
		save_photos(b, photos_dir, dive);
	}
	put_string(b, "},\n");
	(*dive_no)++;
}

void write_no_trip(struct membuffer *b, int *dive_no, const char *photos_dir, const bool list_only)
{
	int i;
	struct dive *dive;

	put_format(b, "{");
	put_format(b, "\"name\":\"Other\",");
	put_format(b, "\"dives\":[");

	for_each_dive (i, dive) {
		if (!dive->divetrip)
			write_one_dive(b, dive, photos_dir, dive_no, list_only);
	}
	put_format(b, "]},\n\n");
}

void write_trip(struct membuffer *b, dive_trip_t *trip, int *dive_no, const char *photos_dir, const bool list_only)
{
	struct dive *dive;

	put_format(b, "{");
	put_format(b, "\"name\":\"%s\",", trip->location);
	put_format(b, "\"dives\":[");

	for (dive = trip->dives; dive != NULL; dive = dive->next) {
		write_one_dive(b, dive, photos_dir, dive_no, list_only);
	}

	put_format(b, "]},\n\n");
}

void write_trips(struct membuffer *b, const char *photos_dir, bool selected_only, const bool list_only)
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

		for_each_dive (i, dive) {
			if (!dive->selected)
				continue;
			write_one_dive(b, dive, photos_dir, &dive_no, list_only);
		}
		put_format(b, "]},\n\n");
	} else {

		for_each_dive (i, dive) {
			trip = dive->divetrip;

			/*Continue if the dive have no trips or we have seen this trip before*/
			if (!trip || trip->index)
				continue;

			/* We haven't seen this trip before - save it and all dives */
			trip->index = 1;
			write_trip(b, trip, &dive_no, photos_dir, list_only);
		}

		/*Save all remaining trips into Others*/
		write_no_trip(b, &dive_no, photos_dir, list_only);
	}
}

void export_list(struct membuffer *b, const char *photos_dir, bool selected_only, const bool list_only)
{
	put_string(b, "trips=[");
	write_trips(b, photos_dir, selected_only, list_only);
	put_string(b, "]");
}

void export_HTML(const char *file_name, const char *photos_dir, const bool selected_only, const bool list_only)
{
	FILE *f;

	struct membuffer buf = { 0 };
	export_list(&buf, photos_dir, selected_only, list_only);

	f = subsurface_fopen(file_name, "w+");
	if (!f)
		report_error(translate("gettextFromC", "Can't open file %s"), file_name);

	flush_buffer(&buf, f); /*check for writing errors? */
	free_buffer(&buf);
	fclose(f);
}
