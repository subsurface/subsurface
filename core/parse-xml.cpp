// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libdivecomputer/parser.h>

#include "gettext.h"

#include "dive.h"
#include "divelog.h"
#include "divesite.h"
#include "errorhelper.h"
#include "parse.h"
#include "format.h"
#include "subsurface-float.h"
#include "subsurface-string.h"
#include "subsurface-time.h"
#include "trip.h"
#include "device.h"
#include "membuffer.h"
#include "picture.h"
#include "qthelper.h"
#include "range.h"
#include "sample.h"
#include "tag.h"
#include "tanksensormapping.h"
#include "version.h"
#include "xmlparams.h"

int last_xml_version = -1;

static xmlDoc *test_xslt_transforms(xmlDoc *doc, const struct xml_params *params);

static void divedate(const char *buffer, timestamp_t *when, struct parser_state *state)
{
	int d, m, y;
	int hh, mm, ss;

	hh = 0;
	mm = 0;
	ss = 0;
	if (sscanf(buffer, "%d.%d.%d %d:%d:%d", &d, &m, &y, &hh, &mm, &ss) >= 3) {
		/* This is ok, and we got at least the date */
	} else if (sscanf(buffer, "%d-%d-%d %d:%d:%d", &y, &m, &d, &hh, &mm, &ss) >= 3) {
		/* This is also ok */
	} else {
		report_info("Unable to parse date '%s'", buffer);
		return;
	}
	state->cur_tm.tm_year = y;
	state->cur_tm.tm_mon = m - 1;
	state->cur_tm.tm_mday = d;
	state->cur_tm.tm_hour = hh;
	state->cur_tm.tm_min = mm;
	state->cur_tm.tm_sec = ss;

	*when = utc_mktime(&state->cur_tm);
}

static void divetime(const char *buffer, timestamp_t *when, struct parser_state *state)
{
	int h, m, s = 0;

	if (sscanf(buffer, "%d:%d:%d", &h, &m, &s) >= 2) {
		state->cur_tm.tm_hour = h;
		state->cur_tm.tm_min = m;
		state->cur_tm.tm_sec = s;
		*when = utc_mktime(&state->cur_tm);
	}
}

/* Libdivecomputer: "2011-03-20 10:22:38" */
static void divedatetime(const char *buffer, timestamp_t *when, struct parser_state *state)
{
	int y, m, d;
	int hr, min, sec;

	if (sscanf(buffer, "%d-%d-%d %d:%d:%d",
		   &y, &m, &d, &hr, &min, &sec) == 6) {
		state->cur_tm.tm_year = y;
		state->cur_tm.tm_mon = m - 1;
		state->cur_tm.tm_mday = d;
		state->cur_tm.tm_hour = hr;
		state->cur_tm.tm_min = min;
		state->cur_tm.tm_sec = sec;
		*when = utc_mktime(&state->cur_tm);
	}
}

enum ParseState {
	FINDSTART,
	FINDEND
};
static void divetags(const char *buffer, tag_list *tags)
{
	int i = 0, start = 0, end = 0;
	enum ParseState state = FINDEND;
	int len = buffer ? strlen(buffer) : 0;

	while (i < len) {
		if (buffer[i] == ',') {
			if (state == FINDSTART) {
				/* Detect empty tags */
			} else if (state == FINDEND) {
				/* Found end of tag */
				if (i > 0 && buffer[i - 1] != '\\') {
					std::string s(buffer + start, i - start);
					state = FINDSTART;
					taglist_add_tag(*tags, s.c_str());
				} else {
					state = FINDSTART;
				}
			}
		} else if (buffer[i] == ' ') {
			/* Handled */
		} else {
			/* Found start of tag */
			if (state == FINDSTART) {
				state = FINDEND;
				start = i;
			} else if (state == FINDEND) {
				end = i;
			}
		}
		i++;
	}
	if (state == FINDEND) {
		if (end < start)
			end = len - 1;
		if (len > 0) {
			std::string s(buffer + start, i - start);
			taglist_add_tag(*tags, buffer + start);
		}
	}
}

enum number_type {
	NEITHER,
	FLOATVAL
};

static enum number_type parse_float(const char *buffer, double &res, const char *&endp)
{
	double val;
	static bool first_time = true;

	val = ascii_strtod(buffer, &endp);
	if (endp == buffer)
		return NEITHER;
	if (*endp == ',') {
		if (nearly_equal(val, rint(val))) {
			/* we really want to send an error if this is a Subsurface native file
			 * as this is likely indication of a bug - but right now we don't have
			 * that information available */
			if (first_time) {
				report_info("Floating point value with decimal comma (%s)?", buffer);
				first_time = false;
			}
			/* Try again in permissive mode*/
			val = permissive_strtod(buffer, &endp);
		}
	}

	res = val;
	return FLOATVAL;
}

static enum number_type parse_float(const char *buffer, double &res)
{
	const char *end;
	return parse_float(buffer, res, end);
}

static void pressure(const char *buffer, pressure_t *pressure, struct parser_state *state)
{
	double mbar = 0.0;
	double val;

	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		/* Just ignore zero values */
		if (!val)
			break;
		switch (state->xml_parsing_units.pressure) {
		case units::PASCALS:
			mbar = val / 100;
			break;
		case units::BAR:
			/* Assume mbar, but if it's really small, it's bar */
			mbar = val;
			if (fabs(mbar) < 5000)
				mbar = mbar * 1000;
			break;
		case units::PSI:
			mbar = psi_to_mbar(val);
			break;
		}
		if (fabs(mbar) > 5 && fabs(mbar) < 5000000) {
			pressure->mbar = lrint(mbar);
			break;
		}
	/* fallthrough */
	default:
		report_info("Strange pressure reading %s", buffer);
	}
}

std::string trimspace(const char *s)
{
	while (isspace(*s))
		++s;
	if (!*s)
		return std::string();
	const char *end = s + strlen(s);
	while (isspace(end[-1]))
		--end;
	return std::string(s, end - s);
}

static void cylinder_use(const char *buffer, enum cylinderuse *cyl_use, struct parser_state *state)
{
	std::string trimmed = trimspace(buffer);
	if (!trimmed.empty()) {
		enum cylinderuse use = cylinderuse_from_text(trimmed.c_str());
		*cyl_use = use;
		if (use == OXYGEN)
			state->o2pressure_sensor = static_cast<int>(state->cur_dive->cylinders.size()) - 1;
	}
}

static void salinity(const char *buffer, int *salinity)
{
	double val;
	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		*salinity = lrint(val * 10.0);
		break;
	default:
		report_info("Strange salinity reading %s", buffer);
	}
}

static void depth(const char *buffer, depth_t *depth, struct parser_state *state)
{
	double val;

	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		switch (state->xml_parsing_units.length) {
		case units::METERS:
			depth->mm = lrint(val * 1000.0);
			break;
		case units::FEET:
			depth->mm = feet_to_mm(val);
			break;
		}
		break;
	default:
		report_info("Strange depth reading %s", buffer);
	}
}

static void extra_data_start(struct parser_state *state)
{
	state->cur_extra_data.key.clear();
	state->cur_extra_data.value.clear();
}

static void extra_data_end(struct parser_state *state)
{
	// don't save partial structures - we must have both key and value
	if (!state->cur_extra_data.key.empty() && !state->cur_extra_data.value.empty())
		add_extra_data(get_dc(state), state->cur_extra_data.key.c_str(), state->cur_extra_data.value.c_str());
}

static void tank_sensor_mapping_start(struct parser_state *state)
{
	state->cur_tank_sensor_mapping.sensor_id = 0;
	state->cur_tank_sensor_mapping.cylinder_index = 0;
}

static void tank_sensor_mapping_end(struct parser_state *state)
{
	get_dc(state)->tank_sensor_mappings.push_back(tank_sensor_mapping { state->cur_tank_sensor_mapping.sensor_id, state->cur_tank_sensor_mapping.cylinder_index });
}

static void weight(const char *buffer, weight_t *weight, struct parser_state *state)
{
	double val;

	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		switch (state->xml_parsing_units.weight) {
		case units::KG:
			weight->grams = lrint(val * 1000.0);
			break;
		case units::LBS:
			weight->grams = lbs_to_grams(val);
			break;
		}
		break;
	default:
		report_info("Strange weight reading %s", buffer);
	}
}

static void temperature(const char *buffer, temperature_t *temperature, struct parser_state *state)
{
	double val;

	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		switch (state->xml_parsing_units.temperature) {
		case units::KELVIN:
			temperature->mkelvin = lrint(val * 1000.0);
			break;
		case units::CELSIUS:
			temperature->mkelvin = C_to_mkelvin(val);
			break;
		case units::FAHRENHEIT:
			temperature->mkelvin = F_to_mkelvin(val);
			break;
		}
		break;
	default:
		report_info("Strange temperature reading %s", buffer);
	}
	/* temperatures outside -40C .. +70C should be ignored */
	if (temperature->mkelvin < ZERO_C_IN_MKELVIN - 40000 ||
	    temperature->mkelvin > ZERO_C_IN_MKELVIN + 70000)
		*temperature = 0_K;
}

static void sampletime(const char *buffer, duration_t *time)
{
	int i;
	int hr, min, sec;

	i = sscanf(buffer, "%d:%d:%d", &hr, &min, &sec);
	switch (i) {
	case 1:
		min = hr;
		hr = 0;
	/* fallthrough */
	case 2:
		sec = min;
		min = hr;
		hr = 0;
	/* fallthrough */
	case 3:
		time->seconds = (hr * 60 + min) * 60 + sec;
		break;
	default:
		*time = 0_sec;
		report_info("Strange sample time reading %s", buffer);
	}
}

static void offsettime(const char *buffer, offset_t *time)
{
	duration_t uoffset;
	int sign = 1;
	if (*buffer == '-') {
		sign = -1;
		buffer++;
	}
	/* yes, this could indeed fail if we have an offset > 34yrs
	 * - too bad */
	sampletime(buffer, &uoffset);
	time->seconds = sign * uoffset.seconds;
}

static void duration(const char *buffer, duration_t *time)
{
	/* DivingLog 5.08 (and maybe other versions) appear to sometimes
	 * store the dive time as 44.00 instead of 44:00;
	 * This attempts to parse this in a fairly robust way */
	if (!strchr(buffer, ':') && strchr(buffer, '.')) {
		std::string mybuffer(buffer);
		char *dot = strchr(mybuffer.data(), '.');
		*dot = ':';
		sampletime(mybuffer.data(), time);
	} else {
		sampletime(buffer, time);
	}
}

static void percent(const char *buffer, fraction_t *fraction)
{
	double val;
	const char *end;

	switch (parse_float(buffer, val, end)) {
	case FLOATVAL:
		/* Turn fractions into percent unless explicit.. */
		if (val <= 1.0) {
			while (isspace(*end))
				end++;
			if (*end != '%')
				val *= 100;
		}

		/* Then turn percent into our integer permille format */
		if (val >= 0 && val <= 100.0) {
			fraction->permille = lrint(val * 10);
			break;
		}
	default:
		report_info(translate("gettextFromC", "Strange percentage reading %s"), buffer);
		break;
	}
}

static void gasmix(const char *buffer, fraction_t *fraction, struct parser_state *state)
{
	/* libdivecomputer does negative percentages. */
	if (*buffer == '-')
		return;
	percent(buffer, fraction);
}

static void gasmix_nitrogen(const char *, struct gasmix *)
{
	/* Ignore n2 percentages. There's no value in them. */
}

static void cylindersize(const char *buffer, volume_t *volume)
{
	double val;

	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		volume->mliter = lrint(val * 1000.0);
		break;

	default:
		report_info("Strange volume reading %s", buffer);
		break;
	}
}

// We don't use gauge as a mode, and pscr doesn't exist as a libdc divemode
static const char *libdc_divemode_text[] = { "oc", "cc", "pscr", "freedive", "gauge"};

/* Extract the dive computer type from the xml text buffer */
static void get_dc_type(const char *buffer, enum divemode_t *dct)
{
	std::string trimmed = trimspace(buffer);
	if (!trimmed.empty()) {
		for (int i = 0; i < NUM_DIVEMODE; i++) {
			if (trimmed == divemode_text[i]) {
				*dct = (divemode_t)i;
				break;
			} else if (trimmed == libdc_divemode_text[i]) {
				*dct = (divemode_t)i;
				break;
			}
		}
	}
}

/* For divemode_text[] (defined in dive.h) determine the index of
 * the string contained in the xml divemode attribute and passed
 * in buffer, below. Typical xml input would be:
 * <event name='modechange' divemode='OC' /> */
static void event_divemode(const char *buffer, int *value)
{
	std::string trimmed = trimspace(buffer);
	for (int i = 0; i < NUM_DIVEMODE; i++) {
		if (trimmed == divemode_text[i]) {
			*value = i;
			break;
		}
	}
}

/* Compare a pattern with a name, whereby the name may end in '\0' or '.'. */
static int match_name(const char *pattern, const char *name)
{
	while (*pattern == *name && *pattern) {
		pattern++;
		name++;
	}
	return *pattern == '\0' && (*name == '\0' || *name == '.');
}

typedef void (*matchfn_t)(const char *buffer, void *);
static int match(const char *pattern, const char *name,
		 matchfn_t fn, char *buf, void *data)
{
	if (!match_name(pattern, name))
		return 0;
	fn(buf, data);
	return 1;
}

typedef void (*matchfn_state_t)(const char *buffer, void *, struct parser_state *state);
static int match_state(const char *pattern, const char *name,
		       matchfn_state_t fn, char *buf, void *data, struct parser_state *state)
{
	if (!match_name(pattern, name))
		return 0;
	fn(buf, data, state);
	return 1;
}

#define MATCH(pattern, fn, dest) ({ 			\
	/* Silly type compatibility test */ 		\
	if (0) (fn)("test", dest);			\
	match(pattern, name, (matchfn_t) (fn), buf, dest); })

#define MATCH_STATE(pattern, fn, dest) ({ 		\
	/* Silly type compatibility test */ 		\
	if (0) (fn)("test", dest, state);		\
	match_state(pattern, name, (matchfn_state_t) (fn), buf, dest, state); })

static void get_index(const char *buffer, int *i)
{
	*i = atoi(buffer);
}

static void get_bool(const char *buffer, bool *i)
{
	*i = atoi(buffer);
}

static void get_uint8(const char *buffer, uint8_t *i)
{
	*i = atoi(buffer);
}

static void get_uint16(const char *buffer, uint16_t *i)
{
	*i = atoi(buffer);
}

static void get_bearing(const char *buffer, bearing_t *bearing)
{
	bearing->degrees = atoi(buffer);
}

static void get_rating(const char *buffer, int *i)
{
	int j = atoi(buffer);
	if (j >= 0 && j <= 5) {
		*i = j;
	}
}

static void double_to_o2pressure(const char *buffer, o2pressure_t *i)
{
	i->mbar = lrint(ascii_strtod(buffer, NULL) * 1000.0);
}

static void hex_value(const char *buffer, uint32_t *i)
{
	*i = strtoul(buffer, NULL, 16);
}

static void dive_site(const char *buffer, struct dive *d, struct parser_state *state)
{
	uint32_t uuid;
	hex_value(buffer, &uuid);
	state->log->sites.get_by_uuid(uuid)->add_dive(d);
}

static void get_notrip(const char *buffer, bool *notrip)
{
	*notrip = !strcmp(buffer, "NOTRIP");
}

/*
 * Divinglog is crazy. The temperatures are in celsius. EXCEPT
 * for the sample temperatures, that are in Fahrenheit.
 * WTF?
 *
 * Oh, and I think Diving Log *internally* probably kept them
 * in celsius, because I'm seeing entries like
 *
 *	<Temp>32.0</Temp>
 *
 * in there. Which is freezing, aka 0 degC. I bet the "0" is
 * what Diving Log uses for "no temperature".
 *
 * So throw away crap like that.
 *
 * It gets worse. Sometimes the sample temperatures are in
 * Celsius, which apparently happens if you are in a SI
 * locale. So we now do:
 *
 * - temperatures < 32.0 == Celsius
 * - temperature == 32.0  -> garbage, it's a missing temperature (zero converted from C to F)
 * - temperatures > 32.0 == Fahrenheit
 */
static void fahrenheit(const char *buffer, temperature_t *temperature)
{
	double val;

	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		if (nearly_equal(val, 32.0))
			break;
		if (val < 32.0)
			temperature->mkelvin = C_to_mkelvin(val);
		else
			temperature->mkelvin = F_to_mkelvin(val);
		break;
	default:
		report_info("Crazy Diving Log temperature reading %s", buffer);
	}
}

/*
 * Did I mention how bat-shit crazy divinglog is? The sample
 * pressures are in PSI. But the tank working pressure is in
 * bar. WTF^2?
 *
 * Crazy stuff like this is why subsurface has everything in
 * these inconvenient typed structures, and you have to say
 * "pressure->mbar" to get the actual value. Exactly so that
 * you can never have unit confusion.
 *
 * It gets worse: sometimes apparently the pressures are in
 * bar, sometimes in psi. Dirk suspects that this may be a
 * DivingLog Uemis importer bug, and that they are always
 * supposed to be in bar, but that the importer got the
 * sample importing wrong.
 *
 * Sadly, there's no way to really tell. So I think we just
 * have to have some arbitrary cut-off point where we assume
 * that smaller values mean bar.. Not good.
 */
static void psi_or_bar(const char *buffer, pressure_t *pressure)
{
	double val;

	switch (parse_float(buffer, val)) {
	case FLOATVAL:
		if (val > 400)
			pressure->mbar = psi_to_mbar(val);
		else
			pressure->mbar = lrint(val * 1000);
		break;
	default:
		report_info("Crazy Diving Log PSI reading %s", buffer);
	}
}

static int divinglog_fill_sample(struct sample *sample, const char *name, char *buf, struct parser_state *state)
{
	return MATCH("time.p", sampletime, &sample->time) ||
	       MATCH_STATE("depth.p", depth, &sample->depth) ||
	       MATCH("temp.p", fahrenheit, &sample->temperature) ||
	       MATCH("press1.p", psi_or_bar, &sample->pressure[0]) ||
	       0;
}

static void uddf_gasswitch(const char *buffer, struct sample *sample, struct parser_state *state)
{
	int idx = atoi(buffer);
	int seconds = sample->time.seconds;
	struct divecomputer *dc = get_dc(state);

	add_gas_switch_event(state->cur_dive.get(), dc, seconds, idx);
}

static int uddf_fill_sample(struct sample *sample, const char *name, char *buf, struct parser_state *state)
{
	return MATCH("divetime", sampletime, &sample->time) ||
	       MATCH_STATE("depth", depth, &sample->depth) ||
	       MATCH_STATE("temperature", temperature, &sample->temperature) ||
	       MATCH_STATE("tankpressure", pressure, &sample->pressure[0]) ||
	       MATCH_STATE("ref.switchmix", uddf_gasswitch, sample) ||
	       0;
}

static void eventtime(const char *buffer, duration_t *duration, struct parser_state *state)
{
	sampletime(buffer, duration);
	if (state->cur_sample)
		*duration += state->cur_sample->time;
}

static void try_to_match_autogroup(const char *name, char *buf, struct parser_state *state)
{
	bool autogroup;

	start_match("autogroup", name, buf);
	if (MATCH("state.autogroup", get_bool, &autogroup)) {
		state->log->autogroup = autogroup;
		return;
	}
	nonmatch("autogroup", name, buf);
}

static void get_cylinderindex(const char *buffer, int16_t *i, struct parser_state *state)
{
	*i = atoi(buffer);
	if (state->lastcylinderindex != *i) {
		add_gas_switch_event(state->cur_dive.get(), get_dc(state), state->cur_sample->time.seconds, *i);
		state->lastcylinderindex = *i;
	}
}

static void get_sensor(const char *buffer, int16_t *i)
{
	*i = atoi(buffer);
}

static void parse_libdc_deco(const char *buffer, struct sample *s)
{
	if (strcmp(buffer, "deco") == 0) {
		s->in_deco = true;
	} else if (strcmp(buffer, "ndl") == 0) {
		s->in_deco = false;
		// The time wasn't stoptime, it was ndl
		s->ndl = s->stoptime;
		s->stoptime = 0_sec;
	}
}

static void try_to_fill_dc_settings(const char *name, char *buf, struct parser_state *state)
{
	start_match("divecomputerid", name, buf);
	if (MATCH("model.divecomputerid", utf8_string_std, &state->cur_settings.dc.model))
		return;
	if (MATCH("deviceid.divecomputerid", hex_value, &state->cur_settings.dc.deviceid))
		return;
	if (MATCH("nickname.divecomputerid", utf8_string_std, &state->cur_settings.dc.nickname))
		return;
	if (MATCH("serial.divecomputerid", utf8_string_std, &state->cur_settings.dc.serial_nr))
		return;
	if (MATCH("firmware.divecomputerid", utf8_string_std, &state->cur_settings.dc.firmware))
		return;

	nonmatch("divecomputerid", name, buf);
}

static void try_to_fill_fingerprint(const char *name, char *buf, struct parser_state *state)
{
	start_match("fingerprint", name, buf);
	if (MATCH("model.fingerprint", hex_value, &state->cur_settings.fingerprint.model))
		return;
	if (MATCH("serial.fingerprint", hex_value, &state->cur_settings.fingerprint.serial))
		return;
	if (MATCH("deviceid.fingerprint", hex_value, &state->cur_settings.fingerprint.fdeviceid))
		return;
	if (MATCH("diveid.fingerprint", hex_value, &state->cur_settings.fingerprint.fdiveid))
		return;
	if (MATCH("data.fingerprint", utf8_string_std, &state->cur_settings.fingerprint.data))
		return;
	nonmatch("fingerprint", name, buf);
}

static void try_to_fill_event(const char *name, char *buf, struct parser_state *state)
{
	start_match("event", name, buf);
	if (MATCH("event", utf8_string_std, &state->cur_event.name))
		return;
	if (MATCH("name", utf8_string_std, &state->cur_event.name))
		return;
	if (MATCH_STATE("time", eventtime, &state->cur_event.time))
		return;
	if (MATCH("type", get_index, &state->cur_event.type))
		return;
	if (MATCH("flags", get_index, &state->cur_event.flags))
		return;
	if (MATCH("value", get_index, &state->cur_event.value))
		return;
	if (MATCH("divemode", event_divemode, &state->cur_event.value))
		return;
	if (MATCH("cylinder", get_index, &state->cur_event.gas.index)) {
		/* We add one to indicate that we got an actual cylinder index value */
		state->cur_event.gas.index++;
		return;
	}
	if (MATCH("o2", percent, &state->cur_event.gas.mix.o2))
		return;
	if (MATCH("he", percent, &state->cur_event.gas.mix.he))
		return;
	nonmatch("event", name, buf);
}

static int match_dc_data_fields(struct divecomputer *dc, const char *name, char *buf, struct parser_state *state)
{
	if (MATCH_STATE("maxdepth", depth, &dc->maxdepth))
		return 1;
	if (MATCH_STATE("meandepth", depth, &dc->meandepth))
		return 1;
	if (MATCH_STATE("max.depth", depth, &dc->maxdepth))
		return 1;
	if (MATCH_STATE("mean.depth", depth, &dc->meandepth))
		return 1;
	if (MATCH("duration", duration, &dc->duration))
		return 1;
	if (MATCH("divetime", duration, &dc->duration))
		return 1;
	if (MATCH("divetimesec", duration, &dc->duration))
		return 1;
	if (MATCH("last-manual-time", duration, &dc->last_manual_time))
		return 1;
	if (MATCH("surfacetime", duration, &dc->surfacetime))
		return 1;
	if (MATCH_STATE("airtemp", temperature, &dc->airtemp))
		return 1;
	if (MATCH_STATE("watertemp", temperature, &dc->watertemp))
		return 1;
	if (MATCH_STATE("air.temperature", temperature, &dc->airtemp))
		return 1;
	if (MATCH_STATE("water.temperature", temperature, &dc->watertemp))
		return 1;
	if (MATCH_STATE("pressure.surface", pressure, &dc->surface_pressure))
		return 1;
	if (MATCH("salinity.water", salinity, &dc->salinity))
		return 1;
	if (MATCH("key.extradata", utf8_string_std, &state->cur_extra_data.key))
		return 1;
	if (MATCH("value.extradata", utf8_string_std, &state->cur_extra_data.value))
		return 1;
	if (MATCH("sensorid.tanksensormapping", get_index, (int *)&state->cur_tank_sensor_mapping.sensor_id))
		return 1;
	if (MATCH("cylinderindex.tanksensormapping", get_index, (int *)&state->cur_tank_sensor_mapping.cylinder_index))
		return 1;
	if (MATCH("divemode", get_dc_type, &dc->divemode))
		return 1;
	if (MATCH("salinity", salinity, &dc->salinity))
		return 1;
	if (MATCH_STATE("atmospheric", pressure, &dc->surface_pressure))
		return 1;
	return 0;
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dc(struct divecomputer *dc, const char *name, char *buf, struct parser_state *state)
{
	unsigned int deviceid;

	start_match("divecomputer", name, buf);

	if (MATCH_STATE("date", divedate, &dc->when))
		return;
	if (MATCH_STATE("time", divetime, &dc->when))
		return;
	if (MATCH("model", utf8_string_std, &dc->model))
		return;
	if (MATCH("deviceid", hex_value, &deviceid))
		return;
	if (MATCH("diveid", hex_value, &dc->diveid))
		return;
	if (MATCH("dctype", get_dc_type, &dc->divemode))
		return;
	if (MATCH("no_o2sensors", get_uint8, &dc->no_o2sensors))
		return;
	if (match_dc_data_fields(dc, name, buf, state))
		return;

	nonmatch("divecomputer", name, buf);
}

/* We're in samples - try to convert the random xml value to something useful */
static void try_to_fill_sample(struct sample *sample, const char *name, char *buf, struct parser_state *state)
{
	int in_deco;
	pressure_t p;

	start_match("sample", name, buf);
	if (MATCH_STATE("pressure.sample", pressure, &sample->pressure[0]))
		return;
	if (MATCH_STATE("cylpress.sample", pressure, &sample->pressure[0]))
		return;
	if (MATCH_STATE("pdiluent.sample", pressure, &sample->pressure[0]))
		return;
	if (MATCH_STATE("o2pressure.sample", pressure, &sample->pressure[1]))
		return;
	/* Christ, this is ugly */
	if (MATCH_STATE("pressure0.sample", pressure, &p)) {
		add_sample_pressure(sample, 0, p.mbar);
		return;
	}
	if (MATCH_STATE("pressure1.sample", pressure, &p)) {
		add_sample_pressure(sample, 1, p.mbar);
		return;
	}
	if (MATCH_STATE("pressure2.sample", pressure, &p)) {
		add_sample_pressure(sample, 2, p.mbar);
		return;
	}
	if (MATCH_STATE("pressure3.sample", pressure, &p)) {
		add_sample_pressure(sample, 3, p.mbar);
		return;
	}
	if (MATCH_STATE("pressure4.sample", pressure, &p)) {
		add_sample_pressure(sample, 4, p.mbar);
		return;
	}
	if (MATCH_STATE("cylinderindex.sample", get_cylinderindex, &sample->sensor[0]))
		return;
	if (MATCH("sensor.sample", get_sensor, &sample->sensor[0]))
		return;
	if (MATCH_STATE("depth.sample", depth, &sample->depth))
		return;
	if (MATCH_STATE("temp.sample", temperature, &sample->temperature))
		return;
	if (MATCH_STATE("temperature.sample", temperature, &sample->temperature))
		return;
	if (MATCH("sampletime.sample", sampletime, &sample->time))
		return;
	if (MATCH("time.sample", sampletime, &sample->time))
		return;
	if (MATCH("ndl.sample", sampletime, &sample->ndl))
		return;
	if (MATCH("tts.sample", sampletime, &sample->tts))
		return;
	if (MATCH("in_deco.sample", get_index, &in_deco)) {
		sample->in_deco = (in_deco == 1);
		return;
	}
	if (MATCH("stoptime.sample", sampletime, &sample->stoptime))
		return;
	if (MATCH_STATE("stopdepth.sample", depth, &sample->stopdepth))
		return;
	if (MATCH("cns.sample", get_uint16, &sample->cns))
		return;
	if (MATCH("rbt.sample", sampletime, &sample->rbt))
		return;
	if (MATCH("sensor1.sample", double_to_o2pressure, &sample->o2sensor[0])) // CCR O2 sensor data
		return;
	if (MATCH("sensor2.sample", double_to_o2pressure, &sample->o2sensor[1]))
		return;
	if (MATCH("sensor3.sample", double_to_o2pressure, &sample->o2sensor[2]))
		return;
	if (MATCH("sensor4.sample", double_to_o2pressure, &sample->o2sensor[3]))
		return;
	if (MATCH("sensor5.sample", double_to_o2pressure, &sample->o2sensor[4]))
		return;
	if (MATCH("sensor6.sample", double_to_o2pressure, &sample->o2sensor[5])) // up to 6 CCR sensors
		return;
	if (MATCH("dc_supplied_ppo2.sample", double_to_o2pressure, &sample->o2sensor[6])) // the dive computer calculated ppO2
		return;
	if (MATCH("po2.sample", double_to_o2pressure, &sample->setpoint))
		return;
	if (MATCH("heartbeat", get_uint8, &sample->heartbeat))
		return;
	if (MATCH("bearing", get_bearing, &sample->bearing))
		return;
	if (MATCH("setpoint.sample", double_to_o2pressure, &sample->setpoint))
		return;
	if (MATCH("ppo2.sample", double_to_o2pressure, &sample->o2sensor[state->next_o2_sensor])) {
		state->next_o2_sensor++;
		return;
	}
	if (MATCH("deco.sample", parse_libdc_deco, sample))
		return;
	if (MATCH("time.deco", sampletime, &sample->stoptime))
		return;
	if (MATCH_STATE("depth.deco", depth, &sample->stopdepth))
		return;

	switch (state->import_source) {
	case parser_state::DIVINGLOG:
		if (divinglog_fill_sample(sample, name, buf, state))
			return;
		break;
	case parser_state::UDDF:
		if (uddf_fill_sample(sample, name, buf, state))
			return;
		break;

	default:
		break;
	}

	nonmatch("sample", name, buf);
}

static void divinglog_place(const char *place, struct dive *d, struct parser_state *state)
{
	struct dive_site *ds;

	std::string buffer = format_string_std(
		 "%s%s%s%s%s",
		 place,
		 !state->city.empty() ? ", " : "",
		 !state->city.empty() ? state->city.c_str() : "",
		 !state->country.empty() ? ", " : "",
		 !state->country.empty() ? state->country.c_str() : "");
	ds = state->log->sites.get_by_name(buffer);
	if (!ds)
		ds = state->log->sites.create(buffer);
	ds->add_dive(d);

	// TODO: capture the country / city info in the taxonomy instead
	state->city.clear();
	state->country.clear();
}

static int divinglog_dive_match(struct dive *dive, const char *name, char *buf, struct parser_state *state)
{
	/* For cylinder related fields, we might have to create a cylinder first. */
	cylinder_t cyl;
	if (MATCH("tanktype", utf8_string_std, &cyl.type.description)) {
		dive->get_or_create_cylinder(0)->type.description = std::move(cyl.type.description);
		return 1;
	}
	if (MATCH("tanksize", cylindersize, &cyl.type.size)) {
		dive->get_or_create_cylinder(0)->type.size = cyl.type.size;
		return 1;
	}
	if (MATCH_STATE("presw", pressure, &cyl.type.workingpressure)) {
		dive->get_or_create_cylinder(0)->type.workingpressure = cyl.type.workingpressure;
		return 1;
	}
	if (MATCH_STATE("press", pressure, &cyl.start)) {
		dive->get_or_create_cylinder(0)->start = cyl.start;
		return 1;
	}
	if (MATCH_STATE("prese", pressure, &cyl.end)) {
		dive->get_or_create_cylinder(0)->end = cyl.end;
		return 1;
	}
	return MATCH_STATE("divedate", divedate, &dive->when) ||
	       MATCH_STATE("entrytime", divetime, &dive->when) ||
	       MATCH("divetime", duration, &dive->dcs[0].duration) ||
	       MATCH_STATE("depth", depth, &dive->dcs[0].maxdepth) ||
	       MATCH_STATE("depthavg", depth, &dive->dcs[0].meandepth) ||
	       MATCH("comments", utf8_string_std, &dive->notes) ||
	       MATCH("names.buddy", utf8_string_std, &dive->buddy) ||
	       MATCH("name.country", utf8_string_std, &state->country) ||
	       MATCH("name.city", utf8_string_std, &state->city) ||
	       MATCH_STATE("name.place", divinglog_place, dive) ||
	       0;
}

/*
 * Uddf specifies ISO 8601 time format.
 *
 * There are many variations on that. This handles the useful cases.
 */
static void uddf_datetime(const char *buffer, timestamp_t *when, struct parser_state *state)
{
	char c;
	int y, m, d, hh, mm, ss;
	struct tm tm = { 0 };
	int i;

	i = sscanf(buffer, "%d-%d-%d%c%d:%d:%d", &y, &m, &d, &c, &hh, &mm, &ss);
	if (i == 7)
		goto success;
	ss = 0;
	if (i == 6)
		goto success;

	i = sscanf(buffer, "%04d%02d%02d%c%02d%02d%02d", &y, &m, &d, &c, &hh, &mm, &ss);
	if (i == 7)
		goto success;
	ss = 0;
	if (i == 6)
		goto success;
bad_date:
	report_info("Bad date time %s", buffer);
	return;

success:
	if (c != 'T' && c != ' ')
		goto bad_date;
	tm.tm_year = y;
	tm.tm_mon = m - 1;
	tm.tm_mday = d;
	tm.tm_hour = hh;
	tm.tm_min = mm;
	tm.tm_sec = ss;
	*when = utc_mktime(&tm);
}

#define uddf_datedata(name, offset)                                                                \
	static void uddf_##name(const char *buffer, timestamp_t *when, struct parser_state *state) \
	{                                                                                          \
		state->cur_tm.tm_##name = atoi(buffer) + offset;                                   \
		*when = utc_mktime(&state->cur_tm);                                                \
	}

uddf_datedata(year, 0)
uddf_datedata(mon, -1)
uddf_datedata(mday, 0)
uddf_datedata(hour, 0)
uddf_datedata(min, 0)

static int uddf_dive_match(struct dive *dive, const char *name, char *buf, struct parser_state *state)
{
	return MATCH_STATE("datetime", uddf_datetime, &dive->when) ||
	       MATCH("diveduration", duration, &dive->dcs[0].duration) ||
	       MATCH_STATE("greatestdepth", depth, &dive->dcs[0].maxdepth) ||
	       MATCH_STATE("year.date", uddf_year, &dive->when) ||
	       MATCH_STATE("month.date", uddf_mon, &dive->when) ||
	       MATCH_STATE("day.date", uddf_mday, &dive->when) ||
	       MATCH_STATE("hour.time", uddf_hour, &dive->when) ||
	       MATCH_STATE("minute.time", uddf_min, &dive->when) ||
	       0;
}

/*
 * This parses "floating point" into micro-degrees.
 * We don't do exponentials etc, if somebody does
 * GPS locations in that format, they are insane.
 */
static degrees_t parse_degrees(const char *buf, const char **end)
{
	int sign = 1, decimals = 6, value = 0;
	degrees_t ret;

	while (isspace(*buf))
		buf++;
	switch (*buf) {
	case '-':
		sign = -1;
	/* fallthrough */
	case '+':
		buf++;
	}
	while (isdigit(*buf)) {
		value = 10 * value + *buf - '0';
		buf++;
	}

	/* Get the first six decimals if they exist */
	if (*buf == '.')
		buf++;
	do {
		value *= 10;
		if (isdigit(*buf)) {
			value += *buf - '0';
			buf++;
		}
	} while (--decimals);

	/* Rounding */
	switch (*buf) {
	case '5' ... '9':
		value++;
	}
	while (isdigit(*buf))
		buf++;

	*end = buf;
	ret.udeg = value * sign;
	return ret;
}

static void gps_lat(const char *buffer, struct dive *dive, struct parser_state *state)
{
	const char *end;
	location_t location = { };
	struct dive_site *ds = dive->dive_site;

	location.lat = parse_degrees(buffer, &end);
	if (!ds) {
		state->log->sites.create(std::string(), location)->add_dive(dive);
	} else {
		if (ds->location.lat.udeg && ds->location.lat.udeg != location.lat.udeg)
			report_info("Oops, changing the latitude of existing dive site id %8x name %s; not good", ds->uuid,
					ds->name.empty() ? "(unknown)" : ds->name.c_str());
		ds->location.lat = location.lat;
	}
}

static void gps_long(const char *buffer, struct dive *dive, struct parser_state *state)
{
	const char *end;
	location_t location = { };
	struct dive_site *ds = dive->dive_site;

	location.lon = parse_degrees(buffer, &end);
	if (!ds) {
		state->log->sites.create(std::string(), location)->add_dive(dive);
	} else {
		if (ds->location.lon.udeg && ds->location.lon.udeg != location.lon.udeg)
			report_info("Oops, changing the longitude of existing dive site id %8x name %s; not good", ds->uuid,
					ds->name.empty() ? "(unknown)" : ds->name.c_str());
		ds->location.lon = location.lon;
	}
}

/* We allow either spaces or a comma between the decimal degrees */
void parse_location(const char *buffer, location_t *loc)
{
	const char *end;
	loc->lat = parse_degrees(buffer, &end);
	if (*end == ',') end++;
	loc->lon = parse_degrees(end, &end);
}

static void gps_location(const char *buffer, struct dive_site *ds)
{
	parse_location(buffer, &ds->location);
}

static void gps_in_dive(const char *buffer, struct dive *dive, struct parser_state *state)
{
	struct dive_site *ds = dive->dive_site;
	location_t location;

	parse_location(buffer, &location);
	if (!ds) {
		// check if we have a dive site within 20 meters of that gps fix
		ds = state->log->sites.get_by_gps_proximity(location, 20);

		if (ds) {
			// found a site nearby; in case it turns out this one had a different name let's
			// remember the original coordinates so we can create the correct dive site later
			state->cur_location = location;
		} else {
			ds = state->log->sites.create(std::string(), location);
		}
		ds->add_dive(dive);
	} else {
		if (ds->has_gps_location() &&
		    has_location(&location) && ds->location != location) {
			// Houston, we have a problem
			report_info("dive site uuid in dive, but gps location (%10.6f/%10.6f) different from dive location (%10.6f/%10.6f)",
				ds->location.lat.udeg / 1000000.0, ds->location.lon.udeg / 1000000.0,
				location.lat.udeg / 1000000.0, location.lon.udeg / 1000000.0);
			std::string coords = printGPSCoordsC(&location);
			ds->notes += '\n';
			ds->notes += format_string_std(translate("gettextFromC", "multiple GPS locations for this dive site; also %s\n"), coords.c_str());
		} else {
			ds->location = location;
		}
	}
}

static void gps_picture_location(const char *buffer, struct picture *pic)
{
	parse_location(buffer, &pic->location);
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dive(struct dive *dive, const char *name, char *buf, struct parser_state *state)
{
	cylinder_t *cyl = !dive->cylinders.empty() ? &dive->cylinders.back() : NULL;
	weightsystem_t *ws = !dive->weightsystems.empty() > 0 ?
		&dive->weightsystems.back() : NULL;
	pressure_t p;
	weight_t w;
	start_match("dive", name, buf);

	switch (state->import_source) {
	case parser_state::DIVINGLOG:
		if (divinglog_dive_match(dive, name, buf, state))
			return;
		break;

	case parser_state::UDDF:
		if (uddf_dive_match(dive, name, buf, state))
			return;
		break;

	default:
		break;
	}
	if (MATCH_STATE("divesiteid", dive_site, dive))
		return;
	if (MATCH("number", get_index, &dive->number))
		return;
	if (MATCH("tags", divetags, &dive->tags))
		return;
	if (MATCH("tripflag", get_notrip, &dive->notrip))
		return;
	if (MATCH_STATE("date", divedate, &dive->when))
		return;
	if (MATCH_STATE("time", divetime, &dive->when))
		return;
	if (MATCH_STATE("datetime", divedatetime, &dive->when))
		return;
	/*
	 * Legacy format note: per-dive depths and duration get saved
	 * in the first dive computer entry
	 */
	if (match_dc_data_fields(&dive->dcs[0], name, buf, state))
		return;

	if (MATCH("filename.picture", utf8_string_std, &state->cur_picture.filename))
		return;
	if (MATCH("offset.picture", offsettime, &state->cur_picture.offset))
		return;
	if (MATCH("gps.picture", gps_picture_location, &state->cur_picture))
		return;
	if (std::string hash; MATCH("hash.picture", utf8_string_std, &hash)) {
		/* Legacy -> ignore. */
		return;
	}
	if (MATCH_STATE("cylinderstartpressure", pressure, &p)) {
		dive->get_or_create_cylinder(0)->start = p;
		return;
	}
	if (MATCH_STATE("cylinderendpressure", pressure, &p)) {
		dive->get_or_create_cylinder(0)->end = p;
		return;
	}
	if (MATCH_STATE("gps", gps_in_dive, dive))
		return;
	if (MATCH_STATE("Place", gps_in_dive, dive))
		return;
	if (MATCH_STATE("latitude", gps_lat, dive))
		return;
	if (MATCH_STATE("sitelat", gps_lat, dive))
		return;
	if (MATCH_STATE("lat", gps_lat, dive))
		return;
	if (MATCH_STATE("longitude", gps_long, dive))
		return;
	if (MATCH_STATE("sitelon", gps_long, dive))
		return;
	if (MATCH_STATE("lon", gps_long, dive))
		return;
	if (MATCH_STATE("location", add_dive_site, dive))
		return;
	if (MATCH_STATE("name.dive", add_dive_site, dive))
		return;
	if (MATCH("suit", utf8_string_std, &dive->suit))
		return;
	if (MATCH("divesuit", utf8_string_std, &dive->suit))
		return;
	if (MATCH("notes", utf8_string_std, &dive->notes))
		return;
	// For historic reasons, we accept dive guide as well as dive master
	if (MATCH("diveguide", utf8_string_std, &dive->diveguide))
		return;
	if (MATCH("divemaster", utf8_string_std, &dive->diveguide))
		return;
	if (MATCH("buddy", utf8_string_std, &dive->buddy))
		return;
	if (MATCH("watersalinity", salinity, &dive->user_salinity))
		return;
	if (MATCH("rating.dive", get_rating, &dive->rating))
		return;
	if (MATCH("visibility.dive", get_rating, &dive->visibility))
		return;
	if (MATCH("wavesize.dive", get_rating, &dive->wavesize))
		return;
	if (MATCH("current.dive", get_rating, &dive->current))
		return;
	if (MATCH("surge.dive", get_rating, &dive->surge))
		return;
	if (MATCH("chill.dive", get_rating, &dive->chill))
		return;
	if (MATCH_STATE("airpressure.dive", pressure, &dive->surface_pressure))
		return;
	if (ws) {
		if (MATCH("description.weightsystem", utf8_string_std, &ws->description))
			return;
		if (MATCH_STATE("weight.weightsystem", weight, &ws->weight))
			return;
	}
	if (MATCH_STATE("weight", weight, &w)) {
		weightsystem_t ws;
		ws.weight = w;
		dive->weightsystems.push_back(std::move(ws));
		return;
	}
	if (cyl) {
		if (MATCH("size.cylinder", cylindersize, &cyl->type.size))
			return;
		if (MATCH_STATE("workpressure.cylinder", pressure, &cyl->type.workingpressure))
			return;
		if (MATCH("description.cylinder", utf8_string_std, &cyl->type.description))
			return;
		if (MATCH_STATE("start.cylinder", pressure, &cyl->start))
			return;
		if (MATCH_STATE("end.cylinder", pressure, &cyl->end))
			return;
		if (MATCH_STATE("use.cylinder", cylinder_use, &cyl->cylinder_use))
			return;
		if (MATCH_STATE("depth.cylinder", depth, &cyl->depth))
			return;
		if (MATCH_STATE("o2", gasmix, &cyl->gasmix.o2))
			return;
		if (MATCH_STATE("o2percent", gasmix, &cyl->gasmix.o2))
			return;
		if (MATCH("n2", gasmix_nitrogen, &cyl->gasmix))
			return;
		if (MATCH_STATE("he", gasmix, &cyl->gasmix.he))
			return;
	}
	if (MATCH_STATE("air.divetemperature", temperature, &dive->airtemp))
		return;
	if (MATCH_STATE("water.divetemperature", temperature, &dive->watertemp))
		return;
	if (MATCH("invalid", get_bool, &dive->invalid))
		return;

	nonmatch("dive", name, buf);
}

/* We're in the top-level trip xml. Try to convert whatever value to a trip value */
static void try_to_fill_trip(dive_trip *dive_trip, const char *name, char *buf, struct parser_state *state)
{
	start_match("trip", name, buf);

	if (MATCH("location", utf8_string_std, &dive_trip->location))
		return;
	if (MATCH("notes", utf8_string_std, &dive_trip->notes))
		return;

	nonmatch("trip", name, buf);
}

/* We're processing a divesite entry - try to fill the components */
static void try_to_fill_dive_site(struct parser_state *state, const char *name, char *buf)
{
	auto &ds = state->cur_dive_site;
	std::string taxonomy_value;

	start_match("divesite", name, buf);

	if (MATCH("uuid", hex_value, &ds->uuid))
		return;
	if (MATCH("name", utf8_string_std, &ds->name))
		return;
	if (MATCH("description", utf8_string_std, &ds->description))
		return;
	if (MATCH("notes", utf8_string_std, &ds->notes))
		return;
	if (MATCH("gps", gps_location, ds.get()))
		return;
	if (MATCH("cat.geo", get_index, &state->taxonomy_category))
		return;
	if (MATCH("origin.geo", get_index, &state->taxonomy_origin))
		return;
	if (MATCH("value.geo", utf8_string_std, &taxonomy_value)) {
		/* The code assumes that "value.geo" comes last, which is against
		 * the expectations of an XML file. Let's at least make sure that
		 * cat and origin have been set! */
		if (state->taxonomy_category < 0 || state->taxonomy_origin < 0) {
			report_error("Warning: taxonomy value without origin or category");
		} else {
			taxonomy_set_category(ds->taxonomy, (taxonomy_category)state->taxonomy_category,
					      taxonomy_value, (taxonomy_origin)state->taxonomy_origin);
		}
		state->taxonomy_category = state->taxonomy_origin = -1;
		return;
	}

	nonmatch("divesite", name, buf);
}

static void try_to_fill_filter(struct filter_preset *filter, const char *name, char *buf)
{
	start_match("filterpreset", name, buf);

	if (MATCH("name", utf8_string_std, &filter->name))
		return;

	nonmatch("filterpreset", name, buf);
}

static void try_to_fill_fulltext(const char *name, char *buf, struct parser_state *state)
{
	start_match("fulltext", name, buf);

	if (MATCH("mode", utf8_string_std, &state->fulltext_string_mode))
		return;
	if (MATCH("fulltext", utf8_string_std, &state->fulltext))
		return;

	nonmatch("fulltext", name, buf);
}

static void try_to_fill_filter_constraint(const char *name, char *buf, struct parser_state *state)
{
	start_match("fulltext", name, buf);

	if (MATCH("type", utf8_string_std, &state->filter_constraint_type))
		return;
	if (MATCH("string_mode", utf8_string_std, &state->filter_constraint_string_mode))
		return;
	if (MATCH("range_mode", utf8_string_std, &state->filter_constraint_range_mode))
		return;
	if (MATCH("negate", get_bool, &state->filter_constraint_negate))
		return;
	if (MATCH("constraint", utf8_string_std, &state->filter_constraint))
		return;

	nonmatch("fulltext", name, buf);
}

static bool entry(const char *name, char *buf, struct parser_state *state)
{
	if (!strncmp(name, "version.program", sizeof("version.program") - 1) ||
	    !strncmp(name, "version.divelog", sizeof("version.divelog") - 1)) {
		last_xml_version = atoi(buf);
		report_datafile_version(last_xml_version);
	}
	if (state->in_userid) {
		return true;
	}
	if (state->in_settings) {
		try_to_fill_fingerprint(name, buf, state);
		try_to_fill_dc_settings(name, buf, state);
		try_to_match_autogroup(name, buf, state);
		return true;
	}
	if (state->cur_dive_site) {
		try_to_fill_dive_site(state, name, buf);
		return true;
	}
	if (state->in_filter_constraint) {
		try_to_fill_filter_constraint(name, buf, state);
		return true;
	}
	if (state->in_fulltext) {
		try_to_fill_fulltext(name, buf, state);
		return true;
	}
	if (state->cur_filter) {
		try_to_fill_filter(state->cur_filter.get(), name, buf);
		return true;
	}
	if (state->event_active) {
		try_to_fill_event(name, buf, state);
		return true;
	}
	if (state->cur_sample) {
		try_to_fill_sample(state->cur_sample, name, buf, state);
		return true;
	}
	if (state->cur_dc) {
		try_to_fill_dc(state->cur_dc, name, buf, state);
		return true;
	}
	if (state->cur_dive) {
		try_to_fill_dive(state->cur_dive.get(), name, buf, state);
		return true;
	}
	if (state->cur_trip) {
		try_to_fill_trip(state->cur_trip.get(), name, buf, state);
		return true;
	}
	return true;
}

static const char *nodename(xmlNode *node, char *buf, int len)
{
	int levels = 2;
	char *p = buf;

	if (!node || (node->type != XML_CDATA_SECTION_NODE && !node->name)) {
		return "root";
	}

	if (node->type == XML_CDATA_SECTION_NODE || (node->parent && !strcmp((const char *)node->name, "text")))
		node = node->parent;

	/* Make sure it's always NUL-terminated */
	p[--len] = 0;

	for (;;) {
		const char *name = (const char *)node->name;
		char c;
		while ((c = *name++) != 0) {
			/* Cheaper 'tolower()' for ASCII */
			c = (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
			*p++ = c;
			if (!--len)
				return buf;
		}
		*p = 0;
		node = node->parent;
		if (!node || !node->name)
			return buf;
		*p++ = '.';
		if (!--len)
			return buf;
		if (!--levels)
			return buf;
	}
}

#define MAXNAME 32

static bool visit_one_node(xmlNode *node, struct parser_state *state)
{
	xmlChar *content;
	char buffer[MAXNAME];
	const char *name;

	content = node->content;
	if (!content || xmlIsBlankNode(node))
		return true;

	name = nodename(node, buffer, sizeof(buffer));

	return entry(name, (char *)content, state);
}

static bool traverse(xmlNode *root, struct parser_state *state);

static bool traverse_properties(xmlNode *node, struct parser_state *state)
{
	xmlAttr *p;
	bool ret = true;

	for (p = node->properties; p; p = p->next)
		if ((ret = traverse(p->children, state)) == false)
			break;
	return ret;
}

static bool visit(xmlNode *n, struct parser_state *state)
{
	return visit_one_node(n, state) && traverse_properties(n, state) && traverse(n->children, state);
}

static void DivingLog_importer(struct parser_state *state)
{
	state->import_source = parser_state::DIVINGLOG;

	/*
	 * Diving Log units are really strange.
	 *
	 * Temperatures are in C, except in samples,
	 * when they are in Fahrenheit. Depths are in
	 * meters, an dpressure is in PSI in the samples,
	 * but in bar when it comes to working pressure.
	 *
	 * Crazy f*%^ morons.
	 */
	state->xml_parsing_units = SI_units;
}

static void uddf_importer(struct parser_state *state)
{
	state->import_source = parser_state::UDDF;
	state->xml_parsing_units = SI_units;
	state->xml_parsing_units.pressure = units::PASCALS;
	state->xml_parsing_units.temperature = units::KELVIN;
}

typedef void (*parser_func)(struct parser_state *);
/*
 * I'm sure this could be done as some fancy DTD rules.
 * It's just not worth the headache.
 */
static struct nesting {
	const char *name;
	parser_func start, end;
} nesting[] = {
	  { "fingerprint", fingerprint_settings_start, fingerprint_settings_end },
	  { "divecomputerid", dc_settings_start, dc_settings_end },
	  { "settings", settings_start, settings_end },
	  { "site", dive_site_start, dive_site_end },
	  { "filterpreset", filter_preset_start, filter_preset_end },
	  { "fulltext", fulltext_start, fulltext_end },
	  { "constraint", filter_constraint_start, filter_constraint_end },
	  { "dive", dive_start, dive_end },
	  { "Dive", dive_start, dive_end },
	  { "trip", trip_start, trip_end },
	  { "sample", sample_start, sample_end },
	  { "waypoint", sample_start, sample_end },
	  { "SAMPLE", sample_start, sample_end },
	  { "reading", sample_start, sample_end },
	  { "event", event_start, event_end },
	  { "mix", (parser_func)cylinder_start, (parser_func)cylinder_end },
	  { "gasmix", (parser_func)cylinder_start, (parser_func)cylinder_end },
	  { "cylinder", (parser_func)cylinder_start, (parser_func)cylinder_end },
	  { "weightsystem", ws_start, ws_end },
	  { "divecomputer", divecomputer_start, divecomputer_end },
	  { "P", sample_start, sample_end },
	  { "userid", userid_start, userid_stop},
	  { "picture", picture_start, picture_end },
	  { "extradata", extra_data_start, extra_data_end },
	  { "tanksensormapping", tank_sensor_mapping_start, tank_sensor_mapping_end },

	  /* Import type recognition */
	  { "Divinglog", DivingLog_importer },
	  { "uddf", uddf_importer },
	  { NULL, }
};

static bool traverse(xmlNode *root, struct parser_state *state)
{
	xmlNode *n;
	bool ret = true;

	for (n = root; n; n = n->next) {
		struct nesting *rule = nesting;

		if (!n->name) {
			if ((ret = visit(n, state)) == false)
				break;
			continue;
		}

		do {
			if (!strcmp(rule->name, (const char *)n->name))
				break;
			rule++;
		} while (rule->name);

		if (rule->start)
			rule->start(state);
		if ((ret = visit(n, state)) == false)
			break;
		if (rule->end)
			rule->end(state);
	}
	return ret;
}

/* Per-file reset */
static void reset_all(struct parser_state *state)
{
	/*
	 * We reset the units for each file. You'd think it was
	 * a per-dive property, but I'm not going to trust people
	 * to do per-dive setup. If the xml does have per-dive
	 * data within one file, we might have to reset it per
	 * dive for that format.
	 */
	state->xml_parsing_units = SI_units;
	state->import_source = parser_state::UNKNOWN;
}

/* divelog.de sends us xml files that claim to be iso-8859-1
 * but once we decode the HTML encoded characters they turn
 * into UTF-8 instead. So skip the incorrect encoding
 * declaration and decode the HTML encoded characters */
static const char *preprocess_divelog_de(const char *buffer)
{
	const char *ret = strstr(buffer, "<DIVELOGSDATA>");

	if (ret) {
		xmlParserCtxtPtr ctx;
		char buf[] = "";
		size_t i;

		for (i = 0; i < strlen(ret); ++i)
			if (!isascii(ret[i]))
				return buffer;

		ctx = xmlCreateMemoryParserCtxt(buf, sizeof(buf));
		ret = (char *)xmlStringLenDecodeEntities(ctx, (xmlChar *)ret, strlen(ret), XML_SUBSTITUTE_REF, 0, 0, 0);

		return ret;
	}
	return buffer;
}

int parse_xml_buffer(const char *url, const char *buffer, int, struct divelog *log,
				const struct xml_params *params)
{
	xmlDoc *doc;
	const char *res = preprocess_divelog_de(buffer);
	int ret = 0;
	struct parser_state state;

	state.log = log;
	state.fingerprints = &fingerprints; // simply use the global table for now
	doc = xmlReadMemory(res, strlen(res), url, NULL, XML_PARSE_HUGE);
	if (!doc)
		doc = xmlReadMemory(res, strlen(res), url, "latin1", XML_PARSE_HUGE);

	if (res != buffer)
		free((char *)res);

	if (!doc)
		return report_error(translate("gettextFromC", "Failed to parse '%s'"), url);

	reset_all(&state);
	dive_start(&state);
	doc = test_xslt_transforms(doc, params);
	if (!traverse(xmlDocGetRootElement(doc), &state)) {
		// we decided to give up on parsing... why?
		ret = -1;
	}
	dive_end(&state);
	xmlFreeDoc(doc);
	return ret;
}

void parse_xml_init()
{
	LIBXML_TEST_VERSION
}

void parse_xml_exit()
{
	xmlCleanupParser();
}

static struct xslt_files {
	const char *root;
	const char *file;
	const char *attribute;
} xslt_files[] = {
	  { "SUUNTO", "SuuntoSDM.xslt", NULL },
	  { "Dive", "SuuntoDM4.xslt", "xmlns" },
	  { "Dive", "shearwater.xslt", "version" },
	  { "JDiveLog", "jdivelog2subsurface.xslt", NULL },
	  { "dives", "MacDive.xslt", NULL },
	  { "DIVELOGSDATA", "divelogs.xslt", NULL },
	  { "uddf", "uddf.xslt", NULL },
	  { "UDDF", "uddf.xslt", NULL },
	  { "profile", "udcf.xslt", NULL },
	  { "Divinglog", "DivingLog.xslt", NULL },
	  { "csv", "csv2xml.xslt", NULL },
	  { "sensuscsv", "sensuscsv.xslt", NULL },
	  { "SubsurfaceCSV", "subsurfacecsv.xslt", NULL },
	  { "manualcsv", "manualcsv2xml.xslt", NULL },
	  { "logbook", "DiveLog.xslt", NULL },
	  { "AV1", "av1.xslt", NULL },
	  { "exportTrak", "Mares.xslt", NULL },
	  { NULL, }
  };

static xmlDoc *test_xslt_transforms(xmlDoc *doc, const struct xml_params *params)
{
	struct xslt_files *info = xslt_files;
	xmlDoc *transformed;
	xsltStylesheetPtr xslt = NULL;
	xmlNode *root_element = xmlDocGetRootElement(doc);
	xmlChar *attribute;

	if (!root_element)
		return NULL;

	while (info->root) {
		if ((strcasecmp((const char *)root_element->name, info->root) == 0)) {
			if (info->attribute == NULL)
				break;

			xmlChar *prop = xmlGetProp(root_element, (const xmlChar *)info->attribute);
			if (prop != NULL) {
				xmlFree(prop);

				break;
			}
		}
		info++;
	}

	if (info->root) {
		attribute = xmlGetProp(xmlFirstElementChild(root_element), (const xmlChar *)"name");
		if (attribute) {
			if (strcasecmp((char *)attribute, "subsurface") == 0) {
				xmlFree(attribute);
				return doc;
			}
			xmlFree(attribute);
		}
		xslt = get_stylesheet(info->file);
		if (xslt == NULL) {
			report_error(translate("gettextFromC", "Can't open stylesheet %s"), info->file);
			return doc;
		}
		transformed = xsltApplyStylesheet(xslt, doc, xml_params_get(params));
		xmlFreeDoc(doc);
		xsltFreeStylesheet(xslt);

		return transformed;
	}
	return doc;
}
