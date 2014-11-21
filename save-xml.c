#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "dive.h"
#include "device.h"
#include "membuffer.h"

/*
 * We're outputting utf8 in xml.
 * We need to quote the characters <, >, &.
 *
 * Technically I don't think we'd necessarily need to quote the control
 * characters, but at least libxml2 doesn't like them. It doesn't even
 * allow them quoted. So we just skip them and replace them with '?'.
 *
 * If we do this for attributes, we need to quote the quotes we use too.
 */
static void quote(struct membuffer *b, const char *text, int is_attribute)
{
	int is_html = 0;
	put_quoted(b, text, is_attribute, is_html);
}

static void show_utf8(struct membuffer *b, const char *text, const char *pre, const char *post, int is_attribute)
{
	int len;
	char *cleaned;

	if (!text)
		return;
	/* remove leading and trailing space */
	/* We need to combine isascii() with isspace(),
	 * because we can only trust isspace() with 7-bit ascii,
	 * on windows for example */
	while (isascii(*text) && isspace(*text))
		text++;
	len = strlen(text);
	if (!len)
		return;
	while (len && isascii(text[len - 1]) && isspace(text[len - 1]))
		len--;
	/* strndup would be easier, but that doesn't appear to exist on Windows / Mac */
	cleaned = strdup(text);
	cleaned[len] = '\0';
	put_string(b, pre);
	quote(b, cleaned, is_attribute);
	put_string(b, post);
	free(cleaned);
}

static void save_depths(struct membuffer *b, struct divecomputer *dc)
{
	/* What's the point of this dive entry again? */
	if (!dc->maxdepth.mm && !dc->meandepth.mm)
		return;

	put_string(b, "  <depth");
	put_depth(b, dc->maxdepth, " max='", " m'");
	put_depth(b, dc->meandepth, " mean='", " m'");
	put_string(b, " />\n");
}

static void save_dive_temperature(struct membuffer *b, struct dive *dive)
{
	if (!dive->airtemp.mkelvin && !dive->watertemp.mkelvin)
		return;
	if (dive->airtemp.mkelvin == dc_airtemp(&dive->dc) && dive->watertemp.mkelvin == dc_watertemp(&dive->dc))
		return;

	put_string(b, "  <divetemperature");
	if (dive->airtemp.mkelvin != dc_airtemp(&dive->dc))
		put_temperature(b, dive->airtemp, " air='", " C'");
	if (dive->watertemp.mkelvin != dc_watertemp(&dive->dc))
		put_temperature(b, dive->watertemp, " water='", " C'");
	put_string(b, "/>\n");
}

static void save_temperatures(struct membuffer *b, struct divecomputer *dc)
{
	if (!dc->airtemp.mkelvin && !dc->watertemp.mkelvin)
		return;
	put_string(b, "  <temperature");
	put_temperature(b, dc->airtemp, " air='", " C'");
	put_temperature(b, dc->watertemp, " water='", " C'");
	put_string(b, " />\n");
}

static void save_airpressure(struct membuffer *b, struct divecomputer *dc)
{
	if (!dc->surface_pressure.mbar)
		return;
	put_string(b, "  <surface");
	put_pressure(b, dc->surface_pressure, " pressure='", " bar'");
	put_string(b, " />\n");
}

static void save_salinity(struct membuffer *b, struct divecomputer *dc)
{
	/* only save if we have a value that isn't the default of sea water */
	if (!dc->salinity || dc->salinity == SEAWATER_SALINITY)
		return;
	put_string(b, "  <water");
	put_salinity(b, dc->salinity, " salinity='", " g/l'");
	put_string(b, " />\n");
}

static void show_location(struct membuffer *b, struct dive *dive)
{
	degrees_t latitude = dive->latitude;
	degrees_t longitude = dive->longitude;

	/* Should we write a location tag at all? */
	if (!(latitude.udeg || longitude.udeg) && !dive->location)
		return;

	put_string(b, "  <location");

	/*
	 * Ok, theoretically I guess you could dive at
	 * exactly 0,0. But we don't support that. So
	 * if you do, just fudge it a bit, and say that
	 * you dove a few meters away.
	 */
	if (latitude.udeg || longitude.udeg) {
		put_degrees(b, latitude, " gps='", " ");
		put_degrees(b, longitude, "", "'");
	}

	/* Do we have a location name or should we write a empty tag? */
	if (dive->location && dive->location[0] != '\0')
		show_utf8(b, dive->location, ">", "</location>\n", 0);
	else
		put_string(b, "/>\n");
}

static void save_overview(struct membuffer *b, struct dive *dive)
{
	show_location(b, dive);
	show_utf8(b, dive->divemaster, "  <divemaster>", "</divemaster>\n", 0);
	show_utf8(b, dive->buddy, "  <buddy>", "</buddy>\n", 0);
	show_utf8(b, dive->notes, "  <notes>", "</notes>\n", 0);
	show_utf8(b, dive->suit, "  <suit>", "</suit>\n", 0);
}

static void put_gasmix(struct membuffer *b, struct gasmix *mix)
{
	int o2 = mix->o2.permille;
	int he = mix->he.permille;

	if (o2) {
		put_format(b, " o2='%u.%u%%'", FRACTION(o2, 10));
		if (he)
			put_format(b, " he='%u.%u%%'", FRACTION(he, 10));
	}
}

static void save_cylinder_info(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_cylinders(dive);

	for (i = 0; i < nr; i++) {
		cylinder_t *cylinder = dive->cylinder + i;
		int volume = cylinder->type.size.mliter;
		const char *description = cylinder->type.description;

		put_format(b, "  <cylinder");
		if (volume)
			put_milli(b, " size='", volume, " l'");
		put_pressure(b, cylinder->type.workingpressure, " workpressure='", " bar'");
		show_utf8(b, description, " description='", "'", 1);
		put_gasmix(b, &cylinder->gasmix);
		put_pressure(b, cylinder->start, " start='", " bar'");
		put_pressure(b, cylinder->end, " end='", " bar'");
		if (cylinder->cylinder_use != OC_GAS)
			show_utf8(b, cylinderuse_text[cylinder->cylinder_use], " use='", "'", 1);
		put_format(b, " />\n");
	}
}

static void save_weightsystem_info(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_weightsystems(dive);

	for (i = 0; i < nr; i++) {
		weightsystem_t *ws = dive->weightsystem + i;
		int grams = ws->weight.grams;
		const char *description = ws->description;

		put_format(b, "  <weightsystem");
		put_milli(b, " weight='", grams, " kg'");
		show_utf8(b, description, " description='", "'", 1);
		put_format(b, " />\n");
	}
}

static void show_index(struct membuffer *b, int value, const char *pre, const char *post)
{
	if (value)
		put_format(b, " %s%d%s", pre, value, post);
}

static void save_sample(struct membuffer *b, struct sample *sample, struct sample *old)
{
	put_format(b, "  <sample time='%u:%02u min'", FRACTION(sample->time.seconds, 60));
	put_milli(b, " depth='", sample->depth.mm, " m'");
	put_temperature(b, sample->temperature, " temp='", " C'");
	put_pressure(b, sample->cylinderpressure, " pressure='", " bar'");
	put_pressure(b, sample->o2cylinderpressure, " o2pressure='", " bar'");

	/*
	 * We only show sensor information for samples with pressure, and only if it
	 * changed from the previous sensor we showed.
	 */
	if (sample->cylinderpressure.mbar && sample->sensor != old->sensor) {
		put_format(b, " sensor='%d'", sample->sensor);
		old->sensor = sample->sensor;
	}

	/* the deco/ndl values are stored whenever they change */
	if (sample->ndl.seconds != old->ndl.seconds) {
		put_format(b, " ndl='%u:%02u min'", FRACTION(sample->ndl.seconds, 60));
		old->ndl = sample->ndl;
	}
	if (sample->tts.seconds != old->tts.seconds) {
		put_format(b, " tts='%u:%02u min'", FRACTION(sample->tts.seconds, 60));
		old->tts = sample->tts;
	}
	if (sample->in_deco != old->in_deco) {
		put_format(b, " in_deco='%d'", sample->in_deco ? 1 : 0);
		old->in_deco = sample->in_deco;
	}
	if (sample->stoptime.seconds != old->stoptime.seconds) {
		put_format(b, " stoptime='%u:%02u min'", FRACTION(sample->stoptime.seconds, 60));
		old->stoptime = sample->stoptime;
	}

	if (sample->stopdepth.mm != old->stopdepth.mm) {
		put_milli(b, " stopdepth='", sample->stopdepth.mm, " m'");
		old->stopdepth = sample->stopdepth;
	}

	if (sample->cns != old->cns) {
		put_format(b, " cns='%u%%'", sample->cns);
		old->cns = sample->cns;
	}

	if ((sample->o2sensor[0].mbar) && (sample->o2sensor[0].mbar != old->o2sensor[0].mbar)) {
		put_milli(b, " sensor1='", sample->o2sensor[0].mbar, " bar'");
		old->o2sensor[0] = sample->o2sensor[0];
	}

	if ((sample->o2sensor[1].mbar) && (sample->o2sensor[1].mbar != old->o2sensor[1].mbar)) {
		put_milli(b, " sensor2='", sample->o2sensor[1].mbar, " bar'");
		old->o2sensor[1] = sample->o2sensor[1];
	}

	if ((sample->o2sensor[2].mbar) && (sample->o2sensor[2].mbar != old->o2sensor[2].mbar)) {
		put_milli(b, " sensor3='", sample->o2sensor[2].mbar, " bar'");
		old->o2sensor[2] = sample->o2sensor[2];
	}

	if (sample->setpoint.mbar != old->setpoint.mbar) {
		put_milli(b, " po2='", sample->setpoint.mbar, " bar'");
		old->setpoint = sample->setpoint;
	}
	show_index(b, sample->heartbeat, "heartbeat='", "'");
	show_index(b, sample->bearing.degrees, "bearing='", "'");
	put_format(b, " />\n");
}

static void save_one_event(struct membuffer *b, struct event *ev)
{
	put_format(b, "  <event time='%d:%02d min'", FRACTION(ev->time.seconds, 60));
	show_index(b, ev->type, "type='", "'");
	show_index(b, ev->flags, "flags='", "'");
	show_index(b, ev->value, "value='", "'");
	show_utf8(b, ev->name, " name='", "'", 1);
	if (event_is_gaschange(ev)) {
		if (ev->gas.index >= 0) {
			show_index(b, ev->gas.index, "cylinder='", "'");
			put_gasmix(b, &ev->gas.mix);
		} else if (!event_gasmix_redundant(ev))
			put_gasmix(b, &ev->gas.mix);
	}
	put_format(b, " />\n");
}


static void save_events(struct membuffer *b, struct event *ev)
{
	while (ev) {
		save_one_event(b, ev);
		ev = ev->next;
	}
}

static void save_tags(struct membuffer *b, struct tag_entry *entry)
{
	if (entry) {
		const char *sep = " tags='";
		do {
			struct divetag *tag = entry->tag;
			put_string(b, sep);
			/* If the tag has been translated, write the source to the xml file */
			quote(b, tag->source ?: tag->name, 0);
			sep = ", ";
		} while ((entry = entry->next) != NULL);
		put_string(b, "'");
	}
}

static void save_extra_data(struct membuffer *b, struct extra_data *ed)
{
	while (ed) {
		if (ed->key && ed->value) {
			put_string(b, "  <extradata");
			show_utf8(b, ed->key, " key='", "'", 1);
			show_utf8(b, ed->value, " value='", "'", 1);
			put_string(b, " />\n");
		}
		ed = ed->next;
	}
}

static void show_date(struct membuffer *b, timestamp_t when)
{
	struct tm tm;

	utc_mkdate(when, &tm);

	put_format(b, " date='%04u-%02u-%02u'",
		   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	put_format(b, " time='%02u:%02u:%02u'",
		   tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void save_samples(struct membuffer *b, int nr, struct sample *s)
{
	struct sample dummy = {};

	while (--nr >= 0) {
		save_sample(b, s, &dummy);
		s++;
	}
}

static void save_dc(struct membuffer *b, struct dive *dive, struct divecomputer *dc)
{
	put_format(b, "  <divecomputer");
	show_utf8(b, dc->model, " model='", "'", 1);
	if (dc->deviceid)
		put_format(b, " deviceid='%08x'", dc->deviceid);
	if (dc->diveid)
		put_format(b, " diveid='%08x'", dc->diveid);
	if (dc->when && dc->when != dive->when)
		show_date(b, dc->when);
	if (dc->duration.seconds && dc->duration.seconds != dive->dc.duration.seconds)
		put_duration(b, dc->duration, " duration='", " min'");
	if (dc->dctype != OC) {
		for (enum dive_comp_type i = 0; i < NUM_DC_TYPE; i++)
			if (dc->dctype == i)
				show_utf8(b, dctype_text[i], " dctype='", "'", 1);
		if (dc->no_o2sensors)
			put_format(b," no_o2sensors='%d'", dc->no_o2sensors);
	}
	put_format(b, ">\n");
	save_depths(b, dc);
	save_temperatures(b, dc);
	save_airpressure(b, dc);
	save_salinity(b, dc);
	put_duration(b, dc->surfacetime, "  <surfacetime>", " min</surfacetime>\n");
	save_extra_data(b, dc->extra_data);
	save_events(b, dc->events);
	save_samples(b, dc->samples, dc->sample);

	put_format(b, "  </divecomputer>\n");
}

static void save_picture(struct membuffer *b, struct picture *pic)
{
	put_string(b, "  <picture filename='");
	put_string(b, pic->filename);
	put_string(b, "'");
	if (pic->offset.seconds) {
		int offset = pic->offset.seconds;
		char sign = '+';
		if (offset < 0) {
			sign = '-';
			offset = -offset;
		}
		put_format(b, " offset='%c%u:%02u min'", sign, FRACTION(offset, 60));
	}
	if (pic->latitude.udeg || pic->longitude.udeg) {
		put_degrees(b, pic->latitude, " gps='", " ");
		put_degrees(b, pic->longitude, "", "'");
	}
	put_string(b, "/>\n");
}

void save_one_dive(struct membuffer *b, struct dive *dive)
{
	struct divecomputer *dc;

	put_string(b, "<dive");
	if (dive->number)
		put_format(b, " number='%d'", dive->number);
	if (dive->tripflag == NO_TRIP)
		put_format(b, " tripflag='NOTRIP'");
	if (dive->rating)
		put_format(b, " rating='%d'", dive->rating);
	if (dive->visibility)
		put_format(b, " visibility='%d'", dive->visibility);
	save_tags(b, dive->tag_list);

	show_date(b, dive->when);
	put_format(b, " duration='%u:%02u min'>\n",
		   FRACTION(dive->dc.duration.seconds, 60));
	save_overview(b, dive);
	save_cylinder_info(b, dive);
	save_weightsystem_info(b, dive);
	save_dive_temperature(b, dive);
	/* Save the dive computer data */
	for_each_dc(dive, dc)
		save_dc(b, dive, dc);
	FOR_EACH_PICTURE(dive)
		save_picture(b, picture);
	put_format(b, "</dive>\n");
}

int save_dive(FILE *f, struct dive *dive)
{
	struct membuffer buf = { 0 };

	save_one_dive(&buf, dive);
	flush_buffer(&buf, f);
	/* Error handling? */
	return 0;
}

static void save_trip(struct membuffer *b, dive_trip_t *trip)
{
	int i;
	struct dive *dive;

	put_format(b, "<trip");
	show_date(b, trip->when);
	show_utf8(b, trip->location, " location=\'", "\'", 1);
	put_format(b, ">\n");
	show_utf8(b, trip->notes, "<notes>", "</notes>\n", 0);

	/*
	 * Incredibly cheesy: we want to save the dives sorted, and they
	 * are sorted in the dive array.. So instead of using the dive
	 * list in the trip, we just traverse the global dive array and
	 * check the divetrip pointer..
	 */
	for_each_dive(i, dive) {
		if (dive->divetrip == trip)
			save_one_dive(b, dive);
	}

	put_format(b, "</trip>\n");
}

static void save_one_device(void *_f, const char *model, uint32_t deviceid,
			    const char *nickname, const char *serial_nr, const char *firmware)
{
	struct membuffer *b = _f;

	/* Nicknames that are empty or the same as the device model are not interesting */
	if (nickname) {
		if (!*nickname || !strcmp(model, nickname))
			nickname = NULL;
	}

	/* Serial numbers that are empty are not interesting */
	if (serial_nr && !*serial_nr)
		serial_nr = NULL;

	/* Firmware strings that are empty are not interesting */
	if (firmware && !*firmware)
		firmware = NULL;

	/* Do we have anything interesting about this dive computer to save? */
	if (!serial_nr && !nickname && !firmware)
		return;

	put_format(b, "<divecomputerid");
	show_utf8(b, model, " model='", "'", 1);
	put_format(b, " deviceid='%08x'", deviceid);
	show_utf8(b, serial_nr, " serial='", "'", 1);
	show_utf8(b, firmware, " firmware='", "'", 1);
	show_utf8(b, nickname, " nickname='", "'", 1);
	put_format(b, "/>\n");
}

#define VERSION 2

int save_dives(const char *filename)
{
	return save_dives_logic(filename, false);
}

void save_dives_buffer(struct membuffer *b, const bool select_only)
{
	int i;
	struct dive *dive;
	dive_trip_t *trip;

	put_format(b, "<divelog program='subsurface' version='%d'>\n<settings>\n", VERSION);

	if (prefs.save_userid_local)
		put_format(b, "  <userid>%30s</userid>\n", prefs.userid);

	/* save the dive computer nicknames, if any */
	call_for_each_dc(b, save_one_device);
	if (autogroup)
		put_format(b, "  <autogroup state='1' />\n");
	put_format(b, "</settings>\n<dives>\n");

	for (trip = dive_trip_list; trip != NULL; trip = trip->next)
		trip->index = 0;

	/* save the dives */
	for_each_dive(i, dive) {
		if (select_only) {

			if (!dive->selected)
				continue;
			save_one_dive(b, dive);

		} else {
			trip = dive->divetrip;

			/* Bare dive without a trip? */
			if (!trip) {
				save_one_dive(b, dive);
				continue;
			}

			/* Have we already seen this trip (and thus saved this dive?) */
			if (trip->index)
				continue;

			/* We haven't seen this trip before - save it and all dives */
			trip->index = 1;
			save_trip(b, trip);
		}
	}
	put_format(b, "</dives>\n</divelog>\n");
}

static void save_backup(const char *name, const char *ext, const char *new_ext)
{
	int len = strlen(name);
	int a = strlen(ext), b = strlen(new_ext);
	char *newname;

	/* len up to and including the final '.' */
	len -= a;
	if (len <= 1)
		return;
	if (name[len - 1] != '.')
		return;
	/* msvc doesn't have strncasecmp, has _strnicmp instead - crazy */
	if (strncasecmp(name + len, ext, a))
		return;

	newname = malloc(len + b + 1);
	if (!newname)
		return;

	memcpy(newname, name, len);
	memcpy(newname + len, new_ext, b + 1);

	/*
	 * Ignore errors. Maybe we can't create the backup file,
	 * maybe no old file existed.  Regardless, we'll write the
	 * new file.
	 */
	(void) subsurface_rename(name, newname);
	free(newname);
}

static void try_to_backup(const char *filename)
{
	char extension[][5] = { "xml", "ssrf", "" };
	int i = 0;
	int flen = strlen(filename);

	/* Maybe we might want to make this configurable? */
	while (extension[i][0] != '\0') {
		int elen = strlen(extension[i]);
		if (strcasecmp(filename + flen - elen, extension[i]) == 0) {
			save_backup(filename, extension[i], "bak");
			break;
		}
		i++;
	}
}

int save_dives_logic(const char *filename, const bool select_only)
{
	struct membuffer buf = { 0 };
	FILE *f;
	void *git;
	const char *branch;
	int error;

	git = is_git_repository(filename, &branch);
	if (git)
		return git_save_dives(git, branch, select_only);

	try_to_backup(filename);

	save_dives_buffer(&buf, select_only);

	error = -1;
	f = subsurface_fopen(filename, "w");
	if (f) {
		flush_buffer(&buf, f);
		error = fclose(f);
	}
	if (error)
		report_error("Save failed (%s)", strerror(errno));

	free_buffer(&buf);
	return error;
}

int export_dives_xslt(const char *filename, const bool selected, const char *export_xslt)
{
	FILE *f;
	struct membuffer buf = { 0 };
	xmlDoc *doc;
	xsltStylesheetPtr xslt = NULL;
	xmlDoc *transformed;
	int res = 0;

	if (!filename)
		return report_error("No filename for export");

	/* Save XML to file and convert it into a memory buffer */
	save_dives_buffer(&buf, selected);

	/*
	 * Parse the memory buffer into XML document and
	 * transform it to selected export format, finally dumping
	 * the XML into a character buffer.
	 */
	doc = xmlReadMemory(buf.buffer, buf.len, "divelog", NULL, 0);
	free_buffer(&buf);
	if (!doc)
		return report_error("Failed to read XML memory");

	/* Convert to export format */
	xslt = get_stylesheet(export_xslt);
	if (!xslt)
		return report_error("Failed to open export conversion stylesheet");

	transformed = xsltApplyStylesheet(xslt, doc, NULL);
	xmlFreeDoc(doc);

	/* Write the transformed export to file */
	f = subsurface_fopen(filename, "w");
	if (f) {
		xsltSaveResultToFile(f, transformed, xslt);
		fclose(f);
		/* Check write errors? */
	} else {
		res = report_error("Failed to open %s for writing (%s)", filename, strerror(errno));
	}
	xsltFreeStylesheet(xslt);
	xmlFreeDoc(transformed);

	return res;
}
