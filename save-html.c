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
		char *fname = get_file_name(pic->filename);
		put_format(b, "{\"filename\":\"%s\"},", fname);
		free(fname);
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
		put_format(b, "\"type\":\"%d\",", ev->type);
		put_format(b, "\"time\":\"%d\",},", ev->time.seconds);
		ev = ev->next;
	}
	put_string(b, "],");
}

static void put_weightsystem_HTML(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_weightsystems(dive);

	put_string(b, "\"Weights\":[");

	for (i = 0; i < nr; i++) {
		weightsystem_t *ws = dive->weightsystem + i;
		int grams = ws->weight.grams;
		const char *description = ws->description;

		put_string(b, "{");
		put_format(b, "\"weight\":\"%d\",", grams);
		write_attribute(b, "description", description);
		put_string(b, "},");
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

void put_HTML_coordinates(struct membuffer *b, struct dive *dive)
{
	degrees_t latitude = dive->latitude;
	degrees_t longitude = dive->longitude;

	//don't put coordinates if in (0,0)
	if (!latitude.udeg && !longitude.udeg)
		return;

	put_string(b, "\"coordinates\":{");
	put_degrees(b, latitude, "\"lat\":\"", "\",");
	put_degrees(b, longitude, "\"lon\":\"", "\",");
	put_string(b, "},");
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
	put_HTML_coordinates(b, dive);
	put_format(b, "\"rating\":%d,", dive->rating);
	put_format(b, "\"visibility\":%d,", dive->visibility);
	put_format(b, "\"dive_duration\":\"%u:%02u min\",",
		   FRACTION(dive->duration.seconds, 60));
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
		put_weightsystem_HTML(b, dive);
		put_HTML_samples(b, dive);
		put_HTML_bookmarks(b, dive);
		write_dive_status(b, dive);
		save_photos(b, photos_dir, dive);
	}
	put_string(b, "},\n");
	(*dive_no)++;
}

void write_no_trip(struct membuffer *b, int *dive_no, bool selected_only, const char *photos_dir, const bool list_only)
{
	int i;
	struct dive *dive;
	bool found_sel_dive = 0;

	for_each_dive (i, dive) {
		// write dive if it doesn't belong to any trip and the dive is selected
		// or we are in exporting all dives mode.
		if (!dive->divetrip && (dive->selected || !selected_only)) {
			if (!found_sel_dive) {
				put_format(b, "{");
				put_format(b, "\"name\":\"Other\",");
				put_format(b, "\"dives\":[");
				found_sel_dive = 1;
			}
			write_one_dive(b, dive, photos_dir, dive_no, list_only);
		}
	}
	if (found_sel_dive)
		put_format(b, "]},\n\n");
}

void write_trip(struct membuffer *b, dive_trip_t *trip, int *dive_no, bool selected_only, const char *photos_dir, const bool list_only)
{
	struct dive *dive;
	bool found_sel_dive = 0;

	for (dive = trip->dives; dive != NULL; dive = dive->next) {
		if (!dive->selected && selected_only)
			continue;

		// save trip if found at least one selected dive.
		if (!found_sel_dive) {
			found_sel_dive = 1;
			put_format(b, "{");
			put_format(b, "\"name\":\"%s\",", trip->location);
			put_format(b, "\"dives\":[");
		}
		write_one_dive(b, dive, photos_dir, dive_no, list_only);
	}

	// close the trip object if contain dives.
	if (found_sel_dive)
		put_format(b, "]},\n\n");
}

void write_trips(struct membuffer *b, const char *photos_dir, bool selected_only, const bool list_only)
{
	int i, dive_no = 0;
	struct dive *dive;
	dive_trip_t *trip;

	for (trip = dive_trip_list; trip != NULL; trip = trip->next)
		trip->index = 0;

	for_each_dive (i, dive) {
		trip = dive->divetrip;

		/*Continue if the dive have no trips or we have seen this trip before*/
		if (!trip || trip->index)
			continue;

		/* We haven't seen this trip before - save it and all dives */
		trip->index = 1;
		write_trip(b, trip, &dive_no, selected_only, photos_dir, list_only);
	}

	/*Save all remaining trips into Others*/
	write_no_trip(b, &dive_no, selected_only, photos_dir, list_only);
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

void export_translation(const char *file_name)
{
	FILE *f;

	struct membuffer buf = { 0 };
	struct membuffer *b = &buf;

	//export translated words here
	put_format(b, "translate={");

	//Dive list view
	write_attribute(b, "Number", translate("gettextFromC", "Number"));
	write_attribute(b, "Date", translate("gettextFromC", "Date"));
	write_attribute(b, "Time", translate("gettextFromC", "Time"));
	write_attribute(b, "Location", translate("gettextFromC", "Location"));
	write_attribute(b, "Air_Temp", translate("gettextFromC", "Air Temp"));
	write_attribute(b, "Water_Temp", translate("gettextFromC", "Water Temp"));
	write_attribute(b, "dives", translate("gettextFromC", "dives"));
	write_attribute(b, "Expand_All", translate("gettextFromC", "Expand All"));
	write_attribute(b, "Collapse_All", translate("gettextFromC", "Collapse All"));
	write_attribute(b, "trips", translate("gettextFromC", "trips"));
	write_attribute(b, "Statistics", translate("gettextFromC", "Statistics"));
	write_attribute(b, "Advanced_Search", translate("gettextFromC", "Advanced Search"));

	//Dive expanded view
	write_attribute(b, "Rating", translate("gettextFromC", "Rating"));
	write_attribute(b, "Visibility", translate("gettextFromC", "Visibility"));
	write_attribute(b, "Duration", translate("gettextFromC", "Duration"));
	write_attribute(b, "DiveMaster", translate("gettextFromC", "DiveMaster"));
	write_attribute(b, "Buddy", translate("gettextFromC", "Buddy"));
	write_attribute(b, "Suit", translate("gettextFromC", "Suit"));
	write_attribute(b, "Tags", translate("gettextFromC", "Tags"));
	write_attribute(b, "Notes", translate("gettextFromC", "Notes"));
	write_attribute(b, "Show_more_details", translate("gettextFromC", "Show more details"));

	//Yearly statistics view
	write_attribute(b, "Yearly_statistics", translate("gettextFromC", "Yearly statistics"));
	write_attribute(b, "Year", translate("gettextFromC", "Year"));
	write_attribute(b, "Total_Time", translate("gettextFromC", "Total Time"));
	write_attribute(b, "Average_Time", translate("gettextFromC", "Average Time"));
	write_attribute(b, "Shortest_Time", translate("gettextFromC", "Shortest Time"));
	write_attribute(b, "Longest_Time", translate("gettextFromC", "Longest Time"));
	write_attribute(b, "Average_Depth", translate("gettextFromC", "Average Depth"));
	write_attribute(b, "Min_Depth", translate("gettextFromC", "Min Depth"));
	write_attribute(b, "Max_Depth", translate("gettextFromC", "Max Depth"));
	write_attribute(b, "Average_SAC", translate("gettextFromC", "Average SAC"));
	write_attribute(b, "Min_SAC", translate("gettextFromC", "Min SAC"));
	write_attribute(b, "Max_SAC", translate("gettextFromC", "Max SAC"));
	write_attribute(b, "Average_Temp", translate("gettextFromC", "Average Temp"));
	write_attribute(b, "Min_Temp", translate("gettextFromC", "Min Temp"));
	write_attribute(b, "Max_Temp", translate("gettextFromC", "Max Temp"));
	write_attribute(b, "Back_to_List", translate("gettextFromC", "Back to List"));

	//dive detailed view
	write_attribute(b, "Dive_No", translate("gettextFromC", "Dive No."));
	write_attribute(b, "Dive_profile", translate("gettextFromC", "Dive profile"));
	write_attribute(b, "Dive_information", translate("gettextFromC", "Dive information"));
	write_attribute(b, "Dive_equipments", translate("gettextFromC", "Dive equipments"));
	write_attribute(b, "Type", translate("gettextFromC", "Type"));
	write_attribute(b, "Size", translate("gettextFromC", "Size"));
	write_attribute(b, "Work_Pressure", translate("gettextFromC", "Work Pressure"));
	write_attribute(b, "Start_Pressure", translate("gettextFromC", "Start Pressure"));
	write_attribute(b, "End_Pressure", translate("gettextFromC", "End Pressure"));
	write_attribute(b, "Weight", translate("gettextFromC", "Weight"));
	write_attribute(b, "Type", translate("gettextFromC", "Type"));
	write_attribute(b, "Events", translate("gettextFromC", "Events"));
	write_attribute(b, "Name", translate("gettextFromC", "Name"));
	write_attribute(b, "Value", translate("gettextFromC", "Value"));
	write_attribute(b, "Coordinates", translate("gettextFromC", "Coordinates"));
	write_attribute(b, "Dive_Status", translate("gettextFromC", "Dive Status"));

	put_format(b, "}");

	f = subsurface_fopen(file_name, "w+");
	if (!f)
		report_error(translate("gettextFromC", "Can't open file %s"), file_name);

	flush_buffer(&buf, f); /*check for writing errors? */
	free_buffer(&buf);
	fclose(f);
}
