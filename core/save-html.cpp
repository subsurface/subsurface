// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "save-html.h"
#include "dive.h"
#include "divelog.h"
#include "qthelper.h"
#include "gettext.h"
#include "divesite.h"
#include "errorhelper.h"
#include "event.h"
#include "file.h"
#include "picture.h"
#include "sample.h"
#include "tag.h"
#include "subsurface-time.h"
#include "trip.h"

#include <stdio.h>
#include <string.h>
#include <QFile>

static void write_attribute(struct membuffer *b, const char *att_name, const char *value, const char *separator)
{
	if (!value)
		value = "--";
	put_format(b, "\"%s\":\"", att_name);
	put_HTML_quoted(b, value);
	put_format(b, "\"%s", separator);
}

static void copy_image_and_overwrite(const std::string &cfileName, const std::string &path, const std::string &cnewName)
{
	QString fileName = QString::fromStdString(cfileName);
	std::string newName = path + cnewName;
	QFile file(QString::fromStdString(newName));
	if (file.exists())
		file.remove();
	if (!QFile::copy(fileName, QString::fromStdString(newName)))
		report_info("copy of %s to %s failed", cfileName.c_str(), newName.c_str());
}

static void save_photos(struct membuffer *b, const char *photos_dir, const struct dive &dive)
{
	if (dive.pictures.empty())
		return;

	const char *separator = "\"photos\":[";
	for (auto &picture: dive.pictures) {
		put_string(b, separator);
		separator = ", ";
		std::string fname = get_file_name(local_file_path(picture));
		put_string(b, "{\"filename\":\"");
		put_quoted(b, fname.c_str(), 1, 0);
		put_string(b, "\"}");
		copy_image_and_overwrite(local_file_path(picture), photos_dir, fname.c_str());
	}
	put_string(b, "],");
}

static void write_divecomputers(struct membuffer *b, const struct dive &dive)
{
	put_string(b, "\"divecomputers\":[");
	const char *separator = "";
	for (auto &dc: dive.dcs) {
		put_string(b, separator);
		separator = ", ";
		put_format(b, "{");
		write_attribute(b, "model", dc.model.c_str(), ", ");
		if (dc.deviceid)
			put_format(b, "\"deviceid\":\"%08x\", ", dc.deviceid);
		else
			put_string(b, "\"deviceid\":\"--\", ");
		if (dc.diveid)
			put_format(b, "\"diveid\":\"%08x\" ", dc.diveid);
		else
			put_string(b, "\"diveid\":\"--\" ");
		put_format(b, "}");
	}
	put_string(b, "],");
}

static void write_dive_status(struct membuffer *b, const struct dive &dive)
{
	put_format(b, "\"sac\":\"%d\",", dive.sac);
	put_format(b, "\"otu\":\"%d\",", dive.otu);
	put_format(b, "\"cns\":\"%d\",", dive.cns);
}

static void put_HTML_bookmarks(struct membuffer *b, const struct dive &dive)
{
	const char *separator = "";
	put_string(b, "\"events\":[");
	for (const auto &ev: dive.dcs[0].events) {
		put_string(b, separator);
		separator = ", ";
		put_string(b, "{\"name\":\"");
		put_quoted(b, ev.name.c_str(), 1, 0);
		put_string(b, "\",");
		put_format(b, "\"value\":\"%d\",", ev.value);
		put_format(b, "\"type\":\"%d\",", ev.type);
		put_format(b, "\"time\":\"%d\"}", ev.time.seconds);
	}
	put_string(b, "],");
}

static void put_weightsystem_HTML(struct membuffer *b, const struct dive &dive)
{
	put_string(b, "\"Weights\":[");

	const char *separator = "";

	for (auto &ws: dive.weightsystems) {
		int grams = ws.weight.grams;

		put_string(b, separator);
		separator = ", ";
		put_string(b, "{");
		put_HTML_weight_units(b, grams, "\"weight\":\"", "\",");
		write_attribute(b, "description", ws.description.c_str(), " ");
		put_string(b, "}");
	}
	put_string(b, "],");
}

static void put_cylinder_HTML(struct membuffer *b, const struct dive &dive)
{
	const char *separator = "\"Cylinders\":[";

	if (dive.cylinders.empty())
		put_string(b, separator);

	for (auto &cyl: dive.cylinders) {
		put_format(b, "%s{", separator);
		separator = ", ";
		write_attribute(b, "Type", cyl.type.description.c_str(), ", ");
		if (cyl.type.size.mliter) {
			int volume = cyl.type.size.mliter;
			if (prefs.units.volume == units::CUFT && cyl.type.workingpressure.mbar)
				volume = lrint(volume * bar_to_atm(cyl.type.workingpressure.mbar / 1000.0));
			put_HTML_volume_units(b, volume, "\"Size\":\"", " \", ");
		} else {
			write_attribute(b, "Size", "--", ", ");
		}
		put_HTML_pressure_units(b, cyl.type.workingpressure, "\"WPressure\":\"", " \", ");

		if (cyl.start.mbar) {
			put_HTML_pressure_units(b, cyl.start, "\"SPressure\":\"", " \", ");
		} else {
			write_attribute(b, "SPressure", "--", ", ");
		}

		if (cyl.end.mbar) {
			put_HTML_pressure_units(b, cyl.end, "\"EPressure\":\"", " \", ");
		} else {
			write_attribute(b, "EPressure", "--", ", ");
		}

		if (cyl.gasmix.o2.permille) {
			put_format(b, "\"O2\":\"%u.%u%%\",", FRACTION_TUPLE(cyl.gasmix.o2.permille, 10));
			put_format(b, "\"He\":\"%u.%u%%\"", FRACTION_TUPLE(cyl.gasmix.he.permille, 10));
		} else {
			write_attribute(b, "O2", "Air", "");
		}

		put_string(b, "}");
	}

	put_string(b, "],");
}


static void put_HTML_samples(struct membuffer *b, const struct dive &dive)
{
	put_format(b, "\"maxdepth\":%d,", dive.dcs[0].maxdepth.mm);
	put_format(b, "\"duration\":%d,", dive.dcs[0].duration.seconds);

	if (dive.dcs[0].samples.empty())
		return;

	const char *separator = "\"samples\":[";
	for (auto &s: dive.dcs[0].samples) {
		put_format(b, "%s[%d,%d,%d,%d]", separator, s.time.seconds, s.depth.mm, s.pressure[0].mbar, s.temperature.mkelvin);
		separator = ", ";
	}
	put_string(b, "],");
}

static void put_HTML_coordinates(struct membuffer *b, const struct dive &dive)
{
	struct dive_site *ds = dive.dive_site;
	if (!ds)
		return;
	degrees_t latitude = ds->location.lat;
	degrees_t longitude = ds->location.lon;

	//don't put coordinates if in (0,0)
	if (!latitude.udeg && !longitude.udeg)
		return;

	put_string(b, "\"coordinates\":{");
	put_degrees(b, latitude, "\"lat\":\"", "\",");
	put_degrees(b, longitude, "\"lon\":\"", "\"");
	put_string(b, "},");
}

void put_HTML_date(struct membuffer *b, const struct dive &dive, const char *pre, const char *post)
{
	struct tm tm;
	utc_mkdate(dive.when, &tm);
	put_format(b, "%s%04u-%02u-%02u%s", pre, tm.tm_year, tm.tm_mon + 1, tm.tm_mday, post);
}

void put_HTML_quoted(struct membuffer *b, const char *text)
{
	int is_html = 1, is_attribute = 1;
	put_quoted(b, text, is_attribute, is_html);
}

void put_HTML_notes(struct membuffer *b, const struct dive &dive, const char *pre, const char *post)
{
	put_string(b, pre);
	if (!dive.notes.empty())
		put_HTML_quoted(b, dive.notes.c_str());
	else
		put_string(b, "--");
	put_string(b, post);
}

void put_HTML_pressure_units(struct membuffer *b, pressure_t pressure, const char *pre, const char *post)
{
	const char *unit;
	double value;

	if (!pressure.mbar) {
		put_format(b, "%s%s", pre, post);
		return;
	}

	value = get_pressure_units(pressure.mbar, &unit);
	put_format(b, "%s%.1f %s%s", pre, value, unit, post);
}

void put_HTML_volume_units(struct membuffer *b, unsigned int ml, const char *pre, const char *post)
{
	const char *unit;
	double value;
	int frac;

	value = get_volume_units(ml, &frac, &unit);
	put_format(b, "%s%.1f %s%s", pre, value, unit, post);
}

void put_HTML_weight_units(struct membuffer *b, unsigned int grams, const char *pre, const char *post)
{
	const char *unit;
	double value;
	int frac;

	value = get_weight_units(grams, &frac, &unit);
	put_format(b, "%s%.1f %s%s", pre, value, unit, post);
}

void put_HTML_time(struct membuffer *b, const struct dive &dive, const char *pre, const char *post)
{
	struct tm tm;
	utc_mkdate(dive.when, &tm);
	put_format(b, "%s%02u:%02u:%02u%s", pre, tm.tm_hour, tm.tm_min, tm.tm_sec, post);
}

void put_HTML_depth(struct membuffer *b, const struct dive &dive, const char *pre, const char *post)
{
	const char *unit;
	double value;
	const struct units *units_p = get_units();

	if (!dive.maxdepth.mm) {
		put_format(b, "%s--%s", pre, post);
		return;
	}
	value = get_depth_units(dive.maxdepth, NULL, &unit);

	switch (units_p->length) {
	case units::METERS:
	default:
		put_format(b, "%s%.1f %s%s", pre, value, unit, post);
		break;
	case units::FEET:
		put_format(b, "%s%.0f %s%s", pre, value, unit, post);
		break;
	}
}

void put_HTML_airtemp(struct membuffer *b, const struct dive &dive, const char *pre, const char *post)
{
	const char *unit;
	double value;

	if (!dive.airtemp.mkelvin) {
		put_format(b, "%s--%s", pre, post);
		return;
	}
	value = get_temp_units(dive.airtemp.mkelvin, &unit);
	put_format(b, "%s%.1f %s%s", pre, value, unit, post);
}

void put_HTML_watertemp(struct membuffer *b, const struct dive &dive, const char *pre, const char *post)
{
	const char *unit;
	double value;

	if (!dive.watertemp.mkelvin) {
		put_format(b, "%s--%s", pre, post);
		return;
	}
	value = get_temp_units(dive.watertemp.mkelvin, &unit);
	put_format(b, "%s%.1f %s%s", pre, value, unit, post);
}

static void put_HTML_tags(struct membuffer *b, const struct dive &dive, const char *pre, const char *post)
{
	put_string(b, pre);

	if (dive.tags.empty())
		put_string(b, "[\"--\"");

	const char *separator = "[";
	for (const divetag *tag: dive.tags) {
		put_format(b, "%s\"", separator);
		separator = ", ";
		put_HTML_quoted(b, tag->name.c_str());
		put_string(b, "\"");
	}
	put_string(b, "]");
	put_string(b, post);
}

/* if exporting list_only mode, we neglect exporting the samples, bookmarks and cylinders */
static void write_one_dive(struct membuffer *b, const struct dive &dive, const char *photos_dir, int *dive_no, bool list_only)
{
	put_string(b, "{");
	put_format(b, "\"number\":%d,", *dive_no);
	put_format(b, "\"subsurface_number\":%d,", dive.number);
	put_HTML_date(b, dive, "\"date\":\"", "\",");
	put_HTML_time(b, dive, "\"time\":\"", "\",");
	write_attribute(b, "location", dive.get_location().c_str(), ", ");
	put_HTML_coordinates(b, dive);
	put_format(b, "\"rating\":%d,", dive.rating);
	put_format(b, "\"visibility\":%d,", dive.visibility);
	put_format(b, "\"current\":%d,", dive.current);
	put_format(b, "\"wavesize\":%d,", dive.wavesize);
	put_format(b, "\"surge\":%d,", dive.surge);
	put_format(b, "\"chill\":%d,", dive.chill);
	put_format(b, "\"dive_duration\":\"%u:%02u min\",",
		   FRACTION_TUPLE(dive.duration.seconds, 60));
	put_string(b, "\"temperature\":{");
	put_HTML_airtemp(b, dive, "\"air\":\"", "\",");
	put_HTML_watertemp(b, dive, "\"water\":\"", "\"");
	put_string(b, "	},");
	write_attribute(b, "buddy", dive.buddy.c_str(), ", ");
	write_attribute(b, "diveguide", dive.diveguide.c_str(), ", ");
	write_attribute(b, "suit", dive.suit.c_str(), ", ");
	put_HTML_tags(b, dive, "\"tags\":", ",");
	if (!list_only) {
		put_cylinder_HTML(b, dive);
		put_weightsystem_HTML(b, dive);
		put_HTML_samples(b, dive);
		put_HTML_bookmarks(b, dive);
		write_dive_status(b, dive);
		if (photos_dir && strcmp(photos_dir, ""))
			save_photos(b, photos_dir, dive);
		write_divecomputers(b, dive);
	}
	put_HTML_notes(b, dive, "\"notes\":\"", "\"");
	put_string(b, "}\n");
	(*dive_no)++;
}

static void write_no_trip(struct membuffer *b, int *dive_no, bool selected_only, const char *photos_dir, const bool list_only, char *sep)
{
	const char *separator = "";
	bool found_sel_dive = 0;

	for (auto &dive: divelog.dives) {
		// write dive if it doesn't belong to any trip and the dive is selected
		// or we are in exporting all dives mode.
		if (!dive->divetrip && (dive->selected || !selected_only)) {
			if (!found_sel_dive) {
				put_format(b, "%c{", *sep);
				(*sep) = ',';
				put_format(b, "\"name\":\"Other\",");
				put_format(b, "\"dives\":[");
				found_sel_dive = 1;
			}
			put_string(b, separator);
			separator = ", ";
			write_one_dive(b, *dive, photos_dir, dive_no, list_only);
		}
	}
	if (found_sel_dive)
		put_format(b, "]}\n\n");
}

static void write_trip(struct membuffer *b, dive_trip *trip, int *dive_no, bool selected_only, const char *photos_dir, const bool list_only, char *sep)
{
	const char *separator = "";
	bool found_sel_dive = 0;

	for (auto dive: trip->dives) {
		if (!dive->selected && selected_only)
			continue;

		// save trip if found at least one selected dive.
		if (!found_sel_dive) {
			found_sel_dive = 1;
			put_format(b, "%c {", *sep);
			(*sep) = ',';
			write_attribute(b, "name", trip->location.c_str(), ", ");
			put_format(b, "\"dives\":[");
		}
		put_string(b, separator);
		separator = ", ";
		write_one_dive(b, *dive, photos_dir, dive_no, list_only);
	}

	// close the trip object if contain dives.
	if (found_sel_dive)
		put_format(b, "]}\n\n");
}

static void write_trips(struct membuffer *b, const char *photos_dir, bool selected_only, const bool list_only)
{
	int dive_no = 0;
	char sep_ = ' ';
	char *sep = &sep_;

	put_string(b, "[");
	for (auto &trip: divelog.trips)
		trip->saved = 0;

	for (auto &dive: divelog.dives) {
		dive_trip *trip = dive->divetrip;

		/*Continue if the dive have no trips or we have seen this trip before*/
		if (!trip || trip->saved)
			continue;

		/* We haven't seen this trip before - save it and all dives */
		trip->saved = 1;
		write_trip(b, trip, &dive_no, selected_only, photos_dir, list_only, sep);
	}

	/*Save all remaining trips into Others*/
	write_no_trip(b, &dive_no, selected_only, photos_dir, list_only, sep);
	put_string(b, "]");
}

void export_list_as_JS(struct membuffer *b, const char *photos_dir, bool selected_only, const bool list_only)
{
	put_string(b, "trips=");
	write_trips(b, photos_dir, selected_only, list_only);
}

void export_JS(const char *file_name, const char *photos_dir, const bool selected_only, const bool list_only)
{
	FILE *f;

	membuffer buf;
	export_list_as_JS(&buf, photos_dir, selected_only, list_only);

	f = subsurface_fopen(file_name, "w+");
	if (!f) {
		report_error(translate("gettextFromC", "Can't open file %s"), file_name);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
}

void export_list_as_JSON(struct membuffer *b, const bool selected_only, const bool list_only)
{
	write_trips(b, /* photos_dir */ nullptr, selected_only, list_only);
}

void export_JSON(const char *file_name, const bool selected_only, const bool list_only)
{
	FILE *f;

	membuffer buf;
	export_list_as_JSON(&buf, selected_only, list_only);

	f = subsurface_fopen(file_name, "w+");
	if (!f) {
		report_error(translate("gettextFromC", "Can't open file %s"), file_name);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
}

void export_translation(const char *file_name)
{
	FILE *f;

	membuffer buf;

	//export translated words here
	put_format(&buf, "translate={");

	//Dive list view
	write_attribute(&buf, "Number", translate("gettextFromC", "Number"), ", ");
	write_attribute(&buf, "Date", translate("gettextFromC", "Date"), ", ");
	write_attribute(&buf, "Time", translate("gettextFromC", "Time"), ", ");
	write_attribute(&buf, "Location", translate("gettextFromC", "Location"), ", ");
	write_attribute(&buf, "Air_Temp", translate("gettextFromC", "Air temp."), ", ");
	write_attribute(&buf, "Water_Temp", translate("gettextFromC", "Water temp."), ", ");
	write_attribute(&buf, "dives", translate("gettextFromC", "Dives"), ", ");
	write_attribute(&buf, "Expand_All", translate("gettextFromC", "Expand all"), ", ");
	write_attribute(&buf, "Collapse_All", translate("gettextFromC", "Collapse all"), ", ");
	write_attribute(&buf, "trips", translate("gettextFromC", "Trips"), ", ");
	write_attribute(&buf, "Statistics", translate("gettextFromC", "Statistics"), ", ");
	write_attribute(&buf, "Advanced_Search", translate("gettextFromC", "Advanced search"), ", ");

	//Dive expanded view

	write_attribute(&buf, "Rating", translate("gettextFromC", "Rating"), ", ");
	write_attribute(&buf, "WaveSize", translate("gettextFromC", "WaveSize"), ", ");
	write_attribute(&buf, "Visibility", translate("gettextFromC", "Visibility"), ", ");
	write_attribute(&buf, "Current", translate("gettextFromC", "Current"), ", ");
	write_attribute(&buf, "Surge", translate("gettextFromC", "Surge"), ", ");
	write_attribute(&buf, "Chill", translate("gettextFromC", "Chill"), ", ");
	write_attribute(&buf, "Duration", translate("gettextFromC", "Duration"), ", ");
	write_attribute(&buf, "DiveGuide", translate("gettextFromC", "Diveguide"), ", ");
	write_attribute(&buf, "DiveMaster", translate("gettextFromC", "Divemaster"), ", ");
	write_attribute(&buf, "Buddy", translate("gettextFromC", "Buddy"), ", ");
	write_attribute(&buf, "Suit", translate("gettextFromC", "Suit"), ", ");
	write_attribute(&buf, "Tags", translate("gettextFromC", "Tags"), ", ");
	write_attribute(&buf, "Notes", translate("gettextFromC", "Notes"), ", ");
	write_attribute(&buf, "Show_more_details", translate("gettextFromC", "Show more details"), ", ");

	//Yearly statistics view
	write_attribute(&buf, "Yearly_statistics", translate("gettextFromC", "Yearly statistics"), ", ");
	write_attribute(&buf, "Year", translate("gettextFromC", "Year"), ", ");
	write_attribute(&buf, "Total_Time", translate("gettextFromC", "Total time"), ", ");
	write_attribute(&buf, "Average_Time", translate("gettextFromC", "Average time"), ", ");
	write_attribute(&buf, "Shortest_Time", translate("gettextFromC", "Shortest time"), ", ");
	write_attribute(&buf, "Longest_Time", translate("gettextFromC", "Longest time"), ", ");
	write_attribute(&buf, "Average_Depth", translate("gettextFromC", "Average depth"), ", ");
	write_attribute(&buf, "Min_Depth", translate("gettextFromC", "Min. depth"), ", ");
	write_attribute(&buf, "Max_Depth", translate("gettextFromC", "Max. depth"), ", ");
	write_attribute(&buf, "Average_SAC", translate("gettextFromC", "Average SAC"), ", ");
	write_attribute(&buf, "Min_SAC", translate("gettextFromC", "Min. SAC"), ", ");
	write_attribute(&buf, "Max_SAC", translate("gettextFromC", "Max. SAC"), ", ");
	write_attribute(&buf, "Average_Temp", translate("gettextFromC", "Average temp."), ", ");
	write_attribute(&buf, "Min_Temp", translate("gettextFromC", "Min. temp."), ", ");
	write_attribute(&buf, "Max_Temp", translate("gettextFromC", "Max. temp."), ", ");
	write_attribute(&buf, "Back_to_List", translate("gettextFromC", "Back to list"), ", ");

	//dive detailed view
	write_attribute(&buf, "Dive_No", translate("gettextFromC", "Dive #"), ", ");
	write_attribute(&buf, "Dive_profile", translate("gettextFromC", "Dive profile"), ", ");
	write_attribute(&buf, "Dive_information", translate("gettextFromC", "Dive information"), ", ");
	write_attribute(&buf, "Dive_equipment", translate("gettextFromC", "Dive equipment"), ", ");
	write_attribute(&buf, "Type", translate("gettextFromC", "Type"), ", ");
	write_attribute(&buf, "Size", translate("gettextFromC", "Size"), ", ");
	write_attribute(&buf, "Work_Pressure", translate("gettextFromC", "Work pressure"), ", ");
	write_attribute(&buf, "Start_Pressure", translate("gettextFromC", "Start pressure"), ", ");
	write_attribute(&buf, "End_Pressure", translate("gettextFromC", "End pressure"), ", ");
	write_attribute(&buf, "Gas", translate("gettextFromC", "Gas"), ", ");
	write_attribute(&buf, "Weight", translate("gettextFromC", "Weight"), ", ");
	write_attribute(&buf, "Type", translate("gettextFromC", "Type"), ", ");
	write_attribute(&buf, "Events", translate("gettextFromC", "Events"), ", ");
	write_attribute(&buf, "Name", translate("gettextFromC", "Name"), ", ");
	write_attribute(&buf, "Value", translate("gettextFromC", "Value"), ", ");
	write_attribute(&buf, "Coordinates", translate("gettextFromC", "Coordinates"), ", ");
	write_attribute(&buf, "Dive_Status", translate("gettextFromC", "Dive status"), " ");

	put_format(&buf, "}");

	f = subsurface_fopen(file_name, "w+");
	if (!f) {
		report_error(translate("gettextFromC", "Can't open file %s"), file_name);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
}
