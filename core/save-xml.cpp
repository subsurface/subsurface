// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "device.h"
#include "dive.h"
#include "divelog.h"
#include "divesite.h"
#include "errorhelper.h"
#include "extradata.h"
#include "event.h"
#include "filterconstraint.h"
#include "filterpreset.h"
#include "format.h"
#include "sample.h"
#include "subsurface-string.h"
#include "subsurface-time.h"
#include "tanksensormapping.h"
#include "trip.h"
#include "file.h"
#include "membuffer.h"
#include "picture.h"
#include "git-access.h"
#include "qthelper.h"
#include "gettext.h"
#include "tag.h"
#include "version.h"
#include "xmlparams.h"

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
	std::string cleaned(text, len);
	put_string(b, pre);
	quote(b, cleaned.c_str(), is_attribute);
	put_string(b, post);
}

static void blankout(std::string &s)
{
	for(char &c: s) {
		switch (c) {
		case 'A'...'Z':
			c = 'X';
			break;
		case 'a'...'z':
			c = 'x';
			break;
		default:
			;
		}
	}
}

static void show_utf8_blanked(struct membuffer *b, const char *text, const char *pre, const char *post, int is_attribute, bool anonymize)
{
	if (!text)
		return;
	std::string copy(text);

	if (anonymize)
		blankout(copy);
	show_utf8(b, copy.c_str(), pre, post, is_attribute);
}

static void save_depths(struct membuffer *b, const struct divecomputer &dc)
{
	/* What's the point of this dive entry again? */
	if (!dc.maxdepth.mm && !dc.meandepth.mm)
		return;

	put_string(b, "  <depth");
	put_depth(b, dc.maxdepth, " max='", " m'");
	put_depth(b, dc.meandepth, " mean='", " m'");
	put_string(b, " />\n");
}

static void save_dive_temperature(struct membuffer *b, const struct dive &dive)
{
	if (!dive.airtemp.mkelvin && !dive.watertemp.mkelvin)
		return;
	if (dive.airtemp.mkelvin == dive.dc_airtemp().mkelvin && dive.watertemp.mkelvin == dive.dc_watertemp().mkelvin)
		return;

	put_string(b, "  <divetemperature");
	if (dive.airtemp.mkelvin != dive.dc_airtemp().mkelvin)
		put_temperature(b, dive.airtemp, " air='", " C'");
	if (dive.watertemp.mkelvin != dive.dc_watertemp().mkelvin)
		put_temperature(b, dive.watertemp, " water='", " C'");
	put_string(b, "/>\n");
}

static void save_temperatures(struct membuffer *b, const struct divecomputer &dc)
{
	if (!dc.airtemp.mkelvin && !dc.watertemp.mkelvin)
		return;
	put_string(b, "  <temperature");
	put_temperature(b, dc.airtemp, " air='", " C'");
	put_temperature(b, dc.watertemp, " water='", " C'");
	put_string(b, " />\n");
}

static void save_airpressure(struct membuffer *b, const struct divecomputer &dc)
{
	if (!dc.surface_pressure.mbar)
		return;
	put_string(b, "  <surface");
	put_pressure(b, dc.surface_pressure, " pressure='", " bar'");
	put_string(b, " />\n");
}

static void save_salinity(struct membuffer *b, const struct divecomputer &dc)
{
	if (!dc.salinity)
		return;
	put_string(b, "  <water");
	put_salinity(b, dc.salinity, " salinity='", " g/l'");
	put_string(b, " />\n");
}

static void save_overview(struct membuffer *b, const struct dive &dive, bool anonymize)
{
	show_utf8_blanked(b, dive.diveguide.c_str(), "  <divemaster>", "</divemaster>\n", 0, anonymize);
	show_utf8_blanked(b, dive.buddy.c_str(), "  <buddy>", "</buddy>\n", 0, anonymize);
	show_utf8_blanked(b, dive.notes.c_str(), "  <notes>", "</notes>\n", 0, anonymize);
	show_utf8_blanked(b, dive.suit.c_str(), "  <suit>", "</suit>\n", 0, anonymize);
}

static void put_gasmix(struct membuffer *b, struct gasmix mix)
{
	int o2 = mix.o2.permille;
	int he = mix.he.permille;

	if (o2) {
		put_format(b, " o2='%u.%u%%'", FRACTION_TUPLE(o2, 10));
		if (he)
			put_format(b, " he='%u.%u%%'", FRACTION_TUPLE(he, 10));
	}
}

static void save_cylinder_info(struct membuffer *b, const struct dive &dive)
{
	for (auto &cyl: dive.cylinders) {
		int volume = cyl.type.size.mliter;
		int use = cyl.cylinder_use;

		put_format(b, "  <cylinder");
		if (volume)
			put_milli(b, " size='", volume, " l'");
		put_pressure(b, cyl.type.workingpressure, " workpressure='", " bar'");
		show_utf8(b, cyl.type.description.c_str(), " description='", "'", 1);
		put_gasmix(b, cyl.gasmix);
		put_pressure(b, cyl.start, " start='", " bar'");
		put_pressure(b, cyl.end, " end='", " bar'");
		if (use > OC_GAS && use < NUM_GAS_USE)
			show_utf8(b, cylinderuse_text[use], " use='", "'", 1);
		if (cyl.depth.mm != 0)
			put_milli(b, " depth='", cyl.depth.mm, " m'");
		put_format(b, " />\n");
	}
}

static void save_weightsystem_info(struct membuffer *b, const struct dive &dive)
{
	for (auto &ws:  dive.weightsystems) {
		int grams = ws.weight.grams;

		put_format(b, "  <weightsystem");
		put_milli(b, " weight='", grams, " kg'");
		show_utf8(b, ws.description.c_str(), " description='", "'", 1);
		put_format(b, " />\n");
	}
}

static void show_integer(struct membuffer *b, int value, const char *pre, const char *post)
{
	put_format(b, " %s%d%s", pre, value, post);
}

static void show_index(struct membuffer *b, int value, const char *pre, const char *post)
{
	if (value)
		show_integer(b, value, pre, post);
}

static void save_sample(struct membuffer *b, const struct sample &sample, struct sample &old)
{
	int idx;

	put_format(b, "  <sample time='%u:%02u min'", FRACTION_TUPLE(sample.time.seconds, 60));
	put_milli(b, " depth='", sample.depth.mm, " m'");
	if (sample.temperature.mkelvin && sample.temperature.mkelvin != old.temperature.mkelvin) {
		put_temperature(b, sample.temperature, " temp='", " C'");
		old.temperature = sample.temperature;
	}

	/*
	 * We only show sensor information for samples with pressure, and only if it
	 * changed from the previous sensor we showed.
	 */
	for (idx = 0; idx < MAX_SENSORS; idx++) {
		pressure_t p = sample.pressure[idx];
		int sensor = sample.sensor[idx];

		if (sensor == NO_SENSOR || !p.mbar || (sensor == old.sensor[idx] && p.mbar == old.pressure[idx].mbar))
			continue;

		put_format(b, " pressure%d=", sensor);
		put_pressure(b, p, "'", " bar'");
		old.sensor[idx] = sensor;
		old.pressure[idx] = p;
	}

	/* the deco/ndl values are stored whenever they change */
	if (sample.ndl.seconds != old.ndl.seconds) {
		put_format(b, " ndl='%u:%02u min'", FRACTION_TUPLE(sample.ndl.seconds, 60));
		old.ndl = sample.ndl;
	}
	if (sample.tts.seconds != old.tts.seconds) {
		put_format(b, " tts='%u:%02u min'", FRACTION_TUPLE(sample.tts.seconds, 60));
		old.tts = sample.tts;
	}
	if (sample.rbt.seconds != old.rbt.seconds) {
		put_format(b, " rbt='%u:%02u min'", FRACTION_TUPLE(sample.rbt.seconds, 60));
		old.rbt = sample.rbt;
	}
	if (sample.in_deco != old.in_deco) {
		put_format(b, " in_deco='%d'", sample.in_deco ? 1 : 0);
		old.in_deco = sample.in_deco;
	}
	if (sample.stoptime.seconds != old.stoptime.seconds) {
		put_format(b, " stoptime='%u:%02u min'", FRACTION_TUPLE(sample.stoptime.seconds, 60));
		old.stoptime = sample.stoptime;
	}

	if (sample.stopdepth.mm != old.stopdepth.mm) {
		put_milli(b, " stopdepth='", sample.stopdepth.mm, " m'");
		old.stopdepth = sample.stopdepth;
	}

	if (sample.cns != old.cns) {
		put_format(b, " cns='%u%%'", sample.cns);
		old.cns = sample.cns;
	}

	if ((sample.o2sensor[0].mbar) && (sample.o2sensor[0].mbar != old.o2sensor[0].mbar)) {
		put_milli(b, " sensor1='", sample.o2sensor[0].mbar, " bar'");
		old.o2sensor[0] = sample.o2sensor[0];
	}

	if ((sample.o2sensor[1].mbar) && (sample.o2sensor[1].mbar != old.o2sensor[1].mbar)) {
		put_milli(b, " sensor2='", sample.o2sensor[1].mbar, " bar'");
		old.o2sensor[1] = sample.o2sensor[1];
	}

	if ((sample.o2sensor[2].mbar) && (sample.o2sensor[2].mbar != old.o2sensor[2].mbar)) {
		put_milli(b, " sensor3='", sample.o2sensor[2].mbar, " bar'");
		old.o2sensor[2] = sample.o2sensor[2];
	}

	if ((sample.o2sensor[3].mbar) && (sample.o2sensor[3].mbar != old.o2sensor[3].mbar)) {
		put_milli(b, " sensor4='", sample.o2sensor[3].mbar, " bar'");
		old.o2sensor[3] = sample.o2sensor[3];
	}

	if ((sample.o2sensor[4].mbar) && (sample.o2sensor[4].mbar != old.o2sensor[4].mbar)) {
		put_milli(b, " sensor5='", sample.o2sensor[4].mbar, " bar'");
		old.o2sensor[4] = sample.o2sensor[4];
	}

	if ((sample.o2sensor[5].mbar) && (sample.o2sensor[5].mbar != old.o2sensor[5].mbar)) {
		put_milli(b, " sensor6='", sample.o2sensor[5].mbar, " bar'");
		old.o2sensor[5] = sample.o2sensor[5];
	}

	if ((sample.o2sensor[6].mbar) && (sample.o2sensor[6].mbar != old.o2sensor[6].mbar)) {
		put_milli(b, " dc_supplied_ppo2='", sample.o2sensor[6].mbar, " bar'");
		old.o2sensor[6] = sample.o2sensor[6];
	}

	if (sample.setpoint.mbar != old.setpoint.mbar) {
		put_milli(b, " po2='", sample.setpoint.mbar, " bar'");
		old.setpoint = sample.setpoint;
	}
	if (sample.heartbeat != old.heartbeat) {
		show_index(b, sample.heartbeat, "heartbeat='", "'");
		old.heartbeat = sample.heartbeat;
	}
	if (sample.bearing.degrees != old.bearing.degrees) {
		show_index(b, sample.bearing.degrees, "bearing='", "'");
		old.bearing.degrees = sample.bearing.degrees;
	}
	put_format(b, " />\n");
}

static void save_one_event(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc, const struct event &ev)
{
	put_format(b, "  <event time='%d:%02d min'", FRACTION_TUPLE(ev.time.seconds, 60));
	show_index(b, ev.type, "type='", "'");
	show_index(b, ev.flags, "flags='", "'");
	if (ev.name == "modechange")
		show_utf8(b, divemode_text[ev.value], " divemode='", "'",1);
	else
		show_index(b, ev.value, "value='", "'");
	show_utf8(b, ev.name.c_str(), " name='", "'", 1);
	if (ev.is_gaschange()) {
		struct gasmix mix = dive.get_gasmix_from_event(ev, dc).first;
		if (ev.gas.index >= 0)
			show_integer(b, ev.gas.index, "cylinder='", "'");
		put_gasmix(b, mix);
	}
	put_format(b, " />\n");
}


static void save_events(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc)
{
	for (auto &ev: dc.events)
		save_one_event(b, dive, dc, ev);
}

static void save_tags(struct membuffer *b, const tag_list &tags)
{
	if (!tags.empty()) {
		const char *sep = " tags='";
		for (const divetag *tag: tags) {
			put_string(b, sep);
			/* If the tag has been translated, write the source to the xml file */
			quote(b, tag->source.empty() ? tag->name.c_str() : tag->source.c_str(), 1);
			sep = ", ";
		}
		put_string(b, "'");
	}
}

static void save_extra_data(struct membuffer *b, const struct divecomputer &dc)
{
	for (const auto &ed: dc.extra_data) {
		if (!ed.key.empty() && !ed.value.empty()) {
			put_string(b, "  <extradata");
			show_utf8(b, ed.key.c_str(), " key='", "'", 1);
			show_utf8(b, ed.value.c_str(), " value='", "'", 1);
			put_string(b, " />\n");
		}
	}
}

static void save_tank_sensor_mappings(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc)
{
	const auto mappings = get_tank_sensor_mappings_for_storage(dive, dc);
	for (const auto &mapping: mappings) {
		put_string(b, "  <tanksensormapping");
		show_integer(b, mapping.sensor_id, "sensorid='", "'");
		show_integer(b, mapping.cylinder_index, "cylinderindex='", "'");
		put_string(b, " />\n");
	}
}

static void show_date(struct membuffer *b, timestamp_t when)
{
	struct tm tm;

	utc_mkdate(when, &tm);

	put_format(b, " date='%04u-%02u-%02u'",
		   tm.tm_year, tm.tm_mon + 1, tm.tm_mday);
	if (tm.tm_hour || tm.tm_min || tm.tm_sec)
		put_format(b, " time='%02u:%02u:%02u'",
			   tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void save_samples(struct membuffer *b, const struct divecomputer &dc)
{
	struct sample dummy;
	for (const auto &s: dc.samples)
		save_sample(b, s, dummy);
}

static void save_dc(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc)
{
	put_format(b, "  <divecomputer");
	show_utf8(b, dc.model.c_str(), " model='", "'", 1);
	if (dc.last_manual_time.seconds)
		put_duration(b, dc.last_manual_time, " last-manual-time='", " min'");
	if (dc.deviceid)
		put_format(b, " deviceid='%08x'", dc.deviceid);
	if (dc.diveid)
		put_format(b, " diveid='%08x'", dc.diveid);
	if (dc.when && dc.when != dive.when)
		show_date(b, dc.when);
	if (dc.duration.seconds && dc.duration.seconds != dive.dcs[0].duration.seconds)
		put_duration(b, dc.duration, " duration='", " min'");
	if (dc.divemode != OC) {
		int i = (int)dc.divemode;
		if (i >= 0 && i < NUM_DIVEMODE)
			show_utf8(b, divemode_text[i], " dctype='", "'", 1);
		if (dc.no_o2sensors)
			put_format(b," no_o2sensors='%d'", dc.no_o2sensors);
	}
	put_format(b, ">\n");
	save_depths(b, dc);
	save_temperatures(b, dc);
	save_airpressure(b, dc);
	save_salinity(b, dc);
	put_duration(b, dc.surfacetime, "  <surfacetime>", " min</surfacetime>\n");
	save_extra_data(b, dc);
	save_tank_sensor_mappings(b, dive, dc);
	save_events(b, dive, dc);
	save_samples(b, dc);

	put_format(b, "  </divecomputer>\n");
}

static void save_picture(struct membuffer *b, const struct picture &pic)
{
	put_string(b, "  <picture filename='");
	put_quoted(b, pic.filename.c_str(), true, false);
	put_string(b, "'");
	if (pic.offset.seconds) {
		int offset = pic.offset.seconds;
		char sign = '+';
		if (offset < 0) {
			sign = '-';
			offset = -offset;
		}
		put_format(b, " offset='%c%u:%02u min'", sign, FRACTION_TUPLE(offset, 60));
	}
	put_location(b, &pic.location, " gps='","'");

	put_string(b, "/>\n");
}

void save_one_dive_to_mb(struct membuffer *b, const struct dive &dive, bool anonymize)
{
	pressure_t surface_pressure = dive.un_fixup_surface_pressure();

	put_string(b, "<dive");
	if (dive.number)
		put_format(b, " number='%d'", dive.number);
	if (dive.notrip)
		put_format(b, " tripflag='NOTRIP'");
	if (dive.rating)
		put_format(b, " rating='%d'", dive.rating);
	if (dive.visibility)
		put_format(b, " visibility='%d'", dive.visibility);
	if (dive.wavesize)
		put_format(b, " wavesize='%d'", dive.wavesize);
	if (dive.current)
		put_format(b, " current='%d'", dive.current);
	if (dive.surge)
		put_format(b, " surge='%d'", dive.surge);
	if (dive.chill)
		put_format(b, " chill='%d'", dive.chill);
	if (dive.invalid)
		put_format(b, " invalid='1'");

	// These three are calculated, and not read when loading.
	// But saving them into the XML is useful for data export.
	if (dive.sac > 100)
		put_format(b, " sac='%d.%03d l/min'", FRACTION_TUPLE(dive.sac, 1000));
	if (dive.otu)
		put_format(b, " otu='%d'", dive.otu);
	if (dive.maxcns)
		put_format(b, " cns='%d%%'", dive.maxcns);

	save_tags(b, dive.tags);
	if (dive.dive_site)
		put_format(b, " divesiteid='%8x'", dive.dive_site->uuid);
	if (dive.user_salinity)
		put_salinity(b, dive.user_salinity, " watersalinity='", " g/l'");
	show_date(b, dive.when);
	if (surface_pressure.mbar)
		put_pressure(b, surface_pressure, " airpressure='", " bar'");
	if (dive.dcs[0].duration.seconds > 0)
		put_format(b, " duration='%u:%02u min'>\n",
			   FRACTION_TUPLE(dive.dcs[0].duration.seconds, 60));
	else
		put_format(b, ">\n");
	save_overview(b, dive, anonymize);
	save_cylinder_info(b, dive);
	save_weightsystem_info(b, dive);
	save_dive_temperature(b, dive);
	/* Save the dive computer data */
	for (auto &dc: dive.dcs)
		save_dc(b, dive, dc);
	for (auto &picture: dive.pictures)
		save_picture(b, picture);
	put_format(b, "</dive>\n");
}

int save_dive(FILE *f, const struct dive &dive, bool anonymize)
{
	membuffer buf;

	save_one_dive_to_mb(&buf, dive, anonymize);
	flush_buffer(&buf, f);
	/* Error handling? */
	return 0;
}

static void save_trip(struct membuffer *b, dive_trip &trip, bool anonymize)
{
	put_format(b, "<trip");
	show_date(b, trip.date());
	show_utf8(b, trip.location.c_str(), " location=\'", "\'", 1);
	put_format(b, ">\n");
	show_utf8(b, trip.notes.c_str(), "<notes>", "</notes>\n", 0);

	/*
	 * Incredibly cheesy: we want to save the dives sorted, and they
	 * are sorted in the dive array.. So instead of using the dive
	 * list in the trip, we just traverse the global dive array and
	 * check the divetrip pointer..
	 */
	for (auto &dive: divelog.dives) {
		if (dive->divetrip == &trip)
			save_one_dive_to_mb(b, *dive, anonymize);
	}

	put_format(b, "</trip>\n");
}

static void save_one_device(struct membuffer *b, const struct device &d)
{
	/* Nicknames that are empty or the same as the device model are not interesting */
	if (d.nickName.empty() || d.serialNumber.empty() || d.model == d.nickName)
		return;

	put_format(b, "<divecomputerid");
	show_utf8(b, d.model.c_str(), " model='", "'", 1);
	put_format(b, " deviceid='%08x'", calculate_string_hash(d.serialNumber.c_str()));
	show_utf8(b, d.serialNumber.c_str(), " serial='", "'", 1);
	show_utf8(b, d.nickName.c_str(), " nickname='", "'", 1);
	put_format(b, "/>\n");
}

static void save_one_fingerprint(struct membuffer *b, const fingerprint_record &fp)
{
	put_format(b, "<fingerprint model='%08x' serial='%08x' deviceid='%08x' diveid='%08x' data='%s'/>\n",
		   fp.model, fp.serial, fp.fdeviceid, fp.fdiveid, fp.get_data().c_str());
}

int save_dives(const char *filename)
{
	return save_dives_logic(filename, false, false);
}

static void save_filter_presets(struct membuffer *b)
{
	if (divelog.filter_presets.empty())
		return;
	put_format(b, "<filterpresets>\n");
	for (auto &filter_preset: divelog.filter_presets) {
		put_format(b, " <filterpreset");
		show_utf8(b, filter_preset.name.c_str(), " name='", "'", 1);
		put_format(b, ">\n");

		std::string fulltext = filter_preset.fulltext_query();
		if (!fulltext.empty()) {
			const char *fulltext_mode = filter_preset.fulltext_mode();
			show_utf8(b, fulltext_mode, "  <fulltext mode='", "'>", 1);
			show_utf8(b, fulltext.c_str(), "", "</fulltext>\n", 0);
		}

		for (auto &constraint: filter_preset.data.constraints) {
			const char *type = filter_constraint_type_to_string(constraint.type);
			put_format(b, "  <constraint");
			show_utf8(b, type, " type='", "'", 1);
			if (filter_constraint_has_string_mode(constraint.type)) {
				const char *mode = filter_constraint_string_mode_to_string(constraint.string_mode);
				show_utf8(b, mode, " string_mode='", "'", 1);
			}
			if (filter_constraint_has_range_mode(constraint.type)) {
				const char *mode = filter_constraint_range_mode_to_string(constraint.range_mode);
				show_utf8(b, mode, " range_mode='", "'", 1);
			}
			if (constraint.negate)
				put_format(b, " negate='1'");
			put_format(b, ">");
			std::string data = filter_constraint_data_to_string(constraint);
			show_utf8(b, data.c_str(), "", "", 0);
			put_format(b, "</constraint>\n");
		}
		put_format(b, " </filterpreset>\n");
	}
	put_format(b, "</filterpresets>\n");
}

static void save_dives_buffer(struct membuffer *b, bool select_only, bool anonymize)
{
	put_format(b, "<divelog program='subsurface' version='%d'>\n<settings>\n", dataformat_version);

	/* save the dive computer nicknames, if any */
	for (auto &d: divelog.devices) {
		if (!select_only || device_used_by_selected_dive(d))
			save_one_device(b, d);
	}
	/* save the fingerprint data */
	for (auto &fp: fingerprints)
		save_one_fingerprint(b, fp);

	if (divelog.autogroup)
		put_format(b, "  <autogroup state='1' />\n");
	put_format(b, "</settings>\n");

	/* save the dive sites */
	put_format(b, "<divesites>\n");
	for (const auto &ds: divelog.sites) {
		/* Don't export empty dive sites */
		if (ds->is_empty())
			continue;
		/* Only write used dive sites when exporting selected dives */
		if (select_only && !ds->is_selected())
			continue;

		put_format(b, "<site uuid='%8x'", ds->uuid);
		show_utf8_blanked(b, ds->name.c_str(), " name='", "'", 1, anonymize);
		put_location(b, &ds->location, " gps='", "'");
		show_utf8_blanked(b, ds->description.c_str(), " description='", "'", 1, anonymize);
		put_format(b, ">\n");
		show_utf8_blanked(b, ds->notes.c_str(), "  <notes>", " </notes>\n", 0, anonymize);
		for (auto const &t: ds->taxonomy) {
			if (t.category != TC_NONE && !t.value.empty()) {
				put_format(b, "  <geo cat='%d'", t.category);
				put_format(b, " origin='%d'", t.origin);
				show_utf8_blanked(b, t.value.c_str(), " value='", "'", 1, anonymize);
				put_format(b, "/>\n");
			}
		}
		put_format(b, "</site>\n");
	}
	put_format(b, "</divesites>\n<dives>\n");
	for (auto &trip: divelog.trips)
		trip->saved = 0;

	/* save the filter presets */
	save_filter_presets(b);

	/* save the dives */
	for (auto &dive: divelog.dives) {
		if (select_only) {
			if (!dive->selected)
				continue;
			save_one_dive_to_mb(b, *dive, anonymize);
		} else {
			dive_trip *trip = dive->divetrip;

			/* Bare dive without a trip? */
			if (!trip) {
				save_one_dive_to_mb(b, *dive, anonymize);
				continue;
			}

			/* Have we already seen this trip (and thus saved this dive?) */
			if (trip->saved)
				continue;

			/* We haven't seen this trip before - save it and all dives */
			trip->saved = 1;
			save_trip(b, *trip, anonymize);
		}
	}
	put_format(b, "</dives>\n</divelog>\n");
}

static void save_backup(const char *name, const char *ext, const char *new_ext)
{
	int len = strlen(name);
	int a = strlen(ext);

	/* len up to and including the final '.' */
	len -= a;
	if (len <= 1)
		return;
	if (name[len - 1] != '.')
		return;
	/* msvc doesn't have strncasecmp, has _strnicmp instead - crazy */
	if (strncasecmp(name + len, ext, a))
		return;

	std::string newname(name, len);
	newname += new_ext;

	/*
	 * Ignore errors. Maybe we can't create the backup file,
	 * maybe no old file existed.  Regardless, we'll write the
	 * new file.
	 */
	(void) subsurface_rename(name, newname.c_str());
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
			if (last_xml_version < dataformat_version) {
				std::string special_ext = std::string(extension[i]) + ".v" + std::to_string(last_xml_version);
				save_backup(filename, extension[i], special_ext.c_str());
			} else {
				save_backup(filename, extension[i], "bak");
			}
			break;
		}
		i++;
	}
}

int save_dives_logic(const char *filename, const bool select_only, bool anonymize)
{
	membuffer buf;
	struct git_info info;
	FILE *f;
	int error = 0;

	if (is_git_repository(filename, &info)) {
		error = git_save_dives(&info, select_only);
		return error;
	}

	save_dives_buffer(&buf, select_only, anonymize);

	if (same_string(filename, "-")) {
		f = stdout;
	} else {
		try_to_backup(filename);
		error = -1;
		f = subsurface_fopen(filename, "w");
	}
	if (f) {
		flush_buffer(&buf, f);
		error = fclose(f);
	}
	if (error)
		report_error(translate("gettextFromC", "Failed to save dives to %s (%s)"), filename, strerror(errno));

	return error;
}

// This is executed in a separate thread with QtConcurrent::run
// so we don't have access to NotificationWidget

static std::pair<int, std::string> export_dives_xslt_doit(const char *filename, struct xml_params *params, bool selected, int units, const char *export_xslt, bool anonymize);

std::pair<int, std::string> export_dives_xslt(const char *filename, const bool selected, const int units, const char *export_xslt, bool anonymize)
{
	struct xml_params *params = alloc_xml_params();
	auto ret = export_dives_xslt_doit(filename, params, selected, units, export_xslt, anonymize);
	free_xml_params(params);
	return ret;
}

static std::pair<int, std::string> export_dives_xslt_doit(const char *filename, struct xml_params *params, bool selected, int units, const char *export_xslt, bool anonymize)
{
	FILE *f;
	membuffer buf;
	xmlDoc *doc;
	xsltStylesheetPtr xslt = NULL;
	xmlDoc *transformed;
	auto res = std::make_pair(0, std::string());

	if (verbose)
		report_info("export_dives_xslt with stylesheet %s", export_xslt);

	if (!filename)
		return std::make_pair(-1, translate("gettextFromC", "No filename for export"));

	/* Save XML to file and convert it into a memory buffer */
	save_dives_buffer(&buf, selected, anonymize);

	/*
	 * Parse the memory buffer into XML document and
	 * transform it to selected export format, finally dumping
	 * the XML into a character buffer.
	 */
	doc = xmlReadMemory(buf.buffer, buf.len, "divelog", NULL, XML_PARSE_HUGE);
	if (!doc)
		return std::make_pair(-1, translate("gettextFromC", "Failed to read XML memory"));

	/* Convert to export format */
	xslt = get_stylesheet(export_xslt);
	if (!xslt)
		return std::make_pair(-1, translate("gettextFromC", "Failed to open export conversion stylesheet"));

	xml_params_add_int(params, "units", units);

	transformed = xsltApplyStylesheet(xslt, doc, xml_params_get(params));
	xmlFreeDoc(doc);

	/* Write the transformed export to file */
	f = subsurface_fopen(filename, "w");
	if (f) {
		xsltSaveResultToFile(f, transformed, xslt);
		fclose(f);
		/* Check write errors? */
	} else {
		res = std::make_pair(-1, format_string_std(translate("gettextFromC", "Failed to open %s for writing (%s)"), filename, strerror(errno)));
	}
	xsltFreeStylesheet(xslt);
	xmlFreeDoc(transformed);

	return res;
}

static void save_dive_sites_buffer(struct membuffer *b, const struct dive_site *sites[], int nr_sites, bool anonymize)
{
	int i;
	put_format(b, "<divesites program='subsurface' version='%d'>\n", dataformat_version);

	/* save the dive sites */
	for (i = 0; i < nr_sites; i++) {
		const struct dive_site *ds = sites[i];

		put_format(b, "<site uuid='%8x'", ds->uuid);
		show_utf8_blanked(b, ds->name.c_str(), " name='", "'", 1, anonymize);
		put_location(b, &ds->location, " gps='", "'");
		show_utf8_blanked(b, ds->description.c_str(), " description='", "'", 1, anonymize);
		put_format(b, ">\n");
		show_utf8_blanked(b, ds->notes.c_str(), "  <notes>", " </notes>\n", 0, anonymize);
		for (const auto &t: ds->taxonomy) {
			if (t.category != TC_NONE && !t.value.empty()) {
				put_format(b, "  <geo cat='%d'", t.category);
				put_format(b, " origin='%d'", t.origin);
				show_utf8_blanked(b, t.value.c_str(), " value='", "'", 1, anonymize);
				put_format(b, "/>\n");
			}
		}
		put_format(b, "</site>\n");
	}
	put_format(b, "</divesites>\n");
}

int save_dive_sites_logic(const char *filename, const struct dive_site *sites[], int nr_sites, bool anonymize)
{
	membuffer buf;
	FILE *f;
	int error = 0;

	save_dive_sites_buffer(&buf, sites, nr_sites, anonymize);

	if (same_string(filename, "-")) {
		f = stdout;
	} else {
		try_to_backup(filename);
		error = -1;
		f = subsurface_fopen(filename, "w");
	}
	if (f) {
		flush_buffer(&buf, f);
		error = fclose(f);
	}
	if (error)
		report_error(translate("gettextFromC", "Failed to save divesites to %s (%s)"), filename, strerror(errno));

	return error;
}
