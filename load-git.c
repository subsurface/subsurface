#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <git2.h>

#include "dive.h"
#include "device.h"
#include "membuffer.h"

struct keyword_action {
	const char *keyword;
	void (*fn)(char *, struct membuffer *, void *);
};
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

extern degrees_t parse_degrees(char *buf, char **end);
static void parse_dive_gps(char *line, struct membuffer *str, void *_dive)
{
	struct dive *dive = _dive;

	dive->latitude = parse_degrees(line, &line);
	dive->longitude = parse_degrees(line, &line);
}

static char *get_utf8(struct membuffer *b)
{
	int len = b->len;
	char *res;

	if (!len)
		return NULL;
	res = malloc(len+1);
	if (res) {
		memcpy(res, b->buffer, len);
		res[len] = 0;
	}
	return res;
}

static temperature_t get_temperature(const char *line)
{
	temperature_t t;
	t.mkelvin = C_to_mkelvin(ascii_strtod(line, NULL));
	return t;
}

static depth_t get_depth(const char *line)
{
	depth_t d;
	d.mm = rint(1000*ascii_strtod(line, NULL));
	return d;
}

static volume_t get_volume(const char *line)
{
	volume_t v;
	v.mliter = rint(1000*ascii_strtod(line, NULL));
	return v;
}

static weight_t get_weight(const char *line)
{
	weight_t w;
	w.grams = rint(1000*ascii_strtod(line, NULL));
	return w;
}

static pressure_t get_pressure(const char *line)
{
	pressure_t p;
	p.mbar = rint(1000*ascii_strtod(line, NULL));
	return p;
}

static fraction_t get_fraction(const char *line)
{
	fraction_t f;
	f.permille = rint(10*ascii_strtod(line, NULL));
	return f;
}

static void update_date(timestamp_t *when, const char *line)
{
	unsigned yyyy, mm, dd;
	struct tm tm;

	if (sscanf(line, "%04u-%02u-%02u", &yyyy, &mm, &dd) != 3)
		return;
	utc_mkdate(*when, &tm);
	tm.tm_year = yyyy - 1900;
	tm.tm_mon = mm - 1;
	tm.tm_mday = dd;
	*when = utc_mktime(&tm);
}

static void update_time(timestamp_t *when, const char *line)
{
	unsigned h, m, s = 0;
	struct tm tm;

	if (sscanf(line, "%02u:%02u:%02u", &h, &m, &s) < 2)
		return;
	utc_mkdate(*when, &tm);
	tm.tm_hour = h;
	tm.tm_min = m;
	tm.tm_sec = s;
	*when = utc_mktime(&tm);
}

static duration_t get_duration(const char *line)
{
	int m = 0, s = 0;
	duration_t d;
	sscanf(line, "%d:%d", &m, &s);
	d.seconds = m*60+s;
	return d;
}

static int get_index(const char *line)
{ return atoi(line); }
static int get_hex(const char *line)
{ return strtoul(line, NULL, 16); }

static void parse_dive_location(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->location = get_utf8(str); }

static void parse_dive_divemaster(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->divemaster = get_utf8(str); }

static void parse_dive_buddy(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->buddy = get_utf8(str); }

static void parse_dive_suit(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->suit = get_utf8(str); }

static void parse_dive_notes(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->notes = get_utf8(str); }

/*
 * We can have multiple tags in the membuffer. They are separated by
 * NUL bytes.
 */
static void parse_dive_tags(char *line, struct membuffer *str, void *_dive)
{
	struct dive *dive = _dive;
	const char *tag;
	int len = str->len;

	if (!len)
		return;

	/* Make sure there is a NUL at the end too */
	tag = mb_cstring(str);
	for (;;) {
		int taglen = strlen(tag);
		if (taglen)
			taglist_add_tag(dive->tag_list, tag);
		len -= taglen;
		if (!len)
			return;
		tag += taglen+1;
		len--;
	}
}

static void parse_dive_airtemp(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->airtemp = get_temperature(line); }

static void parse_dive_watertemp(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->watertemp = get_temperature(line); }

static void parse_dive_duration(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->duration = get_duration(line); }

static void parse_dive_rating(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->rating = get_index(line); }

static void parse_dive_visibility(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->visibility = get_index(line); }

static void parse_dive_notrip(char *line, struct membuffer *str, void *_dive)
{ struct dive *dive = _dive; dive->tripflag = NO_TRIP; }

/* Parse key=val parts of samples and cylinders etc */
static char *parse_keyvalue_entry(void (*fn)(void *, const char *, const char *), void *fndata, char *line)
{
	char *key = line, *val, c;

	while ((c = *line) != 0) {
		if (isspace(c) || c == '=')
			break;
		line++;
	}

	if (c == '=')
		*line++ = 0;
	val = line;

	while ((c = *line) != 0) {
		if (isspace(c))
			break;
		line++;
	}
	if (c)
		*line++ = 0;

	fn(fndata, key, val);
	return line;
}

static int cylinder_index, weightsystem_index;

static void parse_cylinder_keyvalue(void *_cylinder, const char *key, const char *value)
{
	cylinder_t *cylinder = _cylinder;
	if (!strcmp(key, "vol")) {
		cylinder->type.size = get_volume(value);
		return;
	}
	if (!strcmp(key, "workpressure")) {
		cylinder->type.workingpressure = get_pressure(value);
		return;
	}
	/* This is handled by the "get_utf8()" */
	if (!strcmp(key, "description"))
		return;
	if (!strcmp(key, "o2")) {
		cylinder->gasmix.o2 = get_fraction(value);
		return;
	}
	if (!strcmp(key, "he")) {
		cylinder->gasmix.he = get_fraction(value);
		return;
	}
	if (!strcmp(key, "start")) {
		cylinder->start = get_pressure(value);
		return;
	}
	if (!strcmp(key, "end")) {
		cylinder->end = get_pressure(value);
		return;
	}
	report_error("Unknown cylinder key/value pair (%s/%s)", key, value);
}

static void parse_dive_cylinder(char *line, struct membuffer *str, void *_dive)
{
	struct dive *dive = _dive;
	cylinder_t *cylinder = dive->cylinder + cylinder_index;

	cylinder_index++;
	cylinder->type.description = get_utf8(str);
	for (;;) {
		char c;
		while (isspace(c = *line))
			line++;
		if (!c)
			break;
		line = parse_keyvalue_entry(parse_cylinder_keyvalue, cylinder, line);
	}
}

static void parse_weightsystem_keyvalue(void *_ws, const char *key, const char *value)
{
	weightsystem_t *ws = _ws;
	if (!strcmp(key, "weight")) {
		ws->weight = get_weight(value);
		return;
	}
	/* This is handled by the "get_utf8()" */
	if (!strcmp(key, "description"))
		return;
	report_error("Unknown weightsystem key/value pair (%s/%s)", key, value);
}

static void parse_dive_weightsystem(char *line, struct membuffer *str, void *_dive)
{
	struct dive *dive = _dive;
	weightsystem_t *ws = dive->weightsystem + weightsystem_index;

	weightsystem_index++;
	ws->description = get_utf8(str);
	for (;;) {
		char c;
		while (isspace(c = *line))
			line++;
		if (!c)
			break;
		line = parse_keyvalue_entry(parse_weightsystem_keyvalue, ws, line);
	}
}

static int match_action(char *line, struct membuffer *str, void *data,
	struct keyword_action *action, unsigned nr_action)
{
	char *p = line, c;
	unsigned low, high;

	while ((c = *p) >= 'a' && c <= 'z')
		p++;
	if (p == line)
		return -1;
	switch (c) {
	case 0:
		break;
	case ' ':
		*p++ = 0;
		break;
	default:
		return -1;
	}

	/* Standard binary search in a table */
	low = 0;
	high = nr_action;
	while (low < high) {
		unsigned mid = (low+high)/2;
		struct keyword_action *a = action + mid;
		int cmp = strcmp(line, a->keyword);
		if (!cmp) {
			a->fn(p, str, data);
			return 0;
		}
		if (cmp < 0)
			high = mid;
		else
			low = mid+1;
	}
report_error("Unmatched action '%s'", line);
	return -1;
}

/* FIXME! We should do the array thing here too. */
static void parse_sample_keyvalue(void *_sample, const char *key, const char *value)
{
	struct sample *sample = _sample;

	if (!strcmp(key, "sensor")) {
		sample->sensor = atoi(value);
		return;
	}
	if (!strcmp(key, "ndl")) {
		sample->ndl = get_duration(value);
		return;
	}
	if (!strcmp(key, "in_deco")) {
		sample->in_deco = atoi(value);
		return;
	}
	if (!strcmp(key, "stoptime")) {
		sample->stoptime = get_duration(value);
		return;
	}
	if (!strcmp(key, "stopdepth")) {
		sample->stopdepth = get_depth(value);
		return;
	}
	if (!strcmp(key, "cns")) {
		sample->cns = atoi(value);
		return;
	}
	if (!strcmp(key, "po2")) {
		pressure_t p = get_pressure(value);
		// Ugh, typeless.
		sample->po2 = p.mbar;
		return;
	}
	if (!strcmp(key, "heartbeat")) {
		sample->heartbeat = atoi(value);
		return;
	}
	if (!strcmp(key, "bearing")) {
		sample->bearing = atoi(value);
		return;
	}
	report_error("Unexpected sample key/value pair (%s/%s)", key, value);
}

static char *parse_sample_unit(struct sample *sample, double val, char *unit)
{
	char *end = unit, c;

	/* Skip over the unit */
	while ((c = *end) != 0) {
		if (isspace(c)) {
			*end++ = 0;
			break;
		}
		end++;
	}

	/* The units are "°C", "m" or "bar", so let's just look at the first character */
	switch (*unit) {
	case 'm':
		sample->depth.mm = rint(1000*val);
		break;
	case 'b':
		sample->cylinderpressure.mbar = rint(1000*val);
		break;
	default:
		sample->temperature.mkelvin = C_to_mkelvin(val);
		break;
	}

	return end;
}

/*
 * By default the sample data does not change unless the
 * save-file gives an explicit new value. So we copy the
 * data from the previous sample if one exists, and then
 * the parsing will update it as necessary.
 *
 * There are a few exceptions, like the sample pressure:
 * missing sample pressure doesn't mean "same as last
 * time", but "interpolate". We clear those ones
 * explicitly.
 */
static struct sample *new_sample(struct divecomputer *dc)
{
	struct sample *sample = prepare_sample(dc);
	if (sample != dc->sample) {
		memcpy(sample, sample-1, sizeof(struct sample));
		sample->cylinderpressure.mbar = 0;
	}
	return sample;
}

static void sample_parser(char *line, struct divecomputer *dc)
{
	int m,s;
	struct sample *sample = new_sample(dc);

	m = strtol(line, &line, 10);
	if (*line == ':')
		s = strtol(line+1, &line, 10);
	sample->time.seconds = m*60+s;

	for (;;) {
		char c;

		while (isspace(c = *line))
			line++;
		if (!c)
			break;
		/* Less common sample entries have a name */
		if (c >= 'a' && c <= 'z') {
			line = parse_keyvalue_entry(parse_sample_keyvalue, sample, line);
		} else {
			const char *end;
			double val = ascii_strtod(line, &end);
			if (end == line) {
				report_error("Odd sample data: %s", line);
				break;
			}
			line = (char *)end;
			line = parse_sample_unit(sample, val, line);
		}
	}
	finish_sample(dc);
}

static void parse_dc_airtemp(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->airtemp = get_temperature(line); }

static void parse_dc_date(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; update_date(&dc->when, line); }

static void parse_dc_deviceid(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->deviceid = get_hex(line); }

static void parse_dc_diveid(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->diveid = get_hex(line); }

static void parse_dc_duration(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->duration = get_duration(line); }

static void parse_dc_maxdepth(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->maxdepth = get_depth(line); }

static void parse_dc_meandepth(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->meandepth = get_depth(line); }

static void parse_dc_model(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->model = get_utf8(str); }

static void parse_dc_surfacepressure(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->surface_pressure = get_pressure(line); }

static void parse_dc_surfacetime(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->surfacetime = get_duration(line); }

static void parse_dc_time(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; update_time(&dc->when, line); }

static void parse_dc_watertemp(char *line, struct membuffer *str, void *_dc)
{ struct divecomputer *dc = _dc; dc->watertemp = get_temperature(line); }

static void parse_event_keyvalue(void *_event, const char *key, const char *value)
{
	struct event *event = _event;
	int val = atoi(value);

	if (!strcmp(key, "type")) {
		event->type = val;
	} else if (!strcmp(key, "flags")) {
		event->flags = val;
	} else if (!strcmp(key, "value")) {
		event->value = val;
	} else if (!strcmp(key, "name")) {
		/* We get the name from the string handling */
	} else
		report_error("Unexpected event key/value pair (%s/%s)", key, value);
}

static void parse_dc_event(char *line, struct membuffer *str, void *_dc)
{
	int m, s;
	const char *name;
	struct divecomputer *dc = _dc;
	struct event event = { 0 };

	m = strtol(line, &line, 10);
	if (*line == ':')
		s = strtol(line+1, &line, 10);
	event.time.seconds = m*60+s;

	for (;;) {
		char c;
		while (isspace(c = *line))
			line++;
		if (!c)
			break;
		line = parse_keyvalue_entry(parse_event_keyvalue, &event, line);
	}

	name = "";
	if (str->len)
		name = mb_cstring(str);
	add_event(dc, event.time.seconds, event.type, event.flags, event.value, name);
}

static void parse_trip_date(char *line, struct membuffer *str, void *_trip)
{ dive_trip_t *trip = _trip; update_date(&trip->when, line); }

static void parse_trip_time(char *line, struct membuffer *str, void *_trip)
{ dive_trip_t *trip = _trip; update_time(&trip->when, line); }

static void parse_trip_location(char *line, struct membuffer *str, void *_trip)
{ dive_trip_t *trip = _trip; trip->location = get_utf8(str); }

static void parse_trip_notes(char *line, struct membuffer *str, void *_trip)
{ dive_trip_t *trip = _trip; trip->notes = get_utf8(str); }

/* These need to be sorted! */
struct keyword_action dc_action[] = {
#undef D
#define D(x) { #x, parse_dc_ ## x }
	D(airtemp), D(date), D(deviceid), D(diveid), D(duration),
	D(event), D(maxdepth), D(meandepth), D(model),
	D(surfacepressure), D(surfacetime), D(time), D(watertemp),
};

/* Sample lines start with a space or a number */
static void divecomputer_parser(char *line, struct membuffer *str, void *_dc)
{
	char c = *line;
	if (c < 'a' || c > 'z')
		sample_parser(line, _dc);
	match_action(line, str, _dc, dc_action, ARRAY_SIZE(dc_action));
}

/* These need to be sorted! */
struct keyword_action dive_action[] = {
#undef D
#define D(x) { #x, parse_dive_ ## x }
	D(airtemp), D(buddy), D(cylinder), D(divemaster), D(duration),
	D(gps), D(location), D(notes), D(notrip), D(rating), D(suit),
	D(tags), D(visibility), D(watertemp), D(weightsystem)
};

static void dive_parser(char *line, struct membuffer *str, void *_dive)
{
	match_action(line, str, _dive, dive_action, ARRAY_SIZE(dive_action));
}

/* These need to be sorted! */
struct keyword_action trip_action[] = {
#undef D
#define D(x) { #x, parse_trip_ ## x }
	D(date), D(location), D(notes), D(time),
};

static void trip_parser(char *line, struct membuffer *str, void *_trip)
{
	match_action(line, str, _trip, trip_action, ARRAY_SIZE(trip_action));
}

/*
 * We have a very simple line-based interface, with the small
 * complication that lines can have strings in the middle, and
 * a string can be multiple lines.
 *
 * The UTF-8 string escaping is *very* simple, though:
 *
 *  - a string starts and ends with double quotes (")
 *
 *  - inside the string we escape:
 *     (a) double quotes with '\"'
 *     (b) backslash (\) with '\\'
 *
 *  - additionally, for human readability, we escape
 *    newlines with '\n\t', with the exception that
 *    consecutive newlines are left unescaped (so an
 *    empty line doesn't become a line with just a tab
 *    on it).
 *
 * Also, while the UTF-8 string can have arbitrarily
 * long lines, the non-string parts of the lines are
 * never long, so we can use a small temporary buffer
 * on stack for that part.
 *
 * Also, note that if a line has one or more strings
 * in it:
 *
 *  - each string will be represented as a single '"'
 *    character in the output.
 *
 *  - all string will exist in the same 'membuffer',
 *    separated by NUL characters (that cannot exist
 *    in a string, not even quoted).
 */
static const char *parse_one_string(const char *buf, const char *end, struct membuffer *b)
{
	const char *p = buf;

	/*
	 * We turn multiple strings one one line (think dive tags) into one
	 * membuffer that has NUL characters in between strings.
	 */
	if (b->len)
		put_bytes(b, "", 1);

	while (p < end) {
		char replace;

		switch (*p++) {
		default:
			continue;
		case '\n':
			if (p < end && *p == '\t') {
				replace = '\n';
				break;
			}
			continue;
		case '\\':
			if (p < end) {
				replace = *p;
				break;
			}
			continue;
		case '"':
			replace = 0;
			break;
		}
		put_bytes(b, buf, p - buf - 1);
		if (!replace)
			break;
		put_bytes(b, &replace, 1);
		buf = ++p;
	}
	return p;
}

typedef void (line_fn_t)(char *, struct membuffer *, void *);
#define MAXLINE 100
static unsigned parse_one_line(const char *buf, unsigned size, line_fn_t *fn, void *fndata, struct membuffer *b)
{
	const char *end = buf + size;
	const char *p = buf;
	char line[MAXLINE+1];
	int off = 0;

	while (p < end) {
		char c = *p++;
		if (c == '\n')
			break;
		line[off] = c;
		off++;
		if (off > MAXLINE)
			off = MAXLINE;
		if (c == '"')
			p = parse_one_string(p, end, b);
	}
	line[off] = 0;
	fn(line, b, fndata);
	return p - buf;
}

/*
 * We keep on re-using the membuffer that we use for
 * strings, but the callback function can "steal" it by
 * saving its value and just clear the original.
 */
static void for_each_line(git_blob *blob, line_fn_t *fn, void *fndata)
{
	const char *content = git_blob_rawcontent(blob);
	unsigned int size = git_blob_rawsize(blob);
	struct membuffer str = { 0 };

	while (size) {
		unsigned int n = parse_one_line(content, size, fn, fndata, &str);
		content += n;
		size -= n;

		/* Re-use the allocation, but forget the data */
		str.len = 0;
	}
	free_buffer(&str);
}

#define GIT_WALK_OK   0
#define GIT_WALK_SKIP 1

static struct divecomputer *active_dc;
static struct dive *active_dive;
static dive_trip_t *active_trip;

static struct dive *create_new_dive(timestamp_t when)
{
	struct dive *dive = alloc_dive();

	/* We'll fill in more data from the dive file */
	dive->when = when;

	if (active_trip)
		add_dive_to_trip(dive, active_trip);
	return dive;
}

static dive_trip_t *create_new_trip(int yyyy, int mm, int dd)
{
	dive_trip_t *trip = calloc(1, sizeof(dive_trip_t));
	struct tm tm = { 0 };

	/* We'll fill in the real data from the trip descriptor file */
	tm.tm_year = yyyy;
	tm.tm_mon = mm-1;
	tm.tm_mday = dd;
	trip->when = utc_mktime(&tm);

	return trip;
}

static bool validate_date(int yyyy, int mm, int dd)
{
	return yyyy > 1970 && yyyy < 3000 &&
		mm > 0 && mm < 13 &&
		dd > 0 && dd < 32;
}

static bool validate_time(int h, int m, int s)
{
	return h >= 0 && h < 24 &&
		m >= 0 && m < 60 &&
		s >=0 && s <= 60;
}

/*
 * Dive trip directory, name is 'nn-alphabetic[~hex]'
 */
static int dive_trip_directory(const char *root, const char *name)
{
	int yyyy = -1, mm = -1, dd = -1;

	if (sscanf(root, "%d/%d", &yyyy, &mm) != 2)
		return GIT_WALK_SKIP;
	dd = atoi(name);
	if (!validate_date(yyyy, mm, dd))
		return GIT_WALK_SKIP;
	active_trip = create_new_trip(yyyy, mm, dd);
	return GIT_WALK_OK;
}

/*
 * Dive directory, name is [[yyyy-]mm-]nn-ddd-hh:mm:ss[~hex],
 * and 'timeoff' points to what should be the time part of
 * the name (the first digit of the hour).
 *
 * The root path will be of the form yyyy/mm[/tripdir],
 */
static int dive_directory(const char *root, const char *name, int timeoff)
{
	int yyyy = -1, mm = -1, dd = -1;
	int h, m, s;
	int mday_off = timeoff - 7;
	int month_off = mday_off - 3;
	int year_off = month_off - 5;
	struct tm tm;

	/* There has to be a mday */
	if (mday_off < 0)
		return GIT_WALK_SKIP;
	if (name[timeoff-1] != '-')
		return GIT_WALK_SKIP;

	/* Get the time of day */
	if (sscanf(name+timeoff, "%d:%d:%d", &h, &m, &s) != 3)
		return GIT_WALK_SKIP;
	if (!validate_time(h, m, s))
		return GIT_WALK_SKIP;

	/*
	 * Using the "git_tree_walk()" interface is simple, but
	 * it kind of sucks as an interface because there is
	 * no sane way to pass the hierarchy to the callbacks.
	 * The "payload" is a fixed one-time thing: we'd like
	 * the "current trip" to be passed down to the dives
	 * that get parsed under that trip, but we can't.
	 *
	 * So "active_trip" is not the trip that is in the hierarchy
	 * _above_ us, it's just the trip that was _before_ us. But
	 * if a dive is not in a trip at all, we can't tell.
	 *
	 * We could just do a better walker that passes the
	 * return value around, but we hack around this by
	 * instead looking at the one hierarchical piece of
	 * data we have: the pathname to the current entry.
	 *
	 * This is pretty hacky. The magic '8' is the length
	 * of a pathname of the form 'yyyy/mm/'.
	 */
	if (strlen(root) == 8)
		active_trip = NULL;

	/*
	 * Get the date. The day of the month is in the dive directory
	 * name, the year and month might be in the path leading up
	 * to it.
	 */
	dd = atoi(name + mday_off);
	if (year_off < 0) {
		if (sscanf(root, "%d/%d", &yyyy, &mm) != 2)
			return GIT_WALK_SKIP;
	} else
		yyyy = atoi(name + year_off);
	if (month_off >= 0)
		mm = atoi(name + month_off);

	if (!validate_date(yyyy, mm, dd))
		return GIT_WALK_SKIP;

	/* Ok, close enough. We've gotten sufficient information */
	memset(&tm, 0, sizeof(tm));
	tm.tm_hour = h;
	tm.tm_min = m;
	tm.tm_sec = s;
	tm.tm_year = yyyy - 1900;
	tm.tm_mon = mm-1;
	tm.tm_mday = dd;

	if (active_dive)
		record_dive(active_dive);
	active_dive = create_new_dive(utc_mktime(&tm));
	return GIT_WALK_OK;
}

/*
 * Return the length of the string without the unique part.
 */
static int nonunique_length(const char *str)
{
	int len = 0;

	for (;;) {
		char c = *str++;
		if (!c || c == '~')
			return len;
		len++;
	}
}

/*
 * When hitting a directory node, we have a couple of cases:
 *
 *  - It's just a date entry - all numeric (either year or month):
 *
 *       [yyyy|mm]
 *
 *    We don't do anything with these, we just traverse into them.
 *    The numeric data will show up as part of the full path when
 *    we hit more interesting entries.
 *
 *  - It's a trip directory. The name will be of the form
 *
 *       nn-alphabetic[~hex]
 *
 *    where 'nn' is the day of the month (year and month will be
 *    encoded in the path leading up to this).
 *
 *  - It's a dive directory. The name will be of the form
 *
 *       [[yyyy-]mm-]nn-ddd-hh:mm:ss[~hex]
 *
 *    which describes the date and time of a dive (yyyy and mm
 *    are optional, and may be encoded in the path leading up to
 *    the dive).
 *
 *  - It's some random non-dive-data directory.
 *
 *    Subsurface doesn't create these yet, but maybe we'll encode
 *    pictures etc. If it doesn't match the above patterns, we'll
 *    ignore them for dive loading purposes, and not even recurse
 *    into them.
 */
static int walk_tree_directory(const char *root, const git_tree_entry *entry)
{
	const char *name = git_tree_entry_name(entry);
	int digits = 0, len;
	char c;

	while (isdigit(c = name[digits]))
		digits++;

	/* Doesn't start with two or four digits? Skip */
	if (digits != 4 && digits != 2)
		return GIT_WALK_SKIP;

	/* Only digits? Do nothing, but recurse into it */
	if (!c)
		return GIT_WALK_OK;

	/* All valid cases need to have a slash following */
	if (c != '-')
		return GIT_WALK_SKIP;

	/* Do a quick check for a common dive case */
	len = nonunique_length(name);

	/*
	 * We know the len is at least 3, because we had at least
	 * two digits and a dash
	 */
	if (name[len-3] == ':')
		return dive_directory(root, name, len-8);

	if (digits != 2)
		return GIT_WALK_SKIP;

	return dive_trip_directory(root, name);
}

git_blob *git_tree_entry_blob(git_repository *repo, const git_tree_entry *entry)
{
	const git_oid *id = git_tree_entry_id(entry);
	git_blob *blob;

	if (git_blob_lookup(&blob, repo, id))
		return NULL;
	return blob;
}

static struct divecomputer *create_new_dc(struct dive *dive)
{
	struct divecomputer *dc = &dive->dc;

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
	return dc;
}

/*
 * We should *really* try to delay the dive computer data parsing
 * until necessary, in order to reduce load-time. The parsing is
 * cheap, but the loading of the git blob into memory can be pretty
 * costly.
 */
static int parse_divecomputer_entry(git_repository *repo, const git_tree_entry *entry, const char *suffix)
{
	git_blob *blob = git_tree_entry_blob(repo, entry);

	if (!blob)
		return report_error("Unable to read divecomputer file");

	active_dc = create_new_dc(active_dive);
	for_each_line(blob, divecomputer_parser, active_dc);
	git_blob_free(blob);
	active_dc = NULL;
	return 0;
}

static int parse_dive_entry(git_repository *repo, const git_tree_entry *entry, const char *suffix)
{
	struct dive *dive = active_dive;
	git_blob *blob = git_tree_entry_blob(repo, entry);
	if (!blob)
		return report_error("Unable to read dive file");
	if (*suffix)
		dive->number = atoi(suffix+1);
	cylinder_index = weightsystem_index = 0;
	for_each_line(blob, dive_parser, active_dive);
	git_blob_free(blob);
	return 0;
}

static int parse_trip_entry(git_repository *repo, const git_tree_entry *entry)
{
	git_blob *blob = git_tree_entry_blob(repo, entry);
	if (!blob)
		return report_error("Unable to read trip file");
	for_each_line(blob, trip_parser, active_trip);
	git_blob_free(blob);
	return 0;
}

static int walk_tree_file(const char *root, const git_tree_entry *entry, git_repository *repo)
{
	struct dive *dive = active_dive;
	dive_trip_t *trip = active_trip;
	const char *name = git_tree_entry_name(entry);

	if (dive && !strncmp(name, "Divecomputer", 12))
		return parse_divecomputer_entry(repo, entry, name+12);
	if (dive && !strncmp(name, "Dive", 4))
		return parse_dive_entry(repo, entry, name+4);
	if (trip && !strcmp(name, "00-Trip"))
		return parse_trip_entry(repo, entry);
	report_error("Unknown file %s%s (%p %p)", root, name, dive, trip);
	return GIT_WALK_SKIP;
}

static int walk_tree_cb(const char *root, const git_tree_entry *entry, void *payload)
{
	git_repository *repo = payload;
	git_filemode_t mode = git_tree_entry_filemode(entry);

	if (mode == GIT_FILEMODE_TREE)
		return walk_tree_directory(root, entry);

	walk_tree_file(root, entry, repo);
	/* Ignore failed blob loads */
	return GIT_WALK_OK;
}

static int load_dives_from_tree(git_repository *repo, git_tree *tree)
{
	git_tree_walk(tree, GIT_TREEWALK_PRE, walk_tree_cb, repo);
	return 0;
}

static int do_git_load(git_repository *repo, const char *branch)
{
	int ret;
	git_reference *ref;
	git_object *tree;

	ret = git_branch_lookup(&ref, repo, branch, GIT_BRANCH_LOCAL);
	if (ret)
		return report_error("Unable to look up branch '%s'", branch);
	if (git_reference_peel(&tree, ref, GIT_OBJ_TREE))
		return report_error("Could not look up tree of branch '%s'", branch);
	ret = load_dives_from_tree(repo, (git_tree *) tree);
	git_object_free(tree);
	return ret;
}

int git_load_dives(char *where)
{
	int ret, len;
	git_repository *repo;
	char *loc, *branch;

	/* Jump over the "git" marker */
	loc = where + 3;
	while (isspace(*loc))
		loc++;

	/* Trim whitespace from the end */
	len = strlen(loc);
	while (len && isspace(loc[len-1]))
		loc[--len] = 0;

	/* Find a branch name if there is any */
	branch = strrchr(loc, ':');
	if (branch)
		*branch++ = 0;

	if (git_repository_open(&repo, loc))
		return report_error("Unable to open git repository at '%s' (branch '%s')", loc, branch);

	ret = do_git_load(repo, branch);
	git_repository_free(repo);
	if (active_dive)
		record_dive(active_dive);
	active_dive = NULL;
	return ret;
}
