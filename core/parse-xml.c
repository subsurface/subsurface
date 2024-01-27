// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#define __USE_XOPEN
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
#include "subsurface-float.h"
#include "subsurface-string.h"
#include "subsurface-time.h"
#include "trip.h"
#include "device.h"
#include "membuffer.h"
#include "picture.h"
#include "qthelper.h"
#include "sample.h"
#include "tag.h"
#include "xmlparams.h"

int last_xml_version = -1;

static xmlDoc *test_xslt_transforms(xmlDoc *doc, const struct xml_params *params);

const struct units SI_units = SI_UNITS;
const struct units IMPERIAL_units = IMPERIAL_UNITS;

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
		fprintf(stderr, "Unable to parse date '%s'\n", buffer);
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
static void divedatetime(char *buffer, timestamp_t *when, struct parser_state *state)
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
static void divetags(char *buffer, struct tag_entry **tags)
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
					buffer[i] = '\0';
					state = FINDSTART;
					taglist_add_tag(tags, buffer + start);
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
			buffer[end + 1] = '\0';
			taglist_add_tag(tags, buffer + start);
		}
	}
}

enum number_type {
	NEITHER,
	FLOATVAL
};

static enum number_type parse_float(const char *buffer, double *res, const char **endp)
{
	double val;
	static bool first_time = true;

	errno = 0;
	val = ascii_strtod(buffer, endp);
	if (errno || *endp == buffer)
		return NEITHER;
	if (**endp == ',') {
		if (nearly_equal(val, rint(val))) {
			/* we really want to send an error if this is a Subsurface native file
			 * as this is likely indication of a bug - but right now we don't have
			 * that information available */
			if (first_time) {
				fprintf(stderr, "Floating point value with decimal comma (%s)?\n", buffer);
				first_time = false;
			}
			/* Try again in permissive mode*/
			val = strtod_flags(buffer, endp, 0);
		}
	}

	*res = val;
	return FLOATVAL;
}

union int_or_float {
	double fp;
};

static enum number_type integer_or_float(char *buffer, union int_or_float *res)
{
	const char *end;
	return parse_float(buffer, &res->fp, &end);
}

static void pressure(char *buffer, pressure_t *pressure, struct parser_state *state)
{
	double mbar = 0.0;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		/* Just ignore zero values */
		if (!val.fp)
			break;
		switch (state->xml_parsing_units.pressure) {
		case PASCALS:
			mbar = val.fp / 100;
			break;
		case BAR:
			/* Assume mbar, but if it's really small, it's bar */
			mbar = val.fp;
			if (fabs(mbar) < 5000)
				mbar = mbar * 1000;
			break;
		case PSI:
			mbar = psi_to_mbar(val.fp);
			break;
		}
		if (fabs(mbar) > 5 && fabs(mbar) < 5000000) {
			pressure->mbar = lrint(mbar);
			break;
		}
	/* fallthrough */
	default:
		printf("Strange pressure reading %s\n", buffer);
	}
}

static void cylinder_use(char *buffer, enum cylinderuse *cyl_use, struct parser_state *state)
{
	if (trimspace(buffer)) {
		int use = cylinderuse_from_text(buffer);
		*cyl_use = use;
		if (use == OXYGEN)
			state->o2pressure_sensor = state->cur_dive->cylinders.nr - 1;
	}
}

static void salinity(char *buffer, int *salinity)
{
	union int_or_float val;
	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		*salinity = lrint(val.fp * 10.0);
		break;
	default:
		printf("Strange salinity reading %s\n", buffer);
	}
}

static void depth(char *buffer, depth_t *depth, struct parser_state *state)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		switch (state->xml_parsing_units.length) {
		case METERS:
			depth->mm = lrint(val.fp * 1000);
			break;
		case FEET:
			depth->mm = feet_to_mm(val.fp);
			break;
		}
		break;
	default:
		printf("Strange depth reading %s\n", buffer);
	}
}

static void extra_data_start(struct parser_state *state)
{
	memset(&state->cur_extra_data, 0, sizeof(struct extra_data));
}

static void extra_data_end(struct parser_state *state)
{
	// don't save partial structures - we must have both key and value
	if (state->cur_extra_data.key && state->cur_extra_data.value)
		add_extra_data(get_dc(state), state->cur_extra_data.key, state->cur_extra_data.value);
	free((void *)state->cur_extra_data.key);
	free((void *)state->cur_extra_data.value);
	state->cur_extra_data.key = state->cur_extra_data.value = NULL;
}

static void weight(char *buffer, weight_t *weight, struct parser_state *state)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		switch (state->xml_parsing_units.weight) {
		case KG:
			weight->grams = lrint(val.fp * 1000);
			break;
		case LBS:
			weight->grams = lbs_to_grams(val.fp);
			break;
		}
		break;
	default:
		printf("Strange weight reading %s\n", buffer);
	}
}

static void temperature(char *buffer, temperature_t *temperature, struct parser_state *state)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		switch (state->xml_parsing_units.temperature) {
		case KELVIN:
			temperature->mkelvin = lrint(val.fp * 1000);
			break;
		case CELSIUS:
			temperature->mkelvin = C_to_mkelvin(val.fp);
			break;
		case FAHRENHEIT:
			temperature->mkelvin = F_to_mkelvin(val.fp);
			break;
		}
		break;
	default:
		printf("Strange temperature reading %s\n", buffer);
	}
	/* temperatures outside -40C .. +70C should be ignored */
	if (temperature->mkelvin < ZERO_C_IN_MKELVIN - 40000 ||
	    temperature->mkelvin > ZERO_C_IN_MKELVIN + 70000)
		temperature->mkelvin = 0;
}

static void sampletime(char *buffer, duration_t *time)
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
		time->seconds = 0;
		printf("Strange sample time reading %s\n", buffer);
	}
}

static void offsettime(char *buffer, offset_t *time)
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

static void duration(char *buffer, duration_t *time)
{
	/* DivingLog 5.08 (and maybe other versions) appear to sometimes
	 * store the dive time as 44.00 instead of 44:00;
	 * This attempts to parse this in a fairly robust way */
	if (!strchr(buffer, ':') && strchr(buffer, '.')) {
		char *mybuffer = strdup(buffer);
		char *dot = strchr(mybuffer, '.');
		*dot = ':';
		sampletime(mybuffer, time);
		free(mybuffer);
	} else {
		sampletime(buffer, time);
	}
}

static void percent(char *buffer, fraction_t *fraction)
{
	double val;
	const char *end;

	switch (parse_float(buffer, &val, &end)) {
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
		printf(translate("gettextFromC", "Strange percentage reading %s\n"), buffer);
		break;
	}
}

static void gasmix(char *buffer, fraction_t *fraction, struct parser_state *state)
{
	/* libdivecomputer does negative percentages. */
	if (*buffer == '-')
		return;
	percent(buffer, fraction);
}

static void gasmix_nitrogen(char *buffer, struct gasmix *gasmix)
{
	UNUSED(buffer);
	UNUSED(gasmix);
	/* Ignore n2 percentages. There's no value in them. */
}

static void cylindersize(char *buffer, volume_t *volume)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		volume->mliter = lrint(val.fp * 1000);
		break;

	default:
		printf("Strange volume reading %s\n", buffer);
		break;
	}
}

static void event_name(char *buffer, char *name)
{
	int size = trimspace(buffer);
	if (size >= MAX_EVENT_NAME)
		size = MAX_EVENT_NAME - 1;
	memcpy(name, buffer, size);
	name[size] = 0;
}

// We don't use gauge as a mode, and pscr doesn't exist as a libdc divemode
const char *libdc_divemode_text[] = { "oc", "cc", "pscr", "freedive", "gauge"};

/* Extract the dive computer type from the xml text buffer */
static void get_dc_type(char *buffer, enum divemode_t *dct)
{
	if (trimspace(buffer)) {
		for (enum divemode_t i = 0; i < NUM_DIVEMODE; i++) {
			if (strcmp(buffer, divemode_text[i]) == 0)
				*dct = i;
			else if (strcmp(buffer, libdc_divemode_text[i]) == 0)
				*dct = i;
		}
	}
}

/* For divemode_text[] (defined in dive.h) determine the index of
 * the string contained in the xml divemode attribute and passed
 * in buffer, below. Typical xml input would be:
 * <event name='modechange' divemode='OC' /> */
static void event_divemode(char *buffer, int *value)
{
	int size = trimspace(buffer);
	if (size >= MAX_EVENT_NAME)
		size = MAX_EVENT_NAME - 1;
	buffer[size] = 0x0;
	for (int i = 0; i < NUM_DIVEMODE; i++) {
		if (!strcmp(buffer,divemode_text[i])) {
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

typedef void (*matchfn_t)(char *buffer, void *);
static int match(const char *pattern, const char *name,
		 matchfn_t fn, char *buf, void *data)
{
	if (!match_name(pattern, name))
		return 0;
	fn(buf, data);
	return 1;
}

typedef void (*matchfn_state_t)(char *buffer, void *, struct parser_state *state);
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

static void get_index(char *buffer, int *i)
{
	*i = atoi(buffer);
}

static void get_bool(char *buffer, bool *i)
{
	*i = atoi(buffer);
}

static void get_uint8(char *buffer, uint8_t *i)
{
	*i = atoi(buffer);
}

static void get_uint16(char *buffer, uint16_t *i)
{
	*i = atoi(buffer);
}

static void get_bearing(char *buffer, bearing_t *bearing)
{
	bearing->degrees = atoi(buffer);
}

static void get_rating(char *buffer, int *i)
{
	int j = atoi(buffer);
	if (j >= 0 && j <= 5) {
		*i = j;
	}
}

static void double_to_o2pressure(char *buffer, o2pressure_t *i)
{
	i->mbar = lrint(ascii_strtod(buffer, NULL) * 1000.0);
}

static void hex_value(char *buffer, uint32_t *i)
{
	*i = strtoul(buffer, NULL, 16);
}

static void dive_site(char *buffer, struct dive *d, struct parser_state *state)
{
	uint32_t uuid;
	hex_value(buffer, &uuid);
	add_dive_to_dive_site(d, get_dive_site_by_uuid(uuid, state->log->sites));
}

static void get_notrip(char *buffer, bool *notrip)
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
static void fahrenheit(char *buffer, temperature_t *temperature)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		if (nearly_equal(val.fp, 32.0))
			break;
		if (val.fp < 32.0)
			temperature->mkelvin = C_to_mkelvin(val.fp);
		else
			temperature->mkelvin = F_to_mkelvin(val.fp);
		break;
	default:
		fprintf(stderr, "Crazy Diving Log temperature reading %s\n", buffer);
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
static void psi_or_bar(char *buffer, pressure_t *pressure)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOATVAL:
		if (val.fp > 400)
			pressure->mbar = psi_to_mbar(val.fp);
		else
			pressure->mbar = lrint(val.fp * 1000);
		break;
	default:
		fprintf(stderr, "Crazy Diving Log PSI reading %s\n", buffer);
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

static void uddf_gasswitch(char *buffer, struct sample *sample, struct parser_state *state)
{
	int idx = atoi(buffer);
	int seconds = sample->time.seconds;
	struct dive *dive = state->cur_dive;
	struct divecomputer *dc = get_dc(state);

	add_gas_switch_event(dive, dc, seconds, idx);
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

static void eventtime(char *buffer, duration_t *duration, struct parser_state *state)
{
	sampletime(buffer, duration);
	if (state->cur_sample)
		duration->seconds += state->cur_sample->time.seconds;
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

static void get_cylinderindex(char *buffer, int16_t *i, struct parser_state *state)
{
	*i = atoi(buffer);
	if (state->lastcylinderindex != *i) {
		add_gas_switch_event(state->cur_dive, get_dc(state), state->cur_sample->time.seconds, *i);
		state->lastcylinderindex = *i;
	}
}

static void get_sensor(char *buffer, int16_t *i)
{
	*i = atoi(buffer);
}

static void parse_libdc_deco(char *buffer, struct sample *s)
{
	if (strcmp(buffer, "deco") == 0) {
		s->in_deco = true;
	} else if (strcmp(buffer, "ndl") == 0) {
		s->in_deco = false;
		// The time wasn't stoptime, it was ndl
		s->ndl = s->stoptime;
		s->stoptime.seconds = 0;
	}
}

static void try_to_fill_dc_settings(const char *name, char *buf, struct parser_state *state)
{
	start_match("divecomputerid", name, buf);
	if (MATCH("model.divecomputerid", utf8_string, &state->cur_settings.dc.model))
		return;
	if (MATCH("deviceid.divecomputerid", hex_value, &state->cur_settings.dc.deviceid))
		return;
	if (MATCH("nickname.divecomputerid", utf8_string, &state->cur_settings.dc.nickname))
		return;
	if (MATCH("serial.divecomputerid", utf8_string, &state->cur_settings.dc.serial_nr))
		return;
	if (MATCH("firmware.divecomputerid", utf8_string, &state->cur_settings.dc.firmware))
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
	if (MATCH("data.fingerprint", utf8_string, &state->cur_settings.fingerprint.data))
		return;
	nonmatch("fingerprint", name, buf);
}

static void try_to_fill_event(const char *name, char *buf, struct parser_state *state)
{
	start_match("event", name, buf);
	if (MATCH("event", event_name, state->cur_event.name))
		return;
	if (MATCH("name", event_name, state->cur_event.name))
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
	if (MATCH("key.extradata", utf8_string, &state->cur_extra_data.key))
		return 1;
	if (MATCH("value.extradata", utf8_string, &state->cur_extra_data.value))
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
	if (MATCH("model", utf8_string, &dc->model))
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
	case DIVINGLOG:
		if (divinglog_fill_sample(sample, name, buf, state))
			return;
		break;
	case UDDF:
		if (uddf_fill_sample(sample, name, buf, state))
			return;
		break;

	default:
		break;
	}

	nonmatch("sample", name, buf);
}

static void divinglog_place(char *place, struct dive *d, struct parser_state *state)
{
	char buffer[1024];
	struct dive_site *ds;

	snprintf(buffer, sizeof(buffer),
		 "%s%s%s%s%s",
		 place,
		 state->city ? ", " : "",
		 state->city ? state->city : "",
		 state->country ? ", " : "",
		 state->country ? state->country : "");
	ds = get_dive_site_by_name(buffer, state->log->sites);
	if (!ds)
		ds = create_dive_site(buffer, state->log->sites);
	add_dive_to_dive_site(d, ds);

	// TODO: capture the country / city info in the taxonomy instead
	free(state->city);
	free(state->country);
	state->city = NULL;
	state->country = NULL;
}

static int divinglog_dive_match(struct dive *dive, const char *name, char *buf, struct parser_state *state)
{
	/* For cylinder related fields, we might have to create a cylinder first. */
	cylinder_t cyl = empty_cylinder;
	if (MATCH("tanktype", utf8_string, &cyl.type.description)) {
		cylinder_t *cyl0 = get_or_create_cylinder(dive, 0);
		free((void *)cyl0->type.description);
		cyl0->type.description = cyl.type.description;
		return 1;
	}
	if (MATCH("tanksize", cylindersize, &cyl.type.size)) {
		get_or_create_cylinder(dive, 0)->type.size = cyl.type.size;
		return 1;
	}
	if (MATCH_STATE("presw", pressure, &cyl.type.workingpressure)) {
		get_or_create_cylinder(dive, 0)->type.workingpressure = cyl.type.workingpressure;
		return 1;
	}
	if (MATCH_STATE("press", pressure, &cyl.start)) {
		get_or_create_cylinder(dive, 0)->start = cyl.start;
		return 1;
	}
	if (MATCH_STATE("prese", pressure, &cyl.end)) {
		get_or_create_cylinder(dive, 0)->end = cyl.end;
		return 1;
	}
	return MATCH_STATE("divedate", divedate, &dive->when) ||
	       MATCH_STATE("entrytime", divetime, &dive->when) ||
	       MATCH("divetime", duration, &dive->dc.duration) ||
	       MATCH_STATE("depth", depth, &dive->dc.maxdepth) ||
	       MATCH_STATE("depthavg", depth, &dive->dc.meandepth) ||
	       MATCH("comments", utf8_string, &dive->notes) ||
	       MATCH("names.buddy", utf8_string, &dive->buddy) ||
	       MATCH("name.country", utf8_string, &state->country) ||
	       MATCH("name.city", utf8_string, &state->city) ||
	       MATCH_STATE("name.place", divinglog_place, dive) ||
	       0;
}

/*
 * Uddf specifies ISO 8601 time format.
 *
 * There are many variations on that. This handles the useful cases.
 */
static void uddf_datetime(char *buffer, timestamp_t *when, struct parser_state *state)
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
	printf("Bad date time %s\n", buffer);
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

#define uddf_datedata(name, offset)                                                          \
	static void uddf_##name(char *buffer, timestamp_t *when, struct parser_state *state) \
	{                                                                                    \
		state->cur_tm.tm_##name = atoi(buffer) + offset;                             \
		*when = utc_mktime(&state->cur_tm);                                          \
	}

uddf_datedata(year, 0)
uddf_datedata(mon, -1)
uddf_datedata(mday, 0)
uddf_datedata(hour, 0)
uddf_datedata(min, 0)

static int uddf_dive_match(struct dive *dive, const char *name, char *buf, struct parser_state *state)
{
	return MATCH_STATE("datetime", uddf_datetime, &dive->when) ||
	       MATCH("diveduration", duration, &dive->dc.duration) ||
	       MATCH_STATE("greatestdepth", depth, &dive->dc.maxdepth) ||
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

static void gps_lat(char *buffer, struct dive *dive, struct parser_state *state)
{
	const char *end;
	location_t location = { };
	struct dive_site *ds = get_dive_site_for_dive(dive);

	location.lat = parse_degrees(buffer, &end);
	if (!ds) {
		add_dive_to_dive_site(dive, create_dive_site_with_gps(NULL, &location, state->log->sites));
	} else {
		if (ds->location.lat.udeg && ds->location.lat.udeg != location.lat.udeg)
			fprintf(stderr, "Oops, changing the latitude of existing dive site id %8x name %s; not good\n", ds->uuid, ds->name ?: "(unknown)");
		ds->location.lat = location.lat;
	}
}

static void gps_long(char *buffer, struct dive *dive, struct parser_state *state)
{
	const char *end;
	location_t location = { };
	struct dive_site *ds = get_dive_site_for_dive(dive);

	location.lon = parse_degrees(buffer, &end);
	if (!ds) {
		add_dive_to_dive_site(dive, create_dive_site_with_gps(NULL, &location, state->log->sites));
	} else {
		if (ds->location.lon.udeg && ds->location.lon.udeg != location.lon.udeg)
			fprintf(stderr, "Oops, changing the longitude of existing dive site id %8x name %s; not good\n", ds->uuid, ds->name ?: "(unknown)");
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

static void gps_location(char *buffer, struct dive_site *ds)
{
	parse_location(buffer, &ds->location);
}

static void gps_in_dive(char *buffer, struct dive *dive, struct parser_state *state)
{
	struct dive_site *ds = dive->dive_site;
	location_t location;

	parse_location(buffer, &location);
	if (!ds) {
		// check if we have a dive site within 20 meters of that gps fix
		ds = get_dive_site_by_gps_proximity(&location, 20, state->log->sites);

		if (ds) {
			// found a site nearby; in case it turns out this one had a different name let's
			// remember the original coordinates so we can create the correct dive site later
			state->cur_location = location;
		} else {
			ds = create_dive_site_with_gps("", &location, state->log->sites);
		}
		add_dive_to_dive_site(dive, ds);
	} else {
		if (dive_site_has_gps_location(ds) &&
		    has_location(&location) && !same_location(&ds->location, &location)) {
			// Houston, we have a problem
			fprintf(stderr, "dive site uuid in dive, but gps location (%10.6f/%10.6f) different from dive location (%10.6f/%10.6f)\n",
				ds->location.lat.udeg / 1000000.0, ds->location.lon.udeg / 1000000.0,
				location.lat.udeg / 1000000.0, location.lon.udeg / 1000000.0);
			char *coords = printGPSCoordsC(&location);
			ds->notes = add_to_string(ds->notes, translate("gettextFromC", "multiple GPS locations for this dive site; also %s\n"), coords);
			free(coords);
		} else {
			ds->location = location;
		}
	}
}

static void gps_picture_location(char *buffer, struct picture *pic)
{
	parse_location(buffer, &pic->location);
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dive(struct dive *dive, const char *name, char *buf, struct parser_state *state)
{
	char *hash = NULL;
	cylinder_t *cyl = dive->cylinders.nr > 0 ? get_cylinder(dive, dive->cylinders.nr - 1) : NULL;
	weightsystem_t *ws = dive->weightsystems.nr > 0 ?
		&dive->weightsystems.weightsystems[dive->weightsystems.nr - 1] : NULL;
	pressure_t p;
	weight_t w;
	start_match("dive", name, buf);

	switch (state->import_source) {
	case DIVINGLOG:
		if (divinglog_dive_match(dive, name, buf, state))
			return;
		break;

	case UDDF:
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
	if (MATCH("tags", divetags, &dive->tag_list))
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
	if (match_dc_data_fields(&dive->dc, name, buf, state))
		return;

	if (MATCH("filename.picture", utf8_string, &state->cur_picture.filename))
		return;
	if (MATCH("offset.picture", offsettime, &state->cur_picture.offset))
		return;
	if (MATCH("gps.picture", gps_picture_location, &state->cur_picture))
		return;
	if (MATCH("hash.picture", utf8_string, &hash)) {
		/* Legacy -> ignore. */
		free(hash);
		return;
	}
	if (MATCH_STATE("cylinderstartpressure", pressure, &p)) {
		get_or_create_cylinder(dive, 0)->start = p;
		return;
	}
	if (MATCH_STATE("cylinderendpressure", pressure, &p)) {
		get_or_create_cylinder(dive, 0)->end = p;
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
	if (MATCH("suit", utf8_string, &dive->suit))
		return;
	if (MATCH("divesuit", utf8_string, &dive->suit))
		return;
	if (MATCH("notes", utf8_string, &dive->notes))
		return;
	// For historic reasons, we accept dive guide as well as dive master
	if (MATCH("diveguide", utf8_string, &dive->diveguide))
		return;
	if (MATCH("divemaster", utf8_string, &dive->diveguide))
		return;
	if (MATCH("buddy", utf8_string, &dive->buddy))
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
		if (MATCH("description.weightsystem", utf8_string, &ws->description))
			return;
		if (MATCH_STATE("weight.weightsystem", weight, &ws->weight))
			return;
	}
	if (MATCH_STATE("weight", weight, &w)) {
		weightsystem_t ws = empty_weightsystem;
		ws.weight = w;
		add_cloned_weightsystem(&dive->weightsystems, ws);
		return;
	}
	if (cyl) {
		if (MATCH("size.cylinder", cylindersize, &cyl->type.size))
			return;
		if (MATCH_STATE("workpressure.cylinder", pressure, &cyl->type.workingpressure))
			return;
		if (MATCH("description.cylinder", utf8_string, &cyl->type.description))
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
static void try_to_fill_trip(dive_trip_t *dive_trip, const char *name, char *buf, struct parser_state *state)
{
	start_match("trip", name, buf);

	if (MATCH("location", utf8_string, &dive_trip->location))
		return;
	if (MATCH("notes", utf8_string, &dive_trip->notes))
		return;

	nonmatch("trip", name, buf);
}

/* We're processing a divesite entry - try to fill the components */
static void try_to_fill_dive_site(struct parser_state *state, const char *name, char *buf)
{
	struct dive_site *ds = state->cur_dive_site;
	char *taxonomy_value = NULL;

	start_match("divesite", name, buf);

	if (MATCH("uuid", hex_value, &ds->uuid))
		return;
	if (MATCH("name", utf8_string, &ds->name))
		return;
	if (MATCH("description", utf8_string, &ds->description))
		return;
	if (MATCH("notes", utf8_string, &ds->notes))
		return;
	if (MATCH("gps", gps_location, ds))
		return;
	if (MATCH("cat.geo", get_index, &state->taxonomy_category))
		return;
	if (MATCH("origin.geo", get_index, &state->taxonomy_origin))
		return;
	if (MATCH("value.geo", utf8_string, &taxonomy_value)) {
		/* The code assumes that "value.geo" comes last, which is against
		 * the expectations of an XML file. Let's at least make sure that
		 * cat and origin have been set! */
		if (state->taxonomy_category < 0 || state->taxonomy_origin < 0) {
			report_error("Warning: taxonomy value without origin or category");
		} else {
			taxonomy_set_category(&ds->taxonomy, state->taxonomy_category,
					      taxonomy_value, state->taxonomy_origin);
		}
		state->taxonomy_category = state->taxonomy_origin = -1;
		free(taxonomy_value);
		return;
	}

	nonmatch("divesite", name, buf);
}

static void try_to_fill_filter(struct filter_preset *filter, const char *name, char *buf)
{
	start_match("filterpreset", name, buf);

	char *s = NULL;
	if (MATCH("name", utf8_string, &s)) {
		filter_preset_set_name(filter, s);
		free(s);
		return;
	}

	nonmatch("filterpreset", name, buf);
}

static void try_to_fill_fulltext(const char *name, char *buf, struct parser_state *state)
{
	start_match("fulltext", name, buf);

	if (MATCH("mode", utf8_string, &state->fulltext_string_mode))
		return;
	if (MATCH("fulltext", utf8_string, &state->fulltext))
		return;

	nonmatch("fulltext", name, buf);
}

static void try_to_fill_filter_constraint(const char *name, char *buf, struct parser_state *state)
{
	start_match("fulltext", name, buf);

	if (MATCH("type", utf8_string, &state->filter_constraint_type))
		return;
	if (MATCH("string_mode", utf8_string, &state->filter_constraint_string_mode))
		return;
	if (MATCH("range_mode", utf8_string, &state->filter_constraint_range_mode))
		return;
	if (MATCH("negate", get_bool, &state->filter_constraint_negate))
		return;
	if (MATCH("constraint", utf8_string, &state->filter_constraint))
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
		try_to_fill_filter(state->cur_filter, name, buf);
		return true;
	}
	if (!state->cur_event.deleted) {
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
		try_to_fill_dive(state->cur_dive, name, buf, state);
		return true;
	}
	if (state->cur_trip) {
		try_to_fill_trip(state->cur_trip, name, buf, state);
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
	state->import_source = DIVINGLOG;

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
	state->import_source = UDDF;
	state->xml_parsing_units = SI_units;
	state->xml_parsing_units.pressure = PASCALS;
	state->xml_parsing_units.temperature = KELVIN;
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
	state->import_source = UNKNOWN;
}

/* divelog.de sends us xml files that claim to be iso-8859-1
 * but once we decode the HTML encoded characters they turn
 * into UTF-8 instead. So skip the incorrect encoding
 * declaration and decode the HTML encoded characters */
static const char *preprocess_divelog_de(const char *buffer)
{
	char *ret = strstr(buffer, "<DIVELOGSDATA>");

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

int parse_xml_buffer(const char *url, const char *buffer, int size, struct divelog *log,
		     const struct xml_params *params)
{
	UNUSED(size);
	xmlDoc *doc;
	const char *res = preprocess_divelog_de(buffer);
	int ret = 0;
	struct parser_state state;

	init_parser_state(&state);
	state.log = log;
	state.fingerprints = &fingerprint_table; // simply use the global table for now
	doc = xmlReadMemory(res, strlen(res), url, NULL, XML_PARSE_HUGE | XML_PARSE_RECOVER);
	if (!doc)
		doc = xmlReadMemory(res, strlen(res), url, "latin1", XML_PARSE_HUGE | XML_PARSE_RECOVER);

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
	free_parser_state(&state);
	xmlFreeDoc(doc);
	return ret;
}

/*
 * Parse a unsigned 32-bit integer in little-endian mode,
 * that is seconds since Jan 1, 2000.
 */
static timestamp_t parse_dlf_timestamp(unsigned char *buffer)
{
	timestamp_t offset;

	offset = buffer[3];
	offset = (offset << 8) + buffer[2];
	offset = (offset << 8) + buffer[1];
	offset = (offset << 8) + buffer[0];

	// Jan 1, 2000 is 946684800 seconds after Jan 1, 1970, which is
	// the Unix epoch date that "timestamp_t" uses.
	return offset + 946684800;
}

int parse_dlf_buffer(unsigned char *buffer, size_t size, struct divelog *log)
{
	unsigned char *ptr = buffer;
	unsigned char event;
	bool found;
	unsigned int time = 0;
	int i;
	char serial[6];
	struct battery_status {
		uint16_t volt1;
		uint8_t percent1;
		uint16_t volt2;
		uint8_t percent2;
	};
	struct battery_status battery_start = {0, 0, 0, 0};
	struct battery_status battery_end = {0, 0, 0, 0};
	uint16_t o2_sensor_calibration_values[4] = {0};
	cylinder_t *cyl;
	struct parser_state state;

	init_parser_state(&state);
	state.log = log;

	// Check for the correct file magic
	if (ptr[0] != 'D' || ptr[1] != 'i' || ptr[2] != 'v' || ptr[3] != 'E')
		return -1;

	dive_start(&state);
	divecomputer_start(&state);

	state.cur_dc->model = strdup("DLF import");
	// (ptr[7] << 8) + ptr[6] Is "Serial"
	snprintf(serial, sizeof(serial), "%d", (ptr[7] << 8) + ptr[6]);
	state.cur_dc->serial = strdup(serial);
	state.cur_dc->when = parse_dlf_timestamp(ptr + 8);
	state.cur_dive->when = state.cur_dc->when;

	state.cur_dc->duration.seconds = ((ptr[14] & 0xFE) << 16) + (ptr[13] << 8) + ptr[12];

	// ptr[14] >> 1 is scrubber used in %

	// 3 bit dive type
	switch((ptr[15] & 0x38) >> 3) {
	case 0: // unknown
	case 1:
		state.cur_dc->divemode = OC;
		break;
	case 2:
		state.cur_dc->divemode = CCR;
		break;
	case 3:
		state.cur_dc->divemode = CCR; // mCCR
		break;
	case 4:
		state.cur_dc->divemode = FREEDIVE;
		break;
	case 5:
		state.cur_dc->divemode = OC; // Gauge
		break;
	case 6:
		state.cur_dc->divemode = PSCR; // ASCR
		break;
	case 7:
		state.cur_dc->divemode = PSCR;
		break;
	}

	state.cur_dc->maxdepth.mm = ((ptr[21] << 8) + ptr[20]) * 10;
	state.cur_dc->surface_pressure.mbar = ((ptr[25] << 8) + ptr[24]) / 10;

	// Declare initial mix as first cylinder
	cyl = get_or_create_cylinder(state.cur_dive, 0);
	cyl->gasmix.o2.permille = ptr[26] * 10;
	cyl->gasmix.he.permille = ptr[27] * 10;

	/* Done with parsing what we know about the dive header */
	ptr += 32;

	// We're going to interpret ppO2 saved as a sensor value in these modes.
	if (state.cur_dc->divemode == CCR || state.cur_dc->divemode == PSCR)
		state.cur_dc->no_o2sensors = 1;

	for (; ptr < buffer + size; ptr += 16) {
		time = ((ptr[0] >> 4) & 0x0f) +
			((ptr[1] << 4) & 0xff0) +
			((ptr[2] << 12) & 0x1f000);
		event = ptr[0] & 0x0f;
		switch (event) {
		case 0:
			/* Regular sample */
			sample_start(&state);
			state.cur_sample->time.seconds = time;
			state.cur_sample->depth.mm = ((ptr[5] << 8) + ptr[4]) * 10;
			// Crazy precision on these stored values...
			// Only store value if we're in CCR/PSCR mode,
			// because we rather calculate ppo2 our selfs.
			if (state.cur_dc->divemode == CCR || state.cur_dc->divemode == PSCR)
				state.cur_sample->o2sensor[0].mbar = ((ptr[7] << 8) + ptr[6]) / 10;

			// In some test files, ndl / tts / temp is bogus if this bits are 1
			// flag bits in ptr[11] & 0xF0 is probably involved to,
			if ((ptr[2] >> 5) != 1) {
				// NDL in minutes, 10 bit
				state.cur_sample->ndl.seconds = (((ptr[9] & 0x03) << 8) + ptr[8]) * 60;
				// TTS in minutes, 10 bit
				state.cur_sample->tts.seconds = (((ptr[10] & 0x0F) << 6) + (ptr[9] >> 2)) * 60;
				// Temperature in 1/10 C, 10 bit signed
				state.cur_sample->temperature.mkelvin = ((ptr[11] & 0x20) ? -1 : 1)  * (((ptr[11] & 0x1F) << 4) + (ptr[10] >> 4)) * 100 + ZERO_C_IN_MKELVIN;
			}
			state.cur_sample->stopdepth.mm = ((ptr[13] << 8) + ptr[12]) * 10;
			if (state.cur_sample->stopdepth.mm)
				state.cur_sample->in_deco = true;
			//ptr[14] is helium content, always zero?
			//ptr[15] is setpoint, what the computer thinks you should aim for?
			sample_end(&state);
			break;
		case 1: /* dive event */
		case 2: /* automatic parameter change */
		case 3: /* diver error */
		case 4: /* internal error */
		case 5: /* device activity log */
			//Event 18 is a button press. Lets ingore that event.
			if (ptr[4] == 18)
				continue;

			event_start(&state);
			state.cur_event.time.seconds = time;
			switch (ptr[4]) {
			case 1:
				strcpy(state.cur_event.name, "Setpoint Manual");
				state.cur_event.value = ptr[6];
				sample_start(&state);
				state.cur_sample->setpoint.mbar = ptr[6] * 10;
				sample_end(&state);
				break;
			case 2:
				strcpy(state.cur_event.name, "Setpoint Auto");
				state.cur_event.value = ptr[6];
				sample_start(&state);
				state.cur_sample->setpoint.mbar = ptr[6] * 10;
				sample_end(&state);
				switch (ptr[7]) {
				case 0:
					strcat(state.cur_event.name, " Manual");
					break;
				case 1:
					strcat(state.cur_event.name, " Auto Start");
					break;
				case 2:
					strcat(state.cur_event.name, " Auto Hypox");
					break;
				case 3:
					strcat(state.cur_event.name, " Auto Timeout");
					break;
				case 4:
					strcat(state.cur_event.name, " Auto Ascent");
					break;
				case 5:
					strcat(state.cur_event.name, " Auto Stall");
					break;
				case 6:
					strcat(state.cur_event.name, " Auto SP Low");
					break;
				default:
					break;
				}
				break;
			case 3:
				// obsolete
				strcpy(state.cur_event.name, "OC");
				break;
			case 4:
				// obsolete
				strcpy(state.cur_event.name, "CCR");
				break;
			case 5:
				strcpy(state.cur_event.name, "gaschange");
				state.cur_event.type = SAMPLE_EVENT_GASCHANGE2;
				state.cur_event.value = ptr[7] << 8 ^ ptr[6];

				found = false;
				for (i = 0; i < state.cur_dive->cylinders.nr; ++i) {
					const cylinder_t *cyl = get_cylinder(state.cur_dive, i);
					if (cyl->gasmix.o2.permille == ptr[6] * 10 && cyl->gasmix.he.permille == ptr[7] * 10) {
						found = true;
						break;
					}
				}
				if (!found) {
					cyl = cylinder_start(&state);
					cyl->gasmix.o2.permille = ptr[6] * 10;
					cyl->gasmix.he.permille = ptr[7] * 10;
					cylinder_end(&state);
					state.cur_event.gas.index = state.cur_dive->cylinders.nr - 1;
				} else {
					state.cur_event.gas.index = i;
				}
				break;
			case 6:
				strcpy(state.cur_event.name, "Start");
				break;
			case 7:
				strcpy(state.cur_event.name, "Too Fast");
				break;
			case 8:
				strcpy(state.cur_event.name, "Above Ceiling");
				break;
			case 9:
				strcpy(state.cur_event.name, "Toxic");
				break;
			case 10:
				strcpy(state.cur_event.name, "Hypox");
				break;
			case 11:
				strcpy(state.cur_event.name, "Critical");
				break;
			case 12:
				strcpy(state.cur_event.name, "Sensor Disabled");
				break;
			case 13:
				strcpy(state.cur_event.name, "Sensor Enabled");
				break;
			case 14:
				strcpy(state.cur_event.name, "O2 Backup");
				break;
			case 15:
				strcpy(state.cur_event.name, "Peer Down");
				break;
			case 16:
				strcpy(state.cur_event.name, "HS Down");
				break;
			case 17:
				strcpy(state.cur_event.name, "Inconsistent");
				break;
			case 18:
				// key pressed - It should never get in here
				// as we ingored it at the parent 'case 5'.
				break;
			case 19:
				// obsolete
				strcpy(state.cur_event.name, "SCR");
				break;
			case 20:
				strcpy(state.cur_event.name, "Above Stop");
				break;
			case 21:
				strcpy(state.cur_event.name, "Safety Miss");
				break;
			case 22:
				strcpy(state.cur_event.name, "Fatal");
				break;
			case 23:
				strcpy(state.cur_event.name, "gaschange");
				state.cur_event.type = SAMPLE_EVENT_GASCHANGE2;
				state.cur_event.value = ptr[7] << 8 ^ ptr[6];
				event_end(&state);
				break;
			case 24:
				strcpy(state.cur_event.name, "gaschange");
				state.cur_event.type = SAMPLE_EVENT_GASCHANGE2;
				state.cur_event.value = ptr[7] << 8 ^ ptr[6];
				event_end(&state);
				// This is both a mode change and a gas change event
				// so we encode it as two separate events.
				event_start(&state);
				strcpy(state.cur_event.name, "Change Mode");
				switch (ptr[8]) {
				case 1:
					strcat(state.cur_event.name, ": OC");
					break;
				case 2:
					strcat(state.cur_event.name, ": CCR");
					break;
				case 3:
					strcat(state.cur_event.name, ": mCCR");
					break;
				case 4:
					strcat(state.cur_event.name, ": Free");
					break;
				case 5:
					strcat(state.cur_event.name, ": Gauge");
					break;
				case 6:
					strcat(state.cur_event.name, ": ASCR");
					break;
				case 7:
					strcat(state.cur_event.name, ": PSCR");
					break;
				default:
					break;
				}
				event_end(&state);
				break;
			case 25:
				// uint16_t solenoid_bitmap = (ptr[7] << 8) + (ptr[6] << 0);
				// uint32_t time = (ptr[11] << 24) + (ptr[10] << 16) + (ptr[9] << 8) + (ptr[8] << 0);
				snprintf(state.cur_event.name, MAX_EVENT_NAME, "CCR O2 solenoid %s", ptr[12] ? "opened": "closed");
				break;
			case 26:
				strcpy(state.cur_event.name, "User mark");
				break;
			case 27:
				snprintf(state.cur_event.name, MAX_EVENT_NAME, "%sGF Switch (%d/%d)", ptr[6] ? "Bailout, ": "", ptr[7], ptr[8]);
				break;
			case 28:
				strcpy(state.cur_event.name, "Peer Up");
				break;
			case 29:
				strcpy(state.cur_event.name, "HS Up");
				break;
			case 30:
				snprintf(state.cur_event.name, MAX_EVENT_NAME, "CNS %d%%", ptr[6]);
				break;
			default:
				// No values above 30 had any description
				break;
			}
			event_end(&state);
			break;
		case 6:
			/* device configuration */
			switch (((ptr[3] & 0x7f) << 3) + ((ptr[2] & 0xe0) >> 5)) {
				// Buffer to print extra string into
				char config_buf[256];
				// Local variables to temporary decode into
				struct tm tm;
				char *device;
				char *deep_stops;
				case 0: // TEST_CCR_FULL_1
					utc_mkdate(parse_dlf_timestamp(ptr + 12), &tm);
					snprintf(config_buf, sizeof(config_buf), "START=%04u-%02u-%02u %02u:%02u:%02u,TEST=%02X%02X%02X%02X,RESULT=%02X%02X%02X%02X", tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ptr[7], ptr[6], ptr[5], ptr[4], ptr[11], ptr[10], ptr[9], ptr[8]);
					add_extra_data(state.cur_dc, "TEST_CCR_FULL_1", config_buf);
					break;
				case 1: // TEST_CCR_PARTIAL_1
					utc_mkdate(parse_dlf_timestamp(ptr + 12), &tm);
					snprintf(config_buf, sizeof(config_buf), "START=%04u-%02u-%02u %02u:%02u:%02u,TEST=%02X%02X%02X%02X,RESULT=%02X%02X%02X%02X", tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ptr[7], ptr[6], ptr[5], ptr[4], ptr[11], ptr[10], ptr[9], ptr[8]);
					add_extra_data(state.cur_dc, "TEST_CCR_PARTIAL_1", config_buf);
					break;
				case 2: // CFG_OXYGEN_CALIBRATION
					utc_mkdate(parse_dlf_timestamp(ptr + 12), &tm);
					o2_sensor_calibration_values[0] = (ptr[5] << 8) + ptr[4];
					o2_sensor_calibration_values[1] = (ptr[7] << 8) + ptr[6];
					o2_sensor_calibration_values[2] = (ptr[9] << 8) + ptr[8];
					o2_sensor_calibration_values[3] = (ptr[11] << 8) + ptr[10];
					snprintf(config_buf, sizeof(config_buf), "%04u,%04u,%04u,%04u,TIME=%04u-%02u-%02u %02u:%02u:%02u", o2_sensor_calibration_values[0], o2_sensor_calibration_values[1], o2_sensor_calibration_values[2], o2_sensor_calibration_values[3], tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
					add_extra_data(state.cur_dc, "CFG_OXYGEN_CALIBRATION", config_buf);
					break;
				case 3: // CFG_SERIAL
					snprintf(config_buf, sizeof(config_buf), "PRODUCT=%c%c%c%c,SERIAL=%c%c%c%c%c%c%c%c", ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]);
					add_extra_data(state.cur_dc, "CFG_SERIAL", config_buf);
					break;
				case 4: // CFG_CONFIG_DECO
					switch ((ptr[5] & 0xC0) >> 6) {
						case 0:
							deep_stops = "none";
							break;
						case 1:
							deep_stops = "Pyle";
							break;
						case 2:
							deep_stops = "Sladek";
							break;
						default:
							deep_stops = "unknown";
							break;
					}

					snprintf(config_buf, sizeof(config_buf), "%s,%s,%s,safety stop required=%s,last_stop=%s,deco_algorithm=%s,stop_rounding=%u,deep_stops=%s", (ptr[4] & 0x80) ? "imperial" : "metric", (ptr[4] & 0x40) ? "sea" : "fresh", (ptr[4] & 0x30) ? "stops" : "ceiling", (ptr[4] & 0x10) ? "yes" : "no", (ptr[4] & 0x08) ? "6m" : "3m", (ptr[4] & 0x04) ? "VPM" : "Buhlmann+GF", (ptr[4] & 0x03) ? (ptr[4] & 0x03) * 30 : 1, deep_stops);
					add_extra_data(state.cur_dc, "CFG_CONFIG_DECO part 1", config_buf);
					snprintf(config_buf, sizeof(config_buf), "deep_stop_len=%u min,gas_switch_len=%u min,gf_low=%u,gf_high=%u,gf_low_bailout=%u,gf_high_bailout=%u,ppO2_low=%4.2f,ppO2_high=%4.2f", (ptr[5] & 0x38) >> 3, ptr[5] & 0x07, ptr[6], ptr[7], ptr[8], ptr[9], ptr[10] / 100.0f, ptr[11] / 100.0f);
					add_extra_data(state.cur_dc, "CFG_CONFIG_DECO part 2", config_buf);
					snprintf(config_buf, sizeof(config_buf), "alarm_global=%u,alarm_cns=%u,alarm_ppO2=%u,alarm_ceiling=%u,alarm_stop_miss=%u,alarm_decentrate=%u,alarm_ascentrate=%u", (ptr[12] & 0x80) >> 7, (ptr[12] & 0x40) >> 6, (ptr[12] & 0x20) >> 5, (ptr[12] & 0x10) >> 4, (ptr[12] & 0x08) >> 3, (ptr[12] & 0x04) >> 2, (ptr[12] & 0x02) >> 1);
					add_extra_data(state.cur_dc, "CFG_CONFIG_DECO part 3", config_buf);
					break;
				case 5: // CFG_VERSION
					switch (ptr[4]) {
						case 0:
							device = "FREEDOM";
							break;
						case 1:
							device = "LIBERTY_CU";
							break;
						case 2:
							device = "LIBERTY_HS";
							break;
						default:
							device = "UNKNOWN";
							break;
					}
					snprintf(config_buf, sizeof(config_buf), "DEVICE=%s,HW=%d.%d,FW=%d.%d.%d.%d,FLAGS=%04X", device, ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], (ptr[15] << 24) + (ptr[14] << 16) + (ptr[13] << 8) + (ptr[12]), (ptr[11] << 8) + ptr[10]);
					add_extra_data(state.cur_dc, "CFG_VERSION", config_buf);
					break;
			}
			break;
		case 7:
			/* measure record */
			switch (ptr[2] >> 5) {
			case 1:
				/* Record starting battery level */
				if (!battery_start.volt1 && !battery_start.volt2) {
					battery_start.volt1 = (ptr[5] << 8) + ptr[4];
					battery_start.percent1 = ptr[6];
					battery_start.volt2 = (ptr[9] << 8) + ptr[8];
					battery_start.percent2 = ptr[10];
				}

				/* Measure Battery, recording the last reading only */
				battery_end.volt1 = (ptr[5] << 8) + ptr[4];
				battery_end.percent1 = ptr[6];
				battery_end.volt2 = (ptr[9] << 8) + ptr[8];
				battery_end.percent2 = ptr[10];
				break;
			case 2:
				/* Measure He */
				//printf("%ds he2 cells(0.01 mV): %d %d\n", time, (ptr[5] << 8) + ptr[4], (ptr[9] << 8) + ptr[8]);
				break;
			case 3:
				/* Measure Oxygen */
				//printf("%d s: o2 cells(0.01 mV): %d %d %d %d\n", time, (ptr[5] << 8) + ptr[4], (ptr[7] << 8) + ptr[6], (ptr[9] << 8) + ptr[8], (ptr[11] << 8) + ptr[10]);
				// [Pa/mV] coeficient O2
				// 100 Pa == 1 mbar
				sample_start(&state);
				state.cur_sample->time.seconds = time;
				state.cur_sample->o2sensor[0].mbar = ( ((ptr[5] << 8) + ptr[4]) * o2_sensor_calibration_values[0]) / 10000;
				state.cur_sample->o2sensor[1].mbar = ( ((ptr[7] << 8) + ptr[6]) * o2_sensor_calibration_values[1]) / 10000;
				state.cur_sample->o2sensor[2].mbar = ( ((ptr[9] << 8) + ptr[8]) * o2_sensor_calibration_values[2]) / 10000;
				state.cur_sample->o2sensor[3].mbar = ( ((ptr[11] << 8) + ptr[10]) * o2_sensor_calibration_values[3]) / 10000;
				sample_end(&state);
				break;
			case 4:
				/* Measure GPS */
				state.cur_location.lat.udeg =  (int)((ptr[7]  << 24) + (ptr[6]  << 16) + (ptr[5] << 8) + (ptr[4] << 0));
				state.cur_location.lon.udeg = (int)((ptr[11] << 24) + (ptr[10] << 16) + (ptr[9] << 8) + (ptr[8] << 0));
				add_dive_to_dive_site(state.cur_dive, create_dive_site_with_gps("DLF imported", &state.cur_location, state.log->sites));
				break;
			default:
				break;
			}
			break;
		case 8:
			/* Deco event */
			break;
		default:
			/* Unknown... */
			break;
		}
	}

	/* Recording the starting battery status to extra data */
	if (battery_start.volt1) {
		size_t stringsize = snprintf(NULL, 0, "%dmV (%d%%)", battery_start.volt1, battery_start.percent1) + 1;
		char *ptr = malloc(size);

		if (ptr) {
			snprintf(ptr, stringsize, "%dmV (%d%%)", battery_start.volt1, battery_start.percent1);
			add_extra_data(state.cur_dc, "Battery 1 (start)", ptr);
			free(ptr);
		}

		stringsize = snprintf(NULL, 0, "%dmV (%d%%)", battery_start.volt2, battery_start.percent2) + 1;
		ptr = malloc(stringsize);
		if (ptr) {
			snprintf(ptr, stringsize, "%dmV (%d%%)", battery_start.volt2, battery_start.percent2);
			add_extra_data(state.cur_dc, "Battery 2 (start)", ptr);
			free(ptr);
		}
	}

	/* Recording the ending battery status to extra data */
	if (battery_end.volt1) {
		size_t stringsize = snprintf(NULL, 0, "%dmV (%d%%)", battery_end.volt1, battery_end.percent1) + 1;
		char *ptr = malloc(stringsize);

		if (ptr) {
			snprintf(ptr, stringsize, "%dmV (%d%%)", battery_end.volt1, battery_end.percent1);
			add_extra_data(state.cur_dc, "Battery 1 (end)", ptr);
			free(ptr);
		}

		stringsize = snprintf(NULL, 0, "%dmV (%d%%)", battery_end.volt2, battery_end.percent2) + 1;
		ptr = malloc(stringsize);
		if (ptr) {
			snprintf(ptr, stringsize, "%dmV (%d%%)", battery_end.volt2, battery_end.percent2);
			add_extra_data(state.cur_dc, "Battery 2 (end)", ptr);
			free(ptr);
		}
	}

	divecomputer_end(&state);
	dive_end(&state);
	free_parser_state(&state);
	return 0;
}


void parse_xml_init(void)
{
	LIBXML_TEST_VERSION
}

void parse_xml_exit(void)
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
		xmlSubstituteEntitiesDefault(1);
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
