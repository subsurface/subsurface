#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#define __USE_XOPEN
#include <time.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libdivecomputer/parser.h>

#include "gettext.h"

#include "dive.h"
#include "device.h"
#include "membuffer.h"

int verbose, quit;
int metric = 1;

static xmlDoc *test_xslt_transforms(xmlDoc *doc, const char **params);

/* the dive table holds the overall dive list; target table points at
 * the table we are currently filling */
struct dive_table dive_table;
struct dive_table *target_table = NULL;

/* Trim a character string by removing leading and trailing white space characters.
 * Parameter: a pointer to a null-terminated character string (buffer);
 * Return value: length of the trimmed string, excluding the terminal 0x0 byte
 * The original pointer (buffer) remains valid after this function has been called
 * and points to the trimmed string */
int trimspace(char *buffer) {
	int i, size, start, end;
	size = strlen(buffer);
	for(start = 0; isspace(buffer[start]); start++)
		if (start >= size) return 0;	// Find 1st character following leading whitespace
	for(end = size - 1; isspace(buffer[end]); end--) // Find last character before trailing whitespace
		if (end <= 0) return 0;
	for(i = start; i <= end; i++)		// Move the nonspace characters to the start of the string
		buffer[i-start] = buffer[i];
	size = end - start + 1;
	buffer[size] = 0x0;			// then terminate the string
	return size;				// return string length
}

/*
 * Add a dive into the dive_table array
 */
static void record_dive_to_table(struct dive *dive, struct dive_table *table)
{
	assert(table != NULL);
	int nr = table->nr, allocated = table->allocated;
	struct dive **dives = table->dives;

	if (nr >= allocated) {
		allocated = (nr + 32) * 3 / 2;
		dives = realloc(dives, allocated * sizeof(struct dive *));
		if (!dives)
			exit(1);
		table->dives = dives;
		table->allocated = allocated;
	}
	dives[nr] = fixup_dive(dive);
	table->nr = nr + 1;
}

void record_dive(struct dive *dive)
{
	record_dive_to_table(dive, &dive_table);
}

static void start_match(const char *type, const char *name, char *buffer)
{
	if (verbose > 2)
		printf("Matching %s '%s' (%s)\n",
		       type, name, buffer);
}

static void nonmatch(const char *type, const char *name, char *buffer)
{
	if (verbose > 1)
		printf("Unable to match %s '%s' (%s)\n",
		       type, name, buffer);
}

typedef void (*matchfn_t)(char *buffer, void *);

static int match(const char *pattern, int plen,
		 const char *name,
		 matchfn_t fn, char *buf, void *data)
{
	switch (name[plen]) {
	case '\0':
	case '.':
		break;
	default:
		return 0;
	}
	if (memcmp(pattern, name, plen))
		return 0;
	fn(buf, data);
	return 1;
}


struct units xml_parsing_units;
const struct units SI_units = SI_UNITS;
const struct units IMPERIAL_units = IMPERIAL_UNITS;

/*
 * Dive info as it is being built up..
 */
#define MAX_EVENT_NAME 128
static struct divecomputer *cur_dc;
static struct dive *cur_dive;
static dive_trip_t *cur_trip = NULL;
static struct sample *cur_sample;
static struct picture *cur_picture;
static union {
	struct event event;
	char allocation[sizeof(struct event)+MAX_EVENT_NAME];
} event_allocation = { .event.deleted = 1 };
#define cur_event event_allocation.event
static struct {
	struct {
		const char *model;
		uint32_t deviceid;
		const char *nickname, *serial_nr, *firmware;
	} dc;
} cur_settings;
static bool in_settings = false;
static bool in_userid = false;
static struct tm cur_tm;
static int cur_cylinder_index, cur_ws_index;
static int lastndl, laststoptime, laststopdepth, lastcns, lastpo2, lastindeco;
static int lastcylinderindex, lastsensor;
static struct extra_data cur_extra_data;

/*
 * If we don't have an explicit dive computer,
 * we use the implicit one that every dive has..
 */
static struct divecomputer *get_dc(void)
{
	return cur_dc ?: &cur_dive->dc;
}

static enum import_source {
	UNKNOWN,
	LIBDIVECOMPUTER,
	DIVINGLOG,
	UDDF,
} import_source;

static void divedate(const char *buffer, timestamp_t *when)
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
	cur_tm.tm_year = y;
	cur_tm.tm_mon = m - 1;
	cur_tm.tm_mday = d;
	cur_tm.tm_hour = hh;
	cur_tm.tm_min = mm;
	cur_tm.tm_sec = ss;

	*when = utc_mktime(&cur_tm);
}

static void divetime(const char *buffer, timestamp_t *when)
{
	int h, m, s = 0;

	if (sscanf(buffer, "%d:%d:%d", &h, &m, &s) >= 2) {
		cur_tm.tm_hour = h;
		cur_tm.tm_min = m;
		cur_tm.tm_sec = s;
		*when = utc_mktime(&cur_tm);
	}
}

/* Libdivecomputer: "2011-03-20 10:22:38" */
static void divedatetime(char *buffer, timestamp_t *when)
{
	int y, m, d;
	int hr, min, sec;

	if (sscanf(buffer, "%d-%d-%d %d:%d:%d",
		   &y, &m, &d, &hr, &min, &sec) == 6) {
		cur_tm.tm_year = y;
		cur_tm.tm_mon = m - 1;
		cur_tm.tm_mday = d;
		cur_tm.tm_hour = hr;
		cur_tm.tm_min = min;
		cur_tm.tm_sec = sec;
		*when = utc_mktime(&cur_tm);
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
	FLOAT
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
		if (IS_FP_SAME(val, rint(val))) {
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
	return FLOAT;
}

union int_or_float {
	double fp;
};

static enum number_type integer_or_float(char *buffer, union int_or_float *res)
{
	const char *end;
	return parse_float(buffer, &res->fp, &end);
}

static void pressure(char *buffer, pressure_t *pressure)
{
	double mbar = 0.0;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		/* Just ignore zero values */
		if (!val.fp)
			break;
		switch (xml_parsing_units.pressure) {
		case PASCAL:
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
			pressure->mbar = rint(mbar);
			break;
		}
	/* fallthrough */
	default:
		printf("Strange pressure reading %s\n", buffer);
	}
}

static void cylinder_use(char *buffer, enum cylinderuse *cyl_use)
{
	if (trimspace(buffer))
		*cyl_use = cylinderuse_from_text(buffer);
}

static void salinity(char *buffer, int *salinity)
{
	union int_or_float val;
	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		*salinity = rint(val.fp * 10.0);
		break;
	default:
		printf("Strange salinity reading %s\n", buffer);
	}
}

static void depth(char *buffer, depth_t *depth)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		switch (xml_parsing_units.length) {
		case METERS:
			depth->mm = rint(val.fp * 1000);
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

static void extra_data_start(void)
{
	memset(&cur_extra_data, 0, sizeof(struct extra_data));
}

static void extra_data_end(void)
{
	// don't save partial structures - we must have both key and value
	if (cur_extra_data.key && cur_extra_data.value)
		add_extra_data(cur_dc, cur_extra_data.key, cur_extra_data.value);
}

static void weight(char *buffer, weight_t *weight)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		switch (xml_parsing_units.weight) {
		case KG:
			weight->grams = rint(val.fp * 1000);
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

static void temperature(char *buffer, temperature_t *temperature)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		switch (xml_parsing_units.temperature) {
		case KELVIN:
			temperature->mkelvin = val.fp * 1000;
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
	int min, sec;

	i = sscanf(buffer, "%d:%d", &min, &sec);
	switch (i) {
	case 1:
		sec = min;
		min = 0;
	/* fallthrough */
	case 2:
		time->seconds = sec + min * 60;
		break;
	default:
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
	case FLOAT:
		/* Turn fractions into percent unless explicit.. */
		if (val <= 1.0) {
			while (isspace(*end))
				end++;
			if (*end != '%')
				val *= 100;
		}

		/* Then turn percent into our integer permille format */
		if (val >= 0 && val <= 100.0) {
			fraction->permille = rint(val * 10);
			break;
		}
	default:
		printf(translate("gettextFromC", "Strange percentage reading %s\n"), buffer);
		break;
	}
}

static void gasmix(char *buffer, fraction_t *fraction)
{
	/* libdivecomputer does negative percentages. */
	if (*buffer == '-')
		return;
	if (cur_cylinder_index < MAX_CYLINDERS)
		percent(buffer, fraction);
}

static void gasmix_nitrogen(char *buffer, struct gasmix *gasmix)
{
	/* Ignore n2 percentages. There's no value in them. */
}

static void cylindersize(char *buffer, volume_t *volume)
{
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		volume->mliter = rint(val.fp * 1000);
		break;

	default:
		printf("Strange volume reading %s\n", buffer);
		break;
	}
}

static void utf8_string(char *buffer, void *_res)
{
	char **res = _res;
	int size;
	size = trimspace(buffer);
	if(size)
		*res = strdup(buffer);
}

static void event_name(char *buffer, char *name)
{
	int size = trimspace(buffer);
	if (size >= MAX_EVENT_NAME)
		size = MAX_EVENT_NAME-1;
	memcpy(name, buffer, size);
	name[size] = 0;
}

/* Extract the dive computer type from the xml text buffer */
static void get_dc_type(char *buffer, enum dive_comp_type *dct)
{
	if (trimspace(buffer)) {
		for (enum dive_comp_type i = 0; i < NUM_DC_TYPE; i++) {
			if (strcmp(buffer, dctype_text[i]) == 0)
				*dct = i;
		}
	}
}

#define MATCH(pattern, fn, dest) ({ 			\
	/* Silly type compatibility test */ 		\
	if (0) (fn)("test", dest);			\
	match(pattern, strlen(pattern), name, (matchfn_t) (fn), buf, dest); })

static void get_index(char *buffer, int *i)
{
	*i = atoi(buffer);
}

static void get_uint8(char *buffer, uint8_t *i)
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
	i->mbar = rint(ascii_strtod(buffer, NULL) * 1000.0);
}

static void hex_value(char *buffer, uint32_t *i)
{
	*i = strtoul(buffer, NULL, 16);
}

static void get_tripflag(char *buffer, tripflag_t *tf)
{
	*tf = strcmp(buffer, "NOTRIP") ? TF_NONE : NO_TRIP;
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
	case FLOAT:
		if (IS_FP_SAME(val.fp, 32.0))
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
	case FLOAT:
		if (val.fp > 400)
			pressure->mbar = psi_to_mbar(val.fp);
		else
			pressure->mbar = rint(val.fp * 1000);
		break;
	default:
		fprintf(stderr, "Crazy Diving Log PSI reading %s\n", buffer);
	}
}

static int divinglog_fill_sample(struct sample *sample, const char *name, char *buf)
{
	return MATCH("time.p", sampletime, &sample->time) ||
	       MATCH("depth.p", depth, &sample->depth) ||
	       MATCH("temp.p", fahrenheit, &sample->temperature) ||
	       MATCH("press1.p", psi_or_bar, &sample->cylinderpressure) ||
	       0;
}

static void uddf_gasswitch(char *buffer, struct sample *sample)
{
	int idx = atoi(buffer);
	int seconds = sample->time.seconds;
	struct dive *dive = cur_dive;
	struct divecomputer *dc = get_dc();

	add_gas_switch_event(dive, dc, seconds, idx);
}

static int uddf_fill_sample(struct sample *sample, const char *name, char *buf)
{
	return MATCH("divetime", sampletime, &sample->time) ||
	       MATCH("depth", depth, &sample->depth) ||
	       MATCH("temperature", temperature, &sample->temperature) ||
	       MATCH("tankpressure", pressure, &sample->cylinderpressure) ||
	       MATCH("ref.switchmix", uddf_gasswitch, sample) ||
	       0;
}

static void eventtime(char *buffer, duration_t *duration)
{
	sampletime(buffer, duration);
	if (cur_sample)
		duration->seconds += cur_sample->time.seconds;
}

static void try_to_match_autogroup(const char *name, char *buf)
{
	int autogroupvalue;

	start_match("autogroup", name, buf);
	if (MATCH("state.autogroup", get_index, &autogroupvalue)) {
		set_autogroup(autogroupvalue);
		return;
	}
	nonmatch("autogroup", name, buf);
}

void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int seconds, int idx)
{
	/* The gas switch event format is insane for historical reasons */
	struct gasmix *mix = &dive->cylinder[idx].gasmix;
	int o2 = get_o2(mix);
	int he = get_he(mix);
	struct event *ev;
	int value;

	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;
	value = o2 + (he << 16);

	ev = add_event(dc, seconds, he ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE, 0, value, "gaschange");
	if (ev) {
		ev->gas.index = idx;
		ev->gas.mix = *mix;
	}
}

static void get_cylinderindex(char *buffer, uint8_t *i)
{
	*i = atoi(buffer);
	if (lastcylinderindex != *i) {
		add_gas_switch_event(cur_dive, get_dc(), cur_sample->time.seconds, *i);
		lastcylinderindex = *i;
	}
}

static void get_sensor(char *buffer, uint8_t *i)
{
	*i = atoi(buffer);
	lastsensor = *i;
}

static void try_to_fill_dc_settings(const char *name, char *buf)
{
	start_match("divecomputerid", name, buf);
	if (MATCH("model.divecomputerid", utf8_string, &cur_settings.dc.model))
		return;
	if (MATCH("deviceid.divecomputerid", hex_value, &cur_settings.dc.deviceid))
		return;
	if (MATCH("nickname.divecomputerid", utf8_string, &cur_settings.dc.nickname))
		return;
	if (MATCH("serial.divecomputerid", utf8_string, &cur_settings.dc.serial_nr))
		return;
	if (MATCH("firmware.divecomputerid", utf8_string, &cur_settings.dc.firmware))
		return;

	nonmatch("divecomputerid", name, buf);
}

static void try_to_fill_event(const char *name, char *buf)
{
	start_match("event", name, buf);
	if (MATCH("event", event_name, cur_event.name))
		return;
	if (MATCH("name", event_name, cur_event.name))
		return;
	if (MATCH("time", eventtime, &cur_event.time))
		return;
	if (MATCH("type", get_index, &cur_event.type))
		return;
	if (MATCH("flags", get_index, &cur_event.flags))
		return;
	if (MATCH("value", get_index, &cur_event.value))
		return;
	if (MATCH("cylinder", get_index, &cur_event.gas.index)) {
		/* We add one to indicate that we got an actual cylinder index value */
		cur_event.gas.index++;
		return;
	}
	if (MATCH("o2", percent, &cur_event.gas.mix.o2))
		return;
	if (MATCH("he", percent, &cur_event.gas.mix.he))
		return;
	nonmatch("event", name, buf);
}

static int match_dc_data_fields(struct divecomputer *dc, const char *name, char *buf)
{
	if (MATCH("maxdepth", depth, &dc->maxdepth))
		return 1;
	if (MATCH("meandepth", depth, &dc->meandepth))
		return 1;
	if (MATCH("max.depth", depth, &dc->maxdepth))
		return 1;
	if (MATCH("mean.depth", depth, &dc->meandepth))
		return 1;
	if (MATCH("duration", duration, &dc->duration))
		return 1;
	if (MATCH("divetime", duration, &dc->duration))
		return 1;
	if (MATCH("divetimesec", duration, &dc->duration))
		return 1;
	if (MATCH("surfacetime", duration, &dc->surfacetime))
		return 1;
	if (MATCH("airtemp", temperature, &dc->airtemp))
		return 1;
	if (MATCH("watertemp", temperature, &dc->watertemp))
		return 1;
	if (MATCH("air.temperature", temperature, &dc->airtemp))
		return 1;
	if (MATCH("water.temperature", temperature, &dc->watertemp))
		return 1;
	if (MATCH("pressure.surface", pressure, &dc->surface_pressure))
		return 1;
	if (MATCH("salinity.water", salinity, &dc->salinity))
		return 1;
	if (MATCH("key.extradata", utf8_string, &cur_extra_data.key))
		return 1;
	if (MATCH("value.extradata", utf8_string, &cur_extra_data.value))
		return 1;
	return 0;
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dc(struct divecomputer *dc, const char *name, char *buf)
{
	start_match("divecomputer", name, buf);

	if (MATCH("date", divedate, &dc->when))
		return;
	if (MATCH("time", divetime, &dc->when))
		return;
	if (MATCH("model", utf8_string, &dc->model))
		return;
	if (MATCH("deviceid", hex_value, &dc->deviceid))
		return;
	if (MATCH("diveid", hex_value, &dc->diveid))
		return;
	if (MATCH("dctype", get_dc_type, &dc->dctype))
		return;
	if (MATCH("no_o2sensors", get_sensor, &dc->no_o2sensors))
		return;
	if (match_dc_data_fields(dc, name, buf))
		return;

	nonmatch("divecomputer", name, buf);
}

/* We're in samples - try to convert the random xml value to something useful */
static void try_to_fill_sample(struct sample *sample, const char *name, char *buf)
{
	int in_deco;

	start_match("sample", name, buf);
	if (MATCH("pressure.sample", pressure, &sample->cylinderpressure))
		return;
	if (MATCH("cylpress.sample", pressure, &sample->cylinderpressure))
		return;
	if (MATCH("pdiluent.sample", pressure, &sample->cylinderpressure))
		return;
	if (MATCH("o2pressure.sample", pressure, &sample->o2cylinderpressure))
		return;
	if (MATCH("cylinderindex.sample", get_cylinderindex, &sample->sensor))
		return;
	if (MATCH("sensor.sample", get_sensor, &sample->sensor))
		return;
	if (MATCH("depth.sample", depth, &sample->depth))
		return;
	if (MATCH("temp.sample", temperature, &sample->temperature))
		return;
	if (MATCH("temperature.sample", temperature, &sample->temperature))
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
	if (MATCH("stopdepth.sample", depth, &sample->stopdepth))
		return;
	if (MATCH("cns.sample", get_uint8, &sample->cns))
		return;
	if (MATCH("sensor1.sample", double_to_o2pressure, &sample->o2sensor[0])) // CCR O2 sensor data
		return;
	if (MATCH("sensor2.sample", double_to_o2pressure, &sample->o2sensor[1]))
		return;
	if (MATCH("sensor3.sample", double_to_o2pressure, &sample->o2sensor[2])) // up to 3 CCR sensors
		return;
	if (MATCH("po2.sample", double_to_o2pressure, &sample->setpoint)) {
		cur_dive->dc.dctype = CCR;
		return;
	}
	if (MATCH("heartbeat", get_uint8, &sample->heartbeat))
		return;
	if (MATCH("bearing", get_bearing, &sample->bearing))
		return;

	switch (import_source) {
	case DIVINGLOG:
		if (divinglog_fill_sample(sample, name, buf))
			return;
		break;

	case UDDF:
		if (uddf_fill_sample(sample, name, buf))
			return;
		break;

	default:
		break;
	}

	nonmatch("sample", name, buf);
}

void try_to_fill_userid(const char *name, char *buf)
{
	if (prefs.save_userid_local)
		set_userid(buf);
}

static const char *country, *city;

static void divinglog_place(char *place, char **location)
{
	char buffer[1024], *p;
	int len;

	len = snprintf(buffer, sizeof(buffer),
		       "%s%s%s%s%s",
		       place,
		       city ? ", " : "",
		       city ? city : "",
		       country ? ", " : "",
		       country ? country : "");

	p = malloc(len + 1);
	memcpy(p, buffer, len + 1);
	*location = p;

	city = NULL;
	country = NULL;
}

static int divinglog_dive_match(struct dive *dive, const char *name, char *buf)
{
	return MATCH("divedate", divedate, &dive->when) ||
	       MATCH("entrytime", divetime, &dive->when) ||
	       MATCH("divetime", duration, &dive->dc.duration) ||
	       MATCH("depth", depth, &dive->dc.maxdepth) ||
	       MATCH("depthavg", depth, &dive->dc.meandepth) ||
	       MATCH("tanktype", utf8_string, &dive->cylinder[0].type.description) ||
	       MATCH("tanksize", cylindersize, &dive->cylinder[0].type.size) ||
	       MATCH("presw", pressure, &dive->cylinder[0].type.workingpressure) ||
	       MATCH("press", pressure, &dive->cylinder[0].start) ||
	       MATCH("prese", pressure, &dive->cylinder[0].end) ||
	       MATCH("comments", utf8_string, &dive->notes) ||
	       MATCH("names.buddy", utf8_string, &dive->buddy) ||
	       MATCH("name.country", utf8_string, &country) ||
	       MATCH("name.city", utf8_string, &city) ||
	       MATCH("name.place", divinglog_place, &dive->location) ||
	       0;
}

/*
 * Uddf specifies ISO 8601 time format.
 *
 * There are many variations on that. This handles the useful cases.
 */
static void uddf_datetime(char *buffer, timestamp_t *when)
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

#define uddf_datedata(name, offset)                              \
	static void uddf_##name(char *buffer, timestamp_t *when) \
	{                                                        \
		cur_tm.tm_##name = atoi(buffer) + offset;        \
		*when = utc_mktime(&cur_tm);                     \
	}

uddf_datedata(year, 0)
uddf_datedata(mon, -1)
uddf_datedata(mday, 0)
uddf_datedata(hour, 0)
uddf_datedata(min, 0)

static int uddf_dive_match(struct dive *dive, const char *name, char *buf)
{
	return MATCH("datetime", uddf_datetime, &dive->when) ||
	       MATCH("diveduration", duration, &dive->dc.duration) ||
	       MATCH("greatestdepth", depth, &dive->dc.maxdepth) ||
	       MATCH("year.date", uddf_year, &dive->when) ||
	       MATCH("month.date", uddf_mon, &dive->when) ||
	       MATCH("day.date", uddf_mday, &dive->when) ||
	       MATCH("hour.time", uddf_hour, &dive->when) ||
	       MATCH("minute.time", uddf_min, &dive->when) ||
	       0;
}

/*
 * This parses "floating point" into micro-degrees.
 * We don't do exponentials etc, if somebody does
 * gps locations in that format, they are insane.
 */
degrees_t parse_degrees(char *buf, char **end)
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

static void gps_lat(char *buffer, struct dive *dive)
{
	char *end;

	dive->latitude = parse_degrees(buffer, &end);
}

static void gps_long(char *buffer, struct dive *dive)
{
	char *end;

	dive->longitude = parse_degrees(buffer, &end);
}

static void gps_location(char *buffer, struct dive *dive)
{
	char *end;

	dive->latitude = parse_degrees(buffer, &end);
	dive->longitude = parse_degrees(end, &end);
}

static void gps_picture_location(char *buffer, struct picture *pic)
{
	char *end;

	pic->latitude = parse_degrees(buffer, &end);
	pic->longitude = parse_degrees(end, &end);
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dive(struct dive *dive, const char *name, char *buf)
{
	start_match("dive", name, buf);

	switch (import_source) {
	case DIVINGLOG:
		if (divinglog_dive_match(dive, name, buf))
			return;
		break;

	case UDDF:
		if (uddf_dive_match(dive, name, buf))
			return;
		break;

	default:
		break;
	}

	if (MATCH("number", get_index, &dive->number))
		return;
	if (MATCH("tags", divetags, &dive->tag_list))
		return;
	if (MATCH("tripflag", get_tripflag, &dive->tripflag))
		return;
	if (MATCH("date", divedate, &dive->when))
		return;
	if (MATCH("time", divetime, &dive->when))
		return;
	if (MATCH("datetime", divedatetime, &dive->when))
		return;
	/*
	 * Legacy format note: per-dive depths and duration get saved
	 * in the first dive computer entry
	 */
	if (match_dc_data_fields(&dive->dc, name, buf))
		return;

	if (MATCH("filename.picture", utf8_string, &cur_picture->filename))
		return;
	if (MATCH("offset.picture", offsettime, &cur_picture->offset))
		return;
	if (MATCH("gps.picture", gps_picture_location, cur_picture))
		return;
	if (MATCH("cylinderstartpressure", pressure, &dive->cylinder[0].start))
		return;
	if (MATCH("cylinderendpressure", pressure, &dive->cylinder[0].end))
		return;
	if (MATCH("gps", gps_location, dive))
		return;
	if (MATCH("Place", gps_location, dive))
		return;
	if (MATCH("latitude", gps_lat, dive))
		return;
	if (MATCH("sitelat", gps_lat, dive))
		return;
	if (MATCH("lat", gps_lat, dive))
		return;
	if (MATCH("longitude", gps_long, dive))
		return;
	if (MATCH("sitelon", gps_long, dive))
		return;
	if (MATCH("lon", gps_long, dive))
		return;
	if (MATCH("location", utf8_string, &dive->location))
		return;
	if (MATCH("name.dive", utf8_string, &dive->location))
		return;
	if (MATCH("suit", utf8_string, &dive->suit))
		return;
	if (MATCH("divesuit", utf8_string, &dive->suit))
		return;
	if (MATCH("notes", utf8_string, &dive->notes))
		return;
	if (MATCH("divemaster", utf8_string, &dive->divemaster))
		return;
	if (MATCH("buddy", utf8_string, &dive->buddy))
		return;
	if (MATCH("rating.dive", get_rating, &dive->rating))
		return;
	if (MATCH("visibility.dive", get_rating, &dive->visibility))
		return;
	if (MATCH("size.cylinder", cylindersize, &dive->cylinder[cur_cylinder_index].type.size))
		return;
	if (MATCH("workpressure.cylinder", pressure, &dive->cylinder[cur_cylinder_index].type.workingpressure))
		return;
	if (MATCH("description.cylinder", utf8_string, &dive->cylinder[cur_cylinder_index].type.description))
		return;
	if (MATCH("start.cylinder", pressure, &dive->cylinder[cur_cylinder_index].start))
		return;
	if (MATCH("end.cylinder", pressure, &dive->cylinder[cur_cylinder_index].end))
		return;
	if (MATCH("use.cylinder", cylinder_use, &dive->cylinder[cur_cylinder_index].cylinder_use))
		return;
	if (MATCH("description.weightsystem", utf8_string, &dive->weightsystem[cur_ws_index].description))
		return;
	if (MATCH("weight.weightsystem", weight, &dive->weightsystem[cur_ws_index].weight))
		return;
	if (MATCH("weight", weight, &dive->weightsystem[cur_ws_index].weight))
		return;
	if (MATCH("o2", gasmix, &dive->cylinder[cur_cylinder_index].gasmix.o2))
		return;
	if (MATCH("o2percent", gasmix, &dive->cylinder[cur_cylinder_index].gasmix.o2))
		return;
	if (MATCH("n2", gasmix_nitrogen, &dive->cylinder[cur_cylinder_index].gasmix))
		return;
	if (MATCH("he", gasmix, &dive->cylinder[cur_cylinder_index].gasmix.he))
		return;
	if (MATCH("air.divetemperature", temperature, &dive->airtemp))
		return;
	if (MATCH("water.divetemperature", temperature, &dive->watertemp))
		return;

	nonmatch("dive", name, buf);
}

/* We're in the top-level trip xml. Try to convert whatever value to a trip value */
static void try_to_fill_trip(dive_trip_t **dive_trip_p, const char *name, char *buf)
{
	start_match("trip", name, buf);

	dive_trip_t *dive_trip = *dive_trip_p;

	if (MATCH("date", divedate, &dive_trip->when))
		return;
	if (MATCH("time", divetime, &dive_trip->when))
		return;
	if (MATCH("location", utf8_string, &dive_trip->location))
		return;
	if (MATCH("notes", utf8_string, &dive_trip->notes))
		return;

	nonmatch("trip", name, buf);
}

/*
 * While in some formats file boundaries are dive boundaries, in many
 * others (as for example in our native format) there are
 * multiple dives per file, so there can be other events too that
 * trigger a "new dive" marker and you may get some nesting due
 * to that. Just ignore nesting levels.
 * On the flipside it is possible that we start an XML file that ends
 * up having no dives in it at all - don't create a bogus empty dive
 * for those. It's not entirely clear what is the minimum set of data
 * to make a dive valid, but if it has no location, no date and no
 * samples I'm pretty sure it's useless.
 */
static bool is_dive(void)
{
	return (cur_dive &&
		(cur_dive->location || cur_dive->when || cur_dive->dc.samples));
}

static void reset_dc_info(struct divecomputer *dc)
{
	lastcns = lastpo2 = lastndl = laststoptime = laststopdepth = lastindeco = 0;
	lastsensor = lastcylinderindex = 0;
}

static void reset_dc_settings(void)
{
	free((void *)cur_settings.dc.model);
	free((void *)cur_settings.dc.nickname);
	free((void *)cur_settings.dc.serial_nr);
	free((void *)cur_settings.dc.firmware);
	cur_settings.dc.model = NULL;
	cur_settings.dc.nickname = NULL;
	cur_settings.dc.serial_nr = NULL;
	cur_settings.dc.firmware = NULL;
	cur_settings.dc.deviceid = 0;
}

static void settings_start(void)
{
	in_settings = true;
}

static void settings_end(void)
{
	in_settings = false;
}

static void dc_settings_start(void)
{
	reset_dc_settings();
}

static void dc_settings_end(void)
{
	create_device_node(cur_settings.dc.model, cur_settings.dc.deviceid, cur_settings.dc.serial_nr,
			   cur_settings.dc.firmware, cur_settings.dc.nickname);
	reset_dc_settings();
}

static void dive_start(void)
{
	if (cur_dive)
		return;
	cur_dive = alloc_dive();
	reset_dc_info(&cur_dive->dc);
	memset(&cur_tm, 0, sizeof(cur_tm));
	if (cur_trip) {
		add_dive_to_trip(cur_dive, cur_trip);
		cur_dive->tripflag = IN_TRIP;
	}
}

static void dive_end(void)
{
	if (!cur_dive)
		return;
	if (!is_dive())
		free(cur_dive);
	else
		record_dive_to_table(cur_dive, target_table);
	cur_dive = NULL;
	cur_dc = NULL;
	cur_cylinder_index = 0;
	cur_ws_index = 0;
}

static void trip_start(void)
{
	if (cur_trip)
		return;
	dive_end();
	cur_trip = calloc(1, sizeof(dive_trip_t));
	memset(&cur_tm, 0, sizeof(cur_tm));
}

static void trip_end(void)
{
	if (!cur_trip)
		return;
	insert_trip(&cur_trip);
	cur_trip = NULL;
}

static void event_start(void)
{
	memset(&cur_event, 0, sizeof(cur_event));
	cur_event.deleted = 0;	/* Active */
}

static void event_end(void)
{
	struct divecomputer *dc = get_dc();
	if (strcmp(cur_event.name, "surface") != 0) {			/* 123 is a magic event that we used for a while to encode images in dives */
		if (cur_event.type == 123) {
			struct picture *pic = alloc_picture();
			pic->filename = strdup(cur_event.name);
			/* theoretically this could fail - but we didn't support multi year offsets */
			pic->offset.seconds = cur_event.time.seconds;
			dive_add_picture(cur_dive, pic);
		} else {
			struct event *ev;
			/* At some point gas change events did not have any type. Thus we need to add
			 * one on import, if we encounter the type one missing.
			 */
			if (cur_event.type == 0 && strcmp(cur_event.name, "gaschange") == 0)
				cur_event.type = cur_event.value >> 16 > 0 ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE;
			ev = add_event(dc, cur_event.time.seconds,
				       cur_event.type, cur_event.flags,
				       cur_event.value, cur_event.name);
			if (ev && event_is_gaschange(ev)) {
				/* See try_to_fill_event() on why the filled-in index is one too big */
				ev->gas.index = cur_event.gas.index-1;
				if (cur_event.gas.mix.o2.permille || cur_event.gas.mix.he.permille)
					ev->gas.mix = cur_event.gas.mix;
			}
		}
	}
	cur_event.deleted = 1;	/* No longer active */
}

static void picture_start(void)
{
	cur_picture = alloc_picture();
}

static void picture_end(void)
{
	dive_add_picture(cur_dive, cur_picture);
	cur_picture = NULL;
}

static void cylinder_start(void)
{
}

static void cylinder_end(void)
{
	cur_cylinder_index++;
}

static void ws_start(void)
{
}

static void ws_end(void)
{
	cur_ws_index++;
}

static void sample_start(void)
{
	cur_sample = prepare_sample(get_dc());
	cur_sample->ndl.seconds = lastndl;
	cur_sample->in_deco = lastindeco;
	cur_sample->stoptime.seconds = laststoptime;
	cur_sample->stopdepth.mm = laststopdepth;
	cur_sample->cns = lastcns;
	cur_sample->setpoint.mbar = lastpo2;
	cur_sample->sensor = lastsensor;
}

static void sample_end(void)
{
	if (!cur_dive)
		return;

	finish_sample(get_dc());
	lastndl = cur_sample->ndl.seconds;
	lastindeco = cur_sample->in_deco;
	laststoptime = cur_sample->stoptime.seconds;
	laststopdepth = cur_sample->stopdepth.mm;
	lastcns = cur_sample->cns;
	lastpo2 = cur_sample->setpoint.mbar;
	cur_sample = NULL;
}

static void divecomputer_start(void)
{
	struct divecomputer *dc;

	/* Start from the previous dive computer */
	dc = &cur_dive->dc;
	while (dc->next)
		dc = dc->next;

	/* Did we already fill that in? */
	if (dc->samples || dc->model || dc->when) {
		struct divecomputer *newdc = calloc(1, sizeof(*newdc));
		if (newdc) {
			dc->next = newdc;
			dc = newdc;
		}
	}

	/* .. this is the one we'll use */
	cur_dc = dc;
	reset_dc_info(dc);
}

static void divecomputer_end(void)
{
	if (!cur_dc->when)
		cur_dc->when = cur_dive->when;
	cur_dc = NULL;
}

static void userid_start(void)
{
	in_userid = true;
	set_save_userid_local(true); //if the xml contains userid, keep saving it.
}

static void userid_stop(void)
{
	in_userid = false;
}

static void entry(const char *name, char *buf)
{
	if (in_userid) {
		try_to_fill_userid(name, buf);
		return;
	}
	if (in_settings) {
		try_to_fill_dc_settings(name, buf);
		try_to_match_autogroup(name, buf);
		return;
	}
	if (!cur_event.deleted) {
		try_to_fill_event(name, buf);
		return;
	}
	if (cur_sample) {
		try_to_fill_sample(cur_sample, name, buf);
		return;
	}
	if (cur_dc) {
		try_to_fill_dc(cur_dc, name, buf);
		return;
	}
	if (cur_dive) {
		try_to_fill_dive(cur_dive, name, buf);
		return;
	}
	if (cur_trip) {
		try_to_fill_trip(&cur_trip, name, buf);
		return;
	}
}

static const char *nodename(xmlNode *node, char *buf, int len)
{
	int levels = 2;
	char *p = buf;

	if (node->type != XML_CDATA_SECTION_NODE && (!node || !node->name)) {
		return "root";
	}

	if (node->type == XML_CDATA_SECTION_NODE || (node->parent && !strcmp(node->name, "text")))
		node = node->parent;

	/* Make sure it's always NUL-terminated */
	p[--len] = 0;

	for (;;) {
		const char *name = node->name;
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

static void visit_one_node(xmlNode *node)
{
	char *content;
	static char buffer[MAXNAME];
	const char *name;

	content = node->content;
	if (!content || xmlIsBlankNode(node))
		return;

	name = nodename(node, buffer, sizeof(buffer));

	entry(name, content);
}

static void traverse(xmlNode *root);

static void traverse_properties(xmlNode *node)
{
	xmlAttr *p;

	for (p = node->properties; p; p = p->next)
		traverse(p->children);
}

static void visit(xmlNode *n)
{
	visit_one_node(n);
	traverse_properties(n);
	traverse(n->children);
}

static void DivingLog_importer(void)
{
	import_source = DIVINGLOG;

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
	xml_parsing_units = SI_units;
}

static void uddf_importer(void)
{
	import_source = UDDF;
	xml_parsing_units = SI_units;
	xml_parsing_units.pressure = PASCAL;
	xml_parsing_units.temperature = KELVIN;
}

/*
 * I'm sure this could be done as some fancy DTD rules.
 * It's just not worth the headache.
 */
static struct nesting {
	const char *name;
	void (*start)(void), (*end)(void);
} nesting[] = {
	  { "divecomputerid", dc_settings_start, dc_settings_end },
	  { "settings", settings_start, settings_end },
	  { "dive", dive_start, dive_end },
	  { "Dive", dive_start, dive_end },
	  { "trip", trip_start, trip_end },
	  { "sample", sample_start, sample_end },
	  { "waypoint", sample_start, sample_end },
	  { "SAMPLE", sample_start, sample_end },
	  { "reading", sample_start, sample_end },
	  { "event", event_start, event_end },
	  { "mix", cylinder_start, cylinder_end },
	  { "gasmix", cylinder_start, cylinder_end },
	  { "cylinder", cylinder_start, cylinder_end },
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

static void traverse(xmlNode *root)
{
	xmlNode *n;

	for (n = root; n; n = n->next) {
		struct nesting *rule = nesting;

		if (!n->name) {
			visit(n);
			continue;
		}

		do {
			if (!strcmp(rule->name, n->name))
				break;
			rule++;
		} while (rule->name);

		if (rule->start)
			rule->start();
		visit(n);
		if (rule->end)
			rule->end();
	}
}

/* Per-file reset */
static void reset_all(void)
{
	/*
	 * We reset the units for each file. You'd think it was
	 * a per-dive property, but I'm not going to trust people
	 * to do per-dive setup. If the xml does have per-dive
	 * data within one file, we might have to reset it per
	 * dive for that format.
	 */
	xml_parsing_units = SI_units;
	import_source = UNKNOWN;
}

/* divelog.de sends us xml files that claim to be iso-8859-1
 * but once we decode the HTML encoded characters they turn
 * into UTF-8 instead. So skip the incorrect encoding
 * declaration and decode the HTML encoded characters */
const char *preprocess_divelog_de(const char *buffer)
{
	char *ret = strstr(buffer, "<DIVELOGSDATA>");

	if (ret) {
		xmlParserCtxtPtr ctx;
		char buf[] = "";
		int i;

		for (i = 0; i < strlen(ret); ++i)
			if (!isascii(ret[i]))
				return buffer;

		ctx = xmlCreateMemoryParserCtxt(buf, sizeof(buf));
		ret = xmlStringLenDecodeEntities(ctx, ret, strlen(ret), XML_SUBSTITUTE_REF, 0, 0, 0);

		return ret;
	}
	return buffer;
}

void parse_xml_buffer(const char *url, const char *buffer, int size,
		      struct dive_table *table, const char **params)
{
	xmlDoc *doc;
	const char *res = preprocess_divelog_de(buffer);

	target_table = table;
	doc = xmlReadMemory(res, strlen(res), url, NULL, 0);
	if (res != buffer)
		free((char *)res);

	if (!doc) {
		report_error(translate("gettextFromC", "Failed to parse '%s'"), url);
		return;
	}
	set_save_userid_local(false);
	set_userid("");
	reset_all();
	dive_start();
	doc = test_xslt_transforms(doc, params);
	traverse(xmlDocGetRootElement(doc));
	dive_end();
	xmlFreeDoc(doc);
}

void parse_mkvi_buffer(struct membuffer *txt, struct membuffer *csv, const char *starttime)
{
	dive_start();
	divedate(starttime, &cur_dive->when);
	dive_end();
}

extern int dm4_events(void *handle, int columns, char **data, char **column)
{
	event_start();
	if (data[1])
		cur_event.time.seconds = atoi(data[1]);

	if (data[2]) {
		switch (atoi(data[2])) {
		case 1:
			/* 1 Mandatory Safety Stop */
			strcpy(cur_event.name, "safety stop (mandatory)");
			break;
		case 3:
			/* 3 Deco */
			/* What is Subsurface's term for going to
				 * deco? */
			strcpy(cur_event.name, "deco");
			break;
		case 4:
			/* 4 Ascent warning */
			strcpy(cur_event.name, "ascent");
			break;
		case 5:
			/* 5 Ceiling broken */
			strcpy(cur_event.name, "violation");
			break;
		case 6:
			/* 6 Mandatory safety stop ceiling error */
			strcpy(cur_event.name, "violation");
			break;
		case 7:
			/* 7 Below deco floor */
			strcpy(cur_event.name, "below floor");
			break;
		case 8:
			/* 8 Dive time alarm */
			strcpy(cur_event.name, "divetime");
			break;
		case 9:
			/* 9 Depth alarm */
			strcpy(cur_event.name, "maxdepth");
			break;
		case 10:
		/* 10 OLF 80% */
		case 11:
			/* 11 OLF 100% */
			strcpy(cur_event.name, "OLF");
			break;
		case 12:
			/* 12 High pO₂ */
			strcpy(cur_event.name, "PO2");
			break;
		case 13:
			/* 13 Air time */
			strcpy(cur_event.name, "airtime");
			break;
		case 17:
			/* 17 Ascent warning */
			strcpy(cur_event.name, "ascent");
			break;
		case 18:
			/* 18 Ceiling error */
			strcpy(cur_event.name, "ceiling");
			break;
		case 19:
			/* 19 Surfaced */
			strcpy(cur_event.name, "surface");
			break;
		case 20:
			/* 20 Deco */
			strcpy(cur_event.name, "deco");
			break;
		case 22:
			/* 22 Mandatory safety stop violation */
			strcpy(cur_event.name, "violation");
			break;
		case 257:
			/* 257 Dive active */
			/* This seems to be given after surface
				 * when descending again. Ignoring it. */
			break;
		case 258:
			/* 258 Bookmark */
			if (data[3]) {
				strcpy(cur_event.name, "heading");
				cur_event.value = atoi(data[3]);
			} else {
				strcpy(cur_event.name, "bookmark");
			}
			break;
		default:
			strcpy(cur_event.name, "unknown");
			cur_event.value = atoi(data[2]);
			break;
		}
	}
	event_end();

	return 0;
}

extern int dm4_tags(void *handle, int columns, char **data, char **column)
{
	if (data[0])
		taglist_add_tag(&cur_dive->tag_list, data[0]);

	return 0;
}

extern int dm4_dive(void *param, int columns, char **data, char **column)
{
	int i, interval, retval = 0;
	sqlite3 *handle = (sqlite3 *)param;
	float *profileBlob;
	unsigned char *tempBlob;
	int *pressureBlob;
	char *err = NULL;
	char get_events_template[] = "select * from Mark where DiveId = %d";
	char get_tags_template[] = "select Text from DiveTag where DiveId = %d";
	char get_events[64];

	dive_start();
	cur_dive->number = atoi(data[0]);

	cur_dive->when = (time_t)(atol(data[1]));
	if (data[2])
		utf8_string(data[2], &cur_dive->notes);

	/*
	 * DM4 stores Duration and DiveTime. It looks like DiveTime is
	 * 10 to 60 seconds shorter than Duration. However, I have no
	 * idea what is the difference and which one should be used.
	 * Duration = data[3]
	 * DiveTime = data[15]
	 */
	if (data[3])
		cur_dive->duration.seconds = atoi(data[3]);
	if (data[15])
		cur_dive->dc.duration.seconds = atoi(data[15]);

	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start();
	dc_settings_start();
	if (data[4])
		utf8_string(data[4], &cur_settings.dc.serial_nr);
	if (data[5])
		utf8_string(data[5], &cur_settings.dc.model);

	cur_settings.dc.deviceid = 0xffffffff;
	dc_settings_end();
	settings_end();

	if (data[6])
		cur_dive->dc.maxdepth.mm = atof(data[6]) * 1000;
	if (data[8])
		cur_dive->dc.airtemp.mkelvin = C_to_mkelvin(atoi(data[8]));
	if (data[9])
		cur_dive->dc.watertemp.mkelvin = C_to_mkelvin(atoi(data[9]));

	/*
	 * TODO: handle multiple cylinders
	 */
	cylinder_start();
	if (data[22] && atoi(data[22]) > 0)
		cur_dive->cylinder[cur_cylinder_index].start.mbar = atoi(data[22]);
	else if (data[10] && atoi(data[10]) > 0)
		cur_dive->cylinder[cur_cylinder_index].start.mbar = atoi(data[10]);
	if (data[23] && atoi(data[23]) > 0)
		cur_dive->cylinder[cur_cylinder_index].end.mbar = (atoi(data[23]));
	if (data[11] && atoi(data[11]) > 0)
		cur_dive->cylinder[cur_cylinder_index].end.mbar = (atoi(data[11]));
	if (data[12])
		cur_dive->cylinder[cur_cylinder_index].type.size.mliter = (atof(data[12])) * 1000;
	if (data[13])
		cur_dive->cylinder[cur_cylinder_index].type.workingpressure.mbar = (atoi(data[13]));
	if (data[20])
		cur_dive->cylinder[cur_cylinder_index].gasmix.o2.permille = atoi(data[20]) * 10;
	if (data[21])
		cur_dive->cylinder[cur_cylinder_index].gasmix.he.permille = atoi(data[21]) * 10;
	cylinder_end();

	if (data[14])
		cur_dive->dc.surface_pressure.mbar = (atoi(data[14]) * 1000);

	interval = data[16] ? atoi(data[16]) : 0;
	profileBlob = (float *)data[17];
	tempBlob = (unsigned char *)data[18];
	pressureBlob = (int *)data[19];
	for (i = 0; interval && i * interval < cur_dive->duration.seconds; i++) {
		sample_start();
		cur_sample->time.seconds = i * interval;
		if (profileBlob)
			cur_sample->depth.mm = profileBlob[i] * 1000;
		else
			cur_sample->depth.mm = cur_dive->dc.maxdepth.mm;

		if (data[18] && data[18][0])
			cur_sample->temperature.mkelvin = C_to_mkelvin(tempBlob[i]);
		if (data[19] && data[19][0])
			cur_sample->cylinderpressure.mbar = pressureBlob[i];
		sample_end();
	}

	snprintf(get_events, sizeof(get_events) - 1, get_events_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_events, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", translate("gettextFromC", "Database query get_events failed.\n"));
		return 1;
	}

	snprintf(get_events, sizeof(get_events) - 1, get_tags_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_tags, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", translate("gettextFromC", "Database query get_tags failed.\n"));
		return 1;
	}

	dive_end();

	/*
	for (i=0; i<columns;++i) {
		fprintf(stderr, "%s\t", column[i]);
	}
	fprintf(stderr, "\n");
	for (i=0; i<columns;++i) {
		fprintf(stderr, "%s\t", data[i]);
	}
	fprintf(stderr, "\n");
	//exit(0);
	*/
	return SQLITE_OK;
}

extern int dm5_dive(void *param, int columns, char **data, char **column)
{
	int i, interval, retval = 0;
	sqlite3 *handle = (sqlite3 *)param;
	unsigned const char *sampleBlob;
	char *err = NULL;
	char get_events_template[] = "select * from Mark where DiveId = %d";
	char get_tags_template[] = "select Text from DiveTag where DiveId = %d";
	char get_events[64];

	dive_start();
	cur_dive->number = atoi(data[0]);

	cur_dive->when = (time_t)(atol(data[1]));
	if (data[2])
		utf8_string(data[2], &cur_dive->notes);

	if (data[3])
		cur_dive->duration.seconds = atoi(data[3]);
	if (data[15])
		cur_dive->dc.duration.seconds = atoi(data[15]);

	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start();
	dc_settings_start();
	if (data[4])
		utf8_string(data[4], &cur_settings.dc.serial_nr);
	if (data[5])
		utf8_string(data[5], &cur_settings.dc.model);

	cur_settings.dc.deviceid = 0xffffffff;
	dc_settings_end();
	settings_end();

	if (data[6])
		cur_dive->dc.maxdepth.mm = atof(data[6]) * 1000;
	if (data[8])
		cur_dive->dc.airtemp.mkelvin = C_to_mkelvin(atoi(data[8]));
	if (data[9])
		cur_dive->dc.watertemp.mkelvin = C_to_mkelvin(atoi(data[9]));

	/*
	 * TODO: handle multiple cylinders
	 */
	cylinder_start();
	if (data[22] && atoi(data[22]) > 0)
		cur_dive->cylinder[cur_cylinder_index].start.mbar = atoi(data[22]);
	else if (data[10] && atoi(data[10]) > 0)
		cur_dive->cylinder[cur_cylinder_index].start.mbar = atoi(data[10]);
	if (data[23] && atoi(data[23]) > 0)
		cur_dive->cylinder[cur_cylinder_index].end.mbar = (atoi(data[23]));
	if (data[11] && atoi(data[11]) > 0)
		cur_dive->cylinder[cur_cylinder_index].end.mbar = (atoi(data[11]));
	if (data[12])
		cur_dive->cylinder[cur_cylinder_index].type.size.mliter = (atof(data[12])) * 1000;
	if (data[13])
		cur_dive->cylinder[cur_cylinder_index].type.workingpressure.mbar = (atoi(data[13]));
	if (data[20])
		cur_dive->cylinder[cur_cylinder_index].gasmix.o2.permille = atoi(data[20]) * 10;
	if (data[21])
		cur_dive->cylinder[cur_cylinder_index].gasmix.he.permille = atoi(data[21]) * 10;
	cylinder_end();

	if (data[14])
		cur_dive->dc.surface_pressure.mbar = (atoi(data[14]) * 1000);

	interval = data[16] ? atoi(data[16]) : 0;
	sampleBlob = (unsigned const char *)data[17];
	for (i = 0; interval && i * interval < cur_dive->duration.seconds; i++) {
		float *depth = (float *)&sampleBlob[i * 16 + 3];
		int32_t temp = (sampleBlob[i * 16 + 10] << 8) + sampleBlob[i * 16 + 11];
		int32_t pressure = (sampleBlob[i * 16 + 9] << 16) + (sampleBlob[i * 16 + 8] << 8) + sampleBlob[i * 16 + 7];

		sample_start();
		cur_sample->time.seconds = i * interval;
		cur_sample->depth.mm = depth[0] * 1000;
		/*
		 * Limit temperatures and cylinder pressures to somewhat
		 * sensible values
		 */
		if (temp >= -10 && temp < 50)
			cur_sample->temperature.mkelvin = C_to_mkelvin(temp);
		if (pressure >= 0 && pressure < 350000)
			cur_sample->cylinderpressure.mbar = pressure;
		sample_end();
	}

	snprintf(get_events, sizeof(get_events) - 1, get_events_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_events, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", translate("gettextFromC", "Database query get_events failed.\n"));
		return 1;
	}

	snprintf(get_events, sizeof(get_events) - 1, get_tags_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_events, &dm4_tags, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", translate("gettextFromC", "Database query get_tags failed.\n"));
		return 1;
	}

	dive_end();

	return SQLITE_OK;
}


int parse_dm4_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
		     struct dive_table *table)
{
	int retval;
	char *err = NULL;
	target_table = table;

	/* StartTime is converted from Suunto's nano seconds to standard
	 * time. We also need epoch, not seconds since year 1. */
	char get_dives[] = "select D.DiveId,StartTime/10000000-62135596800,Note,Duration,SourceSerialNumber,Source,MaxDepth,SampleInterval,StartTemperature,BottomTemperature,D.StartPressure,D.EndPressure,Size,CylinderWorkPressure,SurfacePressure,DiveTime,SampleInterval,ProfileBlob,TemperatureBlob,PressureBlob,Oxygen,Helium,MIX.StartPressure,MIX.EndPressure FROM Dive AS D JOIN DiveMixture AS MIX ON D.DiveId=MIX.DiveId";

	retval = sqlite3_exec(handle, get_dives, &dm4_dive, handle, &err);

	if (retval != SQLITE_OK) {
		fprintf(stderr, translate("gettextFromC", "Database query failed '%s'.\n"), url);
		return 1;
	}

	return 0;
}

int parse_dm5_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
		     struct dive_table *table)
{
	int retval;
	char *err = NULL;
	target_table = table;

	/* StartTime is converted from Suunto's nano seconds to standard
	 * time. We also need epoch, not seconds since year 1. */
	char get_dives[] = "select D.DiveId,StartTime/10000000-62135596800,Note,Duration,SourceSerialNumber,Source,MaxDepth,SampleInterval,StartTemperature,BottomTemperature,D.StartPressure,D.EndPressure,Size,CylinderWorkPressure,SurfacePressure,DiveTime,SampleInterval,SampleBlob,TemperatureBlob,PressureBlob,Oxygen,Helium,MIX.StartPressure,MIX.EndPressure FROM Dive AS D JOIN DiveMixture AS MIX ON D.DiveId=MIX.DiveId";

	retval = sqlite3_exec(handle, get_dives, &dm5_dive, handle, &err);

	if (retval != SQLITE_OK) {
		fprintf(stderr, translate("gettextFromC", "Database query failed '%s'.\n"), url);
		return 1;
	}

	return 0;
}

extern int shearwater_cylinders(void *handle, int columns, char **data, char **column)
{
	cylinder_start();
	if (data[0])
		cur_dive->cylinder[cur_cylinder_index].gasmix.o2.permille = atof(data[0]) * 1000;
	if (data[1])
		cur_dive->cylinder[cur_cylinder_index].gasmix.he.permille = atof(data[1]) * 1000;
	cylinder_end();

	return 0;
}

extern int shearwater_changes(void *handle, int columns, char **data, char **column)
{
	event_start();
	if (data[0])
		cur_event.time.seconds = atoi(data[0]);
	if (data[1]) {
		strcpy(cur_event.name, "gaschange");
		cur_event.value = atof(data[1]) * 100;
	}
	event_end();

	return 0;
}

extern int shearwater_profile_sample(void *handle, int columns, char **data, char **column)
{
	sample_start();
	if (data[0])
		cur_sample->time.seconds = atoi(data[0]);
	if (data[1])
		cur_sample->depth.mm = metric ? atof(data[1]) * 1000 : feet_to_mm(atof(data[1]));
	if (data[2])
		cur_sample->temperature.mkelvin = metric ? C_to_mkelvin(atof(data[2])) : F_to_mkelvin(atof(data[2]));
	if (data[3]) {
		cur_sample->setpoint.mbar = atof(data[3]) * 1000;
		cur_dive->dc.dctype = CCR;
	}
	if (data[4])
		cur_sample->ndl.seconds = atoi(data[4]) * 60;
	if (data[5])
		cur_sample->cns = atoi(data[5]);
	if (data[6])
		cur_sample->stopdepth.mm = metric ? atoi(data[6]) * 1000 : feet_to_mm(atoi(data[6]));

	/* We don't actually have data[3], but it should appear in the
	 * SQL query at some point.
	if (data[3])
		cur_sample->cylinderpressure.mbar = metric ? atoi(data[3]) * 1000 : psi_to_mbar(atoi(data[3]));
	 */
	sample_end();

	return 0;
}

extern int shearwater_dive(void *param, int columns, char **data, char **column)
{
	int retval = 0;
	sqlite3 *handle = (sqlite3 *)param;
	char *err = NULL;
	char get_profile_template[] = "select currentTime,currentDepth,waterTemp,averagePPO2,currentNdl,CNSPercent,decoCeiling from dive_log_records where diveLogId = %d";
	char get_cylinder_template[] = "select fractionO2,fractionHe from dive_log_records where diveLogId = %d group by fractionO2,fractionHe";
	char get_changes_template[] = "select a.currentTime,a.fractionO2,a.fractionHe from dive_log_records as a,dive_log_records as b where a.diveLogId = %d and b.diveLogId = %d and (a.id - 1) = b.id and (a.fractionO2 != b.fractionO2 or a.fractionHe != b.fractionHe) union select min(currentTime),fractionO2,fractionHe from dive_log_records";
	char get_buffer[1024];

	dive_start();
	cur_dive->number = atoi(data[0]);

	cur_dive->when = (time_t)(atol(data[1]));

	if (data[2])
		utf8_string(data[2], &cur_dive->location);
	if (data[3])
		utf8_string(data[3], &cur_dive->buddy);
	if (data[4])
		utf8_string(data[4], &cur_dive->notes);

	metric = atoi(data[5]) == 1 ? 0 : 1;

	/* TODO: verify that metric calculation is correct */
	if (data[6])
		cur_dive->dc.maxdepth.mm = metric ? atof(data[6]) * 1000 : feet_to_mm(atof(data[6]));

	if (data[7])
		cur_dive->dc.duration.seconds = atoi(data[7]) * 60;

	if (data[8])
		cur_dive->dc.surface_pressure.mbar = atoi(data[8]);
	/*
	 * TODO: the deviceid hash should be calculated here.
	 */
	settings_start();
	dc_settings_start();
	if (data[9])
		utf8_string(data[9], &cur_settings.dc.serial_nr);
	if (data[10])
		utf8_string(data[10], &cur_settings.dc.model);

	cur_settings.dc.deviceid = 0xffffffff;
	dc_settings_end();
	settings_end();

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_cylinder_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_cylinders, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", translate("gettextFromC", "Database query get_cylinders failed.\n"));
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_changes_template, cur_dive->number, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_changes, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", translate("gettextFromC", "Database query get_changes failed.\n"));
		return 1;
	}

	snprintf(get_buffer, sizeof(get_buffer) - 1, get_profile_template, cur_dive->number);
	retval = sqlite3_exec(handle, get_buffer, &shearwater_profile_sample, 0, &err);
	if (retval != SQLITE_OK) {
		fprintf(stderr, "%s", translate("gettextFromC", "Database query get_profile_sample failed.\n"));
		return 1;
	}

	dive_end();

	return SQLITE_OK;
}


int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buffer, int size,
			    struct dive_table *table)
{
	int retval;
	char *err = NULL;
	target_table = table;

	char get_dives[] = "select i.diveId,timestamp,location||' / '||site,buddy,notes,imperialUnits,maxDepth,maxTime,startSurfacePressure,computerSerial,computerModel FROM dive_info AS i JOIN dive_logs AS l ON i.diveId=l.diveId";

	retval = sqlite3_exec(handle, get_dives, &shearwater_dive, handle, &err);

	if (retval != SQLITE_OK) {
		fprintf(stderr, translate("gettextFromC", "Database query failed '%s'.\n"), url);
		return 1;
	}

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
	  { "manualcsv", "manualcsv2xml.xslt", NULL },
	  { NULL, }
  };

static xmlDoc *test_xslt_transforms(xmlDoc *doc, const char **params)
{
	struct xslt_files *info = xslt_files;
	xmlDoc *transformed;
	xsltStylesheetPtr xslt = NULL;
	xmlNode *root_element = xmlDocGetRootElement(doc);
	char *attribute;

	while (info->root) {
		if ((strcasecmp(root_element->name, info->root) == 0)) {
			if (info->attribute == NULL)
				break;
			else if (xmlGetProp(root_element, info->attribute) != NULL)
				break;
		}
		info++;
	}

	if (info->root) {
		attribute = xmlGetProp(xmlFirstElementChild(root_element), "name");
		if (attribute) {
			if (strcasecmp(attribute, "subsurface") == 0) {
				free((void *)attribute);
				return doc;
			}
			free((void *)attribute);
		}

		xmlSubstituteEntitiesDefault(1);
		xslt = get_stylesheet(info->file);
		if (xslt == NULL) {
			report_error(translate("gettextFromC", "Can't open stylesheet %s"), info->file);
			return doc;
		}
		transformed = xsltApplyStylesheet(xslt, doc, params);
		xmlFreeDoc(doc);
		xsltFreeStylesheet(xslt);

		return transformed;
	}
	return doc;
}
