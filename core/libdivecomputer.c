// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include "gettext.h"
#include "dive.h"
#include "device.h"
#include "divelist.h"
#include "display.h"

#include <libdivecomputer/version.h>
#include <libdivecomputer/usbhid.h>
#include <libdivecomputer/serial.h>
#include <libdivecomputer/irda.h>

#include "libdivecomputer.h"
#include "core/version.h"

//
// If we have an old libdivecomputer, it doesn't
// have the new DC_TANKINFO bits, but just volume
// type information.
//
#ifndef DC_TANKINFO_METRIC
#define DC_TANKINFO_METRIC	DC_TANKVOLUME_METRIC
#define DC_TANKINFO_IMPERIAL	DC_TANKVOLUME_IMPERIAL
#define DC_TANKINFO_CC_O2	0
#define DC_TANKINFO_CC_DILUENT	0
#endif

char *dumpfile_name;
char *logfile_name;
const char *progress_bar_text = "";
void (*progress_callback)(const char *text) = NULL;
double progress_bar_fraction = 0.0;

static int stoptime, stopdepth, ndl, po2, cns, heartbeat, bearing;
static bool in_deco, first_temp_is_air;
static int current_gas_index;

/* logging bits from libdivecomputer */
#ifndef __ANDROID__
#define INFO(context, fmt, ...)	fprintf(stderr, "INFO: " fmt "\n", ##__VA_ARGS__)
#define ERROR(context, fmt, ...)	fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__)
#else
#include <android/log.h>
#define INFO(context, fmt, ...)	__android_log_print(ANDROID_LOG_DEBUG, __FILE__, "INFO: " fmt "\n", ##__VA_ARGS__)
#define ERROR(context, fmt, ...)	__android_log_print(ANDROID_LOG_DEBUG, __FILE__, "ERROR: " fmt "\n", ##__VA_ARGS__)
#endif

/*
 * Directly taken from libdivecomputer's examples/common.c to improve
 * the error messages resulting from libdc's return codes
 */
const char *errmsg (dc_status_t rc)
{
	switch (rc) {
	case DC_STATUS_SUCCESS:
		return "Success";
	case DC_STATUS_UNSUPPORTED:
		return "Unsupported operation";
	case DC_STATUS_INVALIDARGS:
		return "Invalid arguments";
	case DC_STATUS_NOMEMORY:
		return "Out of memory";
	case DC_STATUS_NODEVICE:
		return "No device found";
	case DC_STATUS_NOACCESS:
		return "Access denied";
	case DC_STATUS_IO:
		return "Input/output error";
	case DC_STATUS_TIMEOUT:
		return "Timeout";
	case DC_STATUS_PROTOCOL:
		return "Protocol error";
	case DC_STATUS_DATAFORMAT:
		return "Data format error";
	case DC_STATUS_CANCELLED:
		return "Cancelled";
	default:
		return "Unknown error";
	}
}

static dc_status_t create_parser(device_data_t *devdata, dc_parser_t **parser)
{
	return dc_parser_new(parser, devdata->device);
}

static int parse_gasmixes(device_data_t *devdata, struct dive *dive, dc_parser_t *parser, unsigned int ngases)
{
	static bool shown_warning = false;
	unsigned int i;
	int rc;

#if DC_VERSION_CHECK(0, 5, 0) && defined(DC_GASMIX_UNKNOWN)
	unsigned int ntanks = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_TANK_COUNT, 0, &ntanks);
	if (rc == DC_STATUS_SUCCESS) {
		if (ntanks && ntanks < ngases) {
			shown_warning = true;
			report_error("Warning: different number of gases (%d) and cylinders (%d)", ngases, ntanks);
		} else if (ntanks > ngases) {
			shown_warning = true;
			report_error("Warning: smaller number of gases (%d) than cylinders (%d). Assuming air.", ngases, ntanks);
		}
	}
#endif
	bool no_volume = true;

	for (i = 0; i < MAX_CYLINDERS && (i < ngases || i < ntanks); i++) {
		if (i < ngases) {
			dc_gasmix_t gasmix = { 0 };
			int o2, he;

			rc = dc_parser_get_field(parser, DC_FIELD_GASMIX, i, &gasmix);
			if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED)
				return rc;

			o2 = lrint(gasmix.oxygen * 1000);
			he = lrint(gasmix.helium * 1000);

			/* Ignore bogus data - libdivecomputer does some crazy stuff */
			if (o2 + he <= O2_IN_AIR || o2 > 1000) {
				if (!shown_warning) {
					shown_warning = true;
					report_error("unlikely dive gas data from libdivecomputer: o2 = %d he = %d", o2, he);
				}
				o2 = 0;
			}
			if (he < 0 || o2 + he > 1000) {
				if (!shown_warning) {
					shown_warning = true;
					report_error("unlikely dive gas data from libdivecomputer: o2 = %d he = %d", o2, he);
				}
				he = 0;
			}
			dive->cylinder[i].gasmix.o2.permille = o2;
			dive->cylinder[i].gasmix.he.permille = he;
		} else {
			dive->cylinder[i].gasmix.o2.permille = 0;
			dive->cylinder[i].gasmix.he.permille = 0;
		}

#if DC_VERSION_CHECK(0, 5, 0) && defined(DC_GASMIX_UNKNOWN)
		if (i < ntanks) {
			dc_tank_t tank = { 0 };
			rc = dc_parser_get_field(parser, DC_FIELD_TANK, i, &tank);
			if (rc == DC_STATUS_SUCCESS) {
				cylinder_t *cyl = dive->cylinder + i;

				cyl->type.size.mliter = lrint(tank.volume * 1000);
				cyl->type.workingpressure.mbar = lrint(tank.workpressure * 1000);

				cyl->cylinder_use = OC_GAS;
				if (tank.type & DC_TANKINFO_CC_O2)
					cyl->cylinder_use = OXYGEN;
				if (tank.type & DC_TANKINFO_CC_DILUENT)
					cyl->cylinder_use = DILUENT;

				if (tank.type & DC_TANKINFO_IMPERIAL) {
					if (same_string(devdata->model, "Suunto EON Steel")) {
						/* Suunto EON Steele gets this wrong. Badly.
						 * but on the plus side it only supports a few imperial sizes,
						 * so let's try and guess at least the most common ones.
						 * First, the pressures are off by a constant factor. WTF?
						 * Then we can round the wet sizes so we get to multiples of 10
						 * for cuft sizes (as that's all that you can enter) */
						dive->cylinder[i].type.workingpressure.mbar = lrint(
							dive->cylinder[i].type.workingpressure.mbar * 206.843 / 206.7 );
						char name_buffer[17];
						int rounded_size = lrint(ml_to_cuft(gas_volume(&dive->cylinder[i],
							dive->cylinder[i].type.workingpressure)));
						rounded_size = (int)((rounded_size + 5) / 10) * 10;
						switch (dive->cylinder[i].type.workingpressure.mbar) {
						case 206843:
							snprintf(name_buffer, sizeof(name_buffer), "AL%d", rounded_size);
							break;
						case 234422: /* this is wrong - HP tanks tend to be 3440, but Suunto only allows 3400 */
							snprintf(name_buffer, sizeof(name_buffer), "HP%d", rounded_size);
							break;
						case 179263:
							snprintf(name_buffer, sizeof(name_buffer), "LP+%d", rounded_size);
							break;
						case 165474:
							snprintf(name_buffer, sizeof(name_buffer), "LP%d", rounded_size);
							break;
						default:
							snprintf(name_buffer, sizeof(name_buffer), "%d cuft", rounded_size);
							break;
						}
						dive->cylinder[i].type.description = copy_string(name_buffer);
						dive->cylinder[i].type.size.mliter = lrint(cuft_to_l(rounded_size) * 1000 /
											mbar_to_atm(dive->cylinder[i].type.workingpressure.mbar));
					}
				}
				if (tank.gasmix != i) { // we don't handle this, yet
					shown_warning = true;
					report_error("gasmix %d for tank %d doesn't match", tank.gasmix, i);
				}
			}
			if (!IS_FP_SAME(tank.volume, 0.0))
				no_volume = false;

			// this new API also gives us the beginning and end pressure for the tank
			// normally 0 is not a valid pressure, but for some Uwatec dive computers we
			// don't get the actual start and end pressure, but instead a start pressure
			// that matches the consumption and an end pressure of always 0
			// In order to make this work, we arbitrary shift this up by 30bar so the
			// rest of the code treats this as if they were valid values
			if (!IS_FP_SAME(tank.beginpressure, 0.0)) {
				if (!IS_FP_SAME(tank.endpressure, 0.0)) {
					dive->cylinder[i].start.mbar = lrint(tank.beginpressure * 1000);
					dive->cylinder[i].end.mbar = lrint(tank.endpressure * 1000);
				} else if (same_string(devdata->vendor, "Uwatec")) {
					dive->cylinder[i].start.mbar = lrint(tank.beginpressure * 1000 + 30000);
					dive->cylinder[i].end.mbar = 30000;
				}
			}
		}
#endif
		if (no_volume) {
			/* for the first tank, if there is no tanksize available from the
			 * dive computer, fill in the default tank information (if set) */
			fill_default_cylinder(&dive->cylinder[i]);
		}
		/* whatever happens, make sure there is a name for the cylinder */
		if (empty_string(dive->cylinder[i].type.description))
			dive->cylinder[i].type.description = strdup(translate("gettextFromC", "unknown"));
	}
	return DC_STATUS_SUCCESS;
}

static void handle_event(struct divecomputer *dc, struct sample *sample, dc_sample_value_t value)
{
	int type, time;
	struct event *ev;

	/* we mark these for translation here, but we store the untranslated strings
	 * and only translate them when they are displayed on screen */
	static const char *events[] = {
		[SAMPLE_EVENT_NONE]			= QT_TRANSLATE_NOOP("gettextFromC", "none"),
		[SAMPLE_EVENT_DECOSTOP]			= QT_TRANSLATE_NOOP("gettextFromC", "deco stop"),
		[SAMPLE_EVENT_RBT]			= QT_TRANSLATE_NOOP("gettextFromC", "rbt"),
		[SAMPLE_EVENT_ASCENT]			= QT_TRANSLATE_NOOP("gettextFromC", "ascent"),
		[SAMPLE_EVENT_CEILING]			= QT_TRANSLATE_NOOP("gettextFromC", "ceiling"),
		[SAMPLE_EVENT_WORKLOAD]			= QT_TRANSLATE_NOOP("gettextFromC", "workload"),
		[SAMPLE_EVENT_TRANSMITTER]		= QT_TRANSLATE_NOOP("gettextFromC", "transmitter"),
		[SAMPLE_EVENT_VIOLATION]		= QT_TRANSLATE_NOOP("gettextFromC", "violation"),
		[SAMPLE_EVENT_BOOKMARK]			= QT_TRANSLATE_NOOP("gettextFromC", "bookmark"),
		[SAMPLE_EVENT_SURFACE]			= QT_TRANSLATE_NOOP("gettextFromC", "surface"),
		[SAMPLE_EVENT_SAFETYSTOP]		= QT_TRANSLATE_NOOP("gettextFromC", "safety stop"),
		[SAMPLE_EVENT_GASCHANGE]		= QT_TRANSLATE_NOOP("gettextFromC", "gaschange"),
		[SAMPLE_EVENT_SAFETYSTOP_VOLUNTARY]	= QT_TRANSLATE_NOOP("gettextFromC", "safety stop (voluntary)"),
		[SAMPLE_EVENT_SAFETYSTOP_MANDATORY]	= QT_TRANSLATE_NOOP("gettextFromC", "safety stop (mandatory)"),
		[SAMPLE_EVENT_DEEPSTOP]			= QT_TRANSLATE_NOOP("gettextFromC", "deepstop"),
		[SAMPLE_EVENT_CEILING_SAFETYSTOP]	= QT_TRANSLATE_NOOP("gettextFromC", "ceiling (safety stop)"),
		[SAMPLE_EVENT_FLOOR]			= QT_TRANSLATE_NOOP3("gettextFromC", "below floor", "event showing dive is below deco floor and adding deco time"),
		[SAMPLE_EVENT_DIVETIME]			= QT_TRANSLATE_NOOP("gettextFromC", "divetime"),
		[SAMPLE_EVENT_MAXDEPTH]			= QT_TRANSLATE_NOOP("gettextFromC", "maxdepth"),
		[SAMPLE_EVENT_OLF]			= QT_TRANSLATE_NOOP("gettextFromC", "OLF"),
		[SAMPLE_EVENT_PO2]			= QT_TRANSLATE_NOOP("gettextFromC", "pO₂"),
		[SAMPLE_EVENT_AIRTIME]			= QT_TRANSLATE_NOOP("gettextFromC", "airtime"),
		[SAMPLE_EVENT_RGBM]			= QT_TRANSLATE_NOOP("gettextFromC", "rgbm"),
		[SAMPLE_EVENT_HEADING]			= QT_TRANSLATE_NOOP("gettextFromC", "heading"),
		[SAMPLE_EVENT_TISSUELEVEL]		= QT_TRANSLATE_NOOP("gettextFromC", "tissue level warning"),
		[SAMPLE_EVENT_GASCHANGE2]		= QT_TRANSLATE_NOOP("gettextFromC", "gaschange"),
	};
	const int nr_events = sizeof(events) / sizeof(const char *);
	const char *name;

	/*
	 * Other evens might be more interesting, but for now we just print them out.
	 */
	type = value.event.type;
	name = QT_TRANSLATE_NOOP("gettextFromC", "invalid event number");
	if (type < nr_events && events[type])
		name = events[type];
#ifdef SAMPLE_EVENT_STRING
	if (type == SAMPLE_EVENT_STRING)
		name = value.event.name;
#endif

	time = value.event.time;
	if (sample)
		time += sample->time.seconds;

	ev = add_event(dc, time, type, value.event.flags, value.event.value, name);
	if (event_is_gaschange(ev) && ev->gas.index >= 0)
		current_gas_index = ev->gas.index;
}

static void handle_gasmix(struct divecomputer *dc, struct sample *sample, int idx)
{
	if (idx < 0 || idx >= MAX_CYLINDERS)
		return;
	add_event(dc, sample->time.seconds, SAMPLE_EVENT_GASCHANGE2, idx+1, 0, "gaschange");
	current_gas_index = idx;
}

void
sample_cb(dc_sample_type_t type, dc_sample_value_t value, void *userdata)
{
	static unsigned int nsensor = 0;
	struct divecomputer *dc = userdata;
	struct sample *sample;

	/*
	 * We fill in the "previous" sample - except for DC_SAMPLE_TIME,
	 * which creates a new one.
	 */
	sample = dc->samples ? dc->sample + dc->samples - 1 : NULL;

	/*
	 * Ok, sanity check.
	 * If first sample is not a DC_SAMPLE_TIME, Allocate a sample for us
	 */
	if (sample == NULL && type != DC_SAMPLE_TIME)
		sample = prepare_sample(dc);

	switch (type) {
	case DC_SAMPLE_TIME:
		nsensor = 0;

		// Create a new sample.
		// Mark depth as negative
		sample = prepare_sample(dc);
		sample->time.seconds = value.time;
		sample->depth.mm = -1;
		// The current sample gets some sticky values
		// that may have been around from before, these
		// values will be overwritten by new data if available
		sample->in_deco = in_deco;
		sample->ndl.seconds = ndl;
		sample->stoptime.seconds = stoptime;
		sample->stopdepth.mm = stopdepth;
		sample->setpoint.mbar = po2;
		sample->cns = cns;
		sample->heartbeat = heartbeat;
		sample->bearing.degrees = bearing;
		finish_sample(dc);
		break;
	case DC_SAMPLE_DEPTH:
		sample->depth.mm = lrint(value.depth * 1000);
		break;
	case DC_SAMPLE_PRESSURE:
		add_sample_pressure(sample, value.pressure.tank, lrint(value.pressure.value * 1000));
		break;
	case DC_SAMPLE_GASMIX:
		handle_gasmix(dc, sample, value.gasmix);
		break;
	case DC_SAMPLE_TEMPERATURE:
		sample->temperature.mkelvin = C_to_mkelvin(value.temperature);
		break;
	case DC_SAMPLE_EVENT:
		handle_event(dc, sample, value);
		break;
	case DC_SAMPLE_RBT:
		sample->rbt.seconds = (!strncasecmp(dc->model, "suunto", 6)) ? value.rbt : value.rbt * 60;
		break;
	case DC_SAMPLE_HEARTBEAT:
		sample->heartbeat = heartbeat = value.heartbeat;
		break;
	case DC_SAMPLE_BEARING:
		sample->bearing.degrees = bearing = value.bearing;
		break;
#ifdef DEBUG_DC_VENDOR
	case DC_SAMPLE_VENDOR:
		printf("   <vendor time='%u:%02u' type=\"%u\" size=\"%u\">", FRACTION(sample->time.seconds, 60),
		       value.vendor.type, value.vendor.size);
		for (int i = 0; i < value.vendor.size; ++i)
			printf("%02X", ((unsigned char *)value.vendor.data)[i]);
		printf("</vendor>\n");
		break;
#endif
#if DC_VERSION_CHECK(0, 3, 0)
	case DC_SAMPLE_SETPOINT:
		/* for us a setpoint means constant pO2 from here */
		sample->setpoint.mbar = po2 = lrint(value.setpoint * 1000);
		break;
	case DC_SAMPLE_PPO2:
		if (nsensor < 3)
			sample->o2sensor[nsensor].mbar = lrint(value.ppo2 * 1000);
		else
			report_error("%d is more o2 sensors than we can handle", nsensor);
		nsensor++;
		// Set the amount of detected o2 sensors
		if (nsensor > dc->no_o2sensors)
			dc->no_o2sensors = nsensor;
		break;
	case DC_SAMPLE_CNS:
		sample->cns = cns = lrint(value.cns * 100);
		break;
	case DC_SAMPLE_DECO:
		if (value.deco.type == DC_DECO_NDL) {
			sample->ndl.seconds = ndl = value.deco.time;
			sample->stopdepth.mm = stopdepth = lrint(value.deco.depth * 1000.0);
			sample->in_deco = in_deco = false;
		} else if (value.deco.type == DC_DECO_DECOSTOP ||
			   value.deco.type == DC_DECO_DEEPSTOP) {
			sample->in_deco = in_deco = true;
			sample->stopdepth.mm = stopdepth = lrint(value.deco.depth * 1000.0);
			sample->stoptime.seconds = stoptime = value.deco.time;
			ndl = 0;
		} else if (value.deco.type == DC_DECO_SAFETYSTOP) {
			sample->in_deco = in_deco = false;
			sample->stopdepth.mm = stopdepth = lrint(value.deco.depth * 1000.0);
			sample->stoptime.seconds = stoptime = value.deco.time;
		}
#endif
	default:
		break;
	}
}

static void dev_info(device_data_t *devdata, const char *fmt, ...)
{
	(void) devdata;
	static char buffer[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);
	progress_bar_text = buffer;
	if (progress_callback)
		(*progress_callback)(buffer);
}

static int import_dive_number = 0;

static int parse_samples(device_data_t *devdata, struct divecomputer *dc, dc_parser_t *parser)
{
	(void) devdata;
	// Parse the sample data.
	return dc_parser_samples_foreach(parser, sample_cb, dc);
}

static int might_be_same_dc(struct divecomputer *a, struct divecomputer *b)
{
	if (!a->model || !b->model)
		return 1;
	if (strcasecmp(a->model, b->model))
		return 0;
	if (!a->deviceid || !b->deviceid)
		return 1;
	return a->deviceid == b->deviceid;
}

static int match_one_dive(struct divecomputer *a, struct dive *dive)
{
	struct divecomputer *b = &dive->dc;

	/*
	 * Walk the existing dive computer data,
	 * see if we have a match (or an anti-match:
	 * the same dive computer but a different
	 * dive ID).
	 */
	do {
		int match = match_one_dc(a, b);
		if (match)
			return match > 0;
		b = b->next;
	} while (b);

	/* Ok, no exact dive computer match. Does the date match? */
	b = &dive->dc;
	do {
		if (a->when == b->when && might_be_same_dc(a, b))
			return 1;
		b = b->next;
	} while (b);

	return 0;
}

/*
 * Check if this dive already existed before the import
 */
static int find_dive(struct divecomputer *match)
{
	int i;

	for (i = dive_table.preexisting - 1; i >= 0; i--) {
		struct dive *old = dive_table.dives[i];

		if (match_one_dive(match, old))
			return 1;
	}
	return 0;
}

/*
 * Like g_strdup_printf(), but without the stupid g_malloc/g_free confusion.
 * And we limit the string to some arbitrary size.
 */
static char *str_printf(const char *fmt, ...)
{
	va_list args;
	char buf[1024];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	va_end(args);
	buf[sizeof(buf) - 1] = 0;
	return strdup(buf);
}

/*
 * The dive ID for libdivecomputer dives is the first word of the
 * SHA1 of the fingerprint, if it exists.
 *
 * NOTE! This is byte-order dependent, and I don't care.
 */
static uint32_t calculate_diveid(const unsigned char *fingerprint, unsigned int fsize)
{
	uint32_t csum[5];

	if (!fingerprint || !fsize)
		return 0;

	SHA1(fingerprint, fsize, (unsigned char *)csum);
	return csum[0];
}

#ifdef DC_FIELD_STRING
static uint32_t calculate_string_hash(const char *str)
{
	return calculate_diveid((const unsigned char *)str, strlen(str));
}

/*
 * Find an existing device ID for this device model and serial number
 */
static void dc_match_serial(void *_dc, const char *model, uint32_t deviceid, const char *nickname, const char *serial, const char *firmware)
{
	(void)nickname;
	(void)firmware;

	struct divecomputer *dc = _dc;

	if (!deviceid)
		return;
	if (!dc->model || !model || strcasecmp(dc->model, model))
		return;
	if (!dc->serial || !serial || strcasecmp(dc->serial, serial))
		return;
	dc->deviceid = deviceid;
}

/*
 * Set the serial number.
 *
 * This also sets the device ID by looking for existing devices that
 * have that serial number.
 *
 * If no existing device ID exists, create a new by hashing the serial
 * number string.
 */
static void set_dc_serial(struct divecomputer *dc, const char *serial)
{
	dc->serial = serial;
	call_for_each_dc(dc, dc_match_serial, false);
	if (!dc->deviceid)
		dc->deviceid = calculate_string_hash(serial);
}

static void parse_string_field(struct dive *dive, dc_field_string_t *str)
{
	// Our dive ID is the string hash of the "Dive ID" string
	if (!strcmp(str->desc, "Dive ID")) {
		if (!dive->dc.diveid)
			dive->dc.diveid = calculate_string_hash(str->value);
		return;
	}
	add_extra_data(&dive->dc, str->desc, str->value);
	if (!strcmp(str->desc, "Serial")) {
		set_dc_serial(&dive->dc, str->value);
		return;
	}
	if (!strcmp(str->desc, "FW Version")) {
		dive->dc.fw_version = strdup(str->value);
		return;
	}
}
#endif

static dc_status_t libdc_header_parser(dc_parser_t *parser, device_data_t *devdata, struct dive *dive)
{
	dc_status_t rc = 0;
	dc_datetime_t dt = { 0 };
	struct tm tm;

	rc = dc_parser_get_datetime(parser, &dt);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error parsing the datetime"));
		return rc;
	}

	dive->dc.deviceid = devdata->deviceid;

	if (rc == DC_STATUS_SUCCESS) {
		tm.tm_year = dt.year;
		tm.tm_mon = dt.month - 1;
		tm.tm_mday = dt.day;
		tm.tm_hour = dt.hour;
		tm.tm_min = dt.minute;
		tm.tm_sec = dt.second;
		dive->when = dive->dc.when = utc_mktime(&tm);
	}

	// Parse the divetime.
	char *date_string = get_dive_date_c_string(dive->when);
	dev_info(devdata, translate("gettextFromC", "Dive %d: %s"), import_dive_number, date_string);
	free(date_string);

	unsigned int divetime = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_DIVETIME, 0, &divetime);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error parsing the divetime"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dc.duration.seconds = divetime;

	// Parse the maxdepth.
	double maxdepth = 0.0;
	rc = dc_parser_get_field(parser, DC_FIELD_MAXDEPTH, 0, &maxdepth);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error parsing the maxdepth"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dc.maxdepth.mm = lrint(maxdepth * 1000);

#if DC_VERSION_CHECK(0, 5, 0) && defined(DC_GASMIX_UNKNOWN)
	// if this is defined then we have a fairly late version of libdivecomputer
	// from the 0.5 development cylcle - most likely temperatures and tank sizes
	// are supported

	// Parse temperatures
	double temperature;
	dc_field_type_t temp_fields[] = {DC_FIELD_TEMPERATURE_SURFACE,
					 DC_FIELD_TEMPERATURE_MAXIMUM,
					 DC_FIELD_TEMPERATURE_MINIMUM};
	for (int i = 0; i < 3; i++) {
		rc = dc_parser_get_field(parser, temp_fields[i], 0, &temperature);
		if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
			dev_info(devdata, translate("gettextFromC", "Error parsing temperature"));
			return rc;
		}
		if (rc == DC_STATUS_SUCCESS)
			switch(i) {
			case 0:
				dive->dc.airtemp.mkelvin = C_to_mkelvin(temperature);
				break;
			case 1: // we don't distinguish min and max water temp here, so take min if given, max otherwise
			case 2:
				dive->dc.watertemp.mkelvin = C_to_mkelvin(temperature);
				break;
			}
	}
#endif

	// Parse the gas mixes.
	unsigned int ngases = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_GASMIX_COUNT, 0, &ngases);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error parsing the gas mix count"));
		return rc;
	}

#if DC_VERSION_CHECK(0, 3, 0)
	// Check if the libdivecomputer version already supports salinity & atmospheric
	dc_salinity_t salinity = {
		.type = DC_WATER_SALT,
		.density = SEAWATER_SALINITY / 10.0
	};
	rc = dc_parser_get_field(parser, DC_FIELD_SALINITY, 0, &salinity);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error obtaining water salinity"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dc.salinity = lrint(salinity.density * 10.0);

	double surface_pressure = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_ATMOSPHERIC, 0, &surface_pressure);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error obtaining surface pressure"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dc.surface_pressure.mbar = lrint(surface_pressure * 1000.0);
#endif

#ifdef DC_FIELD_STRING
	// The dive parsing may give us more device information
	int idx;
	for (idx = 0; idx < 100; idx++) {
		dc_field_string_t str = { NULL };
		rc = dc_parser_get_field(parser, DC_FIELD_STRING, idx, &str);
		if (rc != DC_STATUS_SUCCESS)
			break;
		if (!str.desc || !str.value)
			break;
		parse_string_field(dive, &str);
	}
#endif

#if DC_VERSION_CHECK(0, 5, 0) && defined(DC_GASMIX_UNKNOWN)
	dc_divemode_t divemode;
	rc = dc_parser_get_field(parser, DC_FIELD_DIVEMODE, 0, &divemode);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error obtaining dive mode"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		switch(divemode) {
		case DC_DIVEMODE_FREEDIVE:
			dive->dc.divemode = FREEDIVE;
			break;
		case DC_DIVEMODE_GAUGE:
		case DC_DIVEMODE_OC: /* Open circuit */
			dive->dc.divemode = OC;
			break;
		case DC_DIVEMODE_CCR:  /* Closed circuit rebreather*/
			dive->dc.divemode = CCR;
			break;
		case DC_DIVEMODE_SCR:  /* Semi-closed circuit rebreather */
			dive->dc.divemode = PSCR;
			break;
		}
#endif

	rc = parse_gasmixes(devdata, dive, parser, ngases);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error parsing the gas mix"));
		return rc;
	}

	return DC_STATUS_SUCCESS;
}

/* returns true if we want libdivecomputer's dc_device_foreach() to continue,
 *  false otherwise */
static int dive_cb(const unsigned char *data, unsigned int size,
		   const unsigned char *fingerprint, unsigned int fsize,
		   void *userdata)
{
	int rc;
	dc_parser_t *parser = NULL;
	device_data_t *devdata = userdata;
	struct dive *dive = NULL;

	/* reset static data, that is only valid per dive */
	stoptime = stopdepth = po2 = cns = heartbeat = 0;
	ndl = bearing = -1;
	in_deco = false;
	current_gas_index = -1;

	rc = create_parser(devdata, &parser);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, translate("gettextFromC", "Unable to create parser for %s %s"), devdata->vendor, devdata->product);
		return false;
	}

	rc = dc_parser_set_data(parser, data, size);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, translate("gettextFromC", "Error registering the data"));
		goto error_exit;
	}

	import_dive_number++;
	dive = alloc_dive();

	// Fill in basic fields
	dive->dc.model = strdup(devdata->model);
	dive->dc.diveid = calculate_diveid(fingerprint, fsize);

	// Parse the dive's header data
	rc = libdc_header_parser (parser, devdata, dive);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, translate("getextFromC", "Error parsing the header"));
		goto error_exit;
	}

	// Initialize the sample data.
	rc = parse_samples(devdata, &dive->dc, parser);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, translate("gettextFromC", "Error parsing the samples"));
		goto error_exit;
	}

	/* If we already saw this dive, abort. */
	if (!devdata->force_download && find_dive(&dive->dc)) {
		char *date_string = get_dive_date_c_string(dive->when);
		dev_info(devdata, translate("gettextFromC", "Already downloaded dive at %s"), date_string);
		free(date_string);
		goto error_exit;
	}

	dc_parser_destroy(parser);

	/* Various libdivecomputer interface fixups */
	if (dive->dc.airtemp.mkelvin == 0 && first_temp_is_air && dive->dc.samples) {
		dive->dc.airtemp = dive->dc.sample[0].temperature;
		dive->dc.sample[0].temperature.mkelvin = 0;
	}

	if (devdata->create_new_trip) {
		if (!devdata->trip)
			devdata->trip = create_and_hookup_trip_from_dive(dive);
		else
			add_dive_to_trip(dive, devdata->trip);
	}

	dive->downloaded = true;
	record_dive_to_table(dive, devdata->download_table);
	mark_divelist_changed(true);
	return true;

error_exit:
	dc_parser_destroy(parser);
	free(dive);
	return false;

}

/*
 * The device ID for libdivecomputer devices is the first 32-bit word
 * of the SHA1 hash of the model/firmware/serial numbers.
 *
 * NOTE! This is byte-order-dependent. And I can't find it in myself to
 * care.
 */
static uint32_t calculate_sha1(unsigned int model, unsigned int firmware, unsigned int serial)
{
	SHA_CTX ctx;
	uint32_t csum[5];

	SHA1_Init(&ctx);
	SHA1_Update(&ctx, &model, sizeof(model));
	SHA1_Update(&ctx, &firmware, sizeof(firmware));
	SHA1_Update(&ctx, &serial, sizeof(serial));
	SHA1_Final((unsigned char *)csum, &ctx);
	return csum[0];
}

/*
 * libdivecomputer has returned two different serial numbers for the
 * same device in different versions. First it used to just do the four
 * bytes as one 32-bit number, then it turned it into a decimal number
 * with each byte giving two digits (0-99).
 *
 * The only way we can tell is by looking at the format of the number,
 * so we'll just fix it to the first format.
 */
static unsigned int undo_libdivecomputer_suunto_nr_changes(unsigned int serial)
{
	unsigned char b0, b1, b2, b3;

	/*
	 * The second format will never have more than 8 decimal
	 * digits, so do a cheap check first
	 */
	if (serial >= 100000000)
		return serial;

	/* The original format seems to be four bytes of values 00-99 */
	b0 = (serial >> 0) & 0xff;
	b1 = (serial >> 8) & 0xff;
	b2 = (serial >> 16) & 0xff;
	b3 = (serial >> 24) & 0xff;

	/* Looks like an old-style libdivecomputer serial number */
	if ((b0 < 100) && (b1 < 100) && (b2 < 100) && (b3 < 100))
		return serial;

	/* Nope, it was converted. */
	b0 = serial % 100;
	serial /= 100;
	b1 = serial % 100;
	serial /= 100;
	b2 = serial % 100;
	serial /= 100;
	b3 = serial % 100;

	serial = b0 + (b1 << 8) + (b2 << 16) + (b3 << 24);
	return serial;
}

static unsigned int fixup_suunto_versions(device_data_t *devdata, const dc_event_devinfo_t *devinfo)
{
	unsigned int serial = devinfo->serial;
	char serial_nr[13] = "";
	char firmware[13] = "";

	first_temp_is_air = 1;

	serial = undo_libdivecomputer_suunto_nr_changes(serial);

	if (serial) {
		snprintf(serial_nr, sizeof(serial_nr), "%02d%02d%02d%02d",
			 (devinfo->serial >> 24) & 0xff,
			 (devinfo->serial >> 16) & 0xff,
			 (devinfo->serial >> 8) & 0xff,
			 (devinfo->serial >> 0) & 0xff);
	}
	if (devinfo->firmware) {
		snprintf(firmware, sizeof(firmware), "%d.%d.%d",
			 (devinfo->firmware >> 16) & 0xff,
			 (devinfo->firmware >> 8) & 0xff,
			 (devinfo->firmware >> 0) & 0xff);
	}
	create_device_node(devdata->model, devdata->deviceid, serial_nr, firmware, "");

	return serial;
}

static void event_cb(dc_device_t *device, dc_event_type_t event, const void *data, void *userdata)
{
	(void) device;
	const dc_event_progress_t *progress = data;
	const dc_event_devinfo_t *devinfo = data;
	const dc_event_clock_t *clock = data;
	const dc_event_vendor_t *vendor = data;
	device_data_t *devdata = userdata;
	unsigned int serial;

	switch (event) {
	case DC_EVENT_WAITING:
		dev_info(devdata, translate("gettextFromC", "Event: waiting for user action"));
		break;
	case DC_EVENT_PROGRESS:
		if (!progress->maximum)
			break;
		progress_bar_fraction = (double)progress->current / (double)progress->maximum;
		break;
	case DC_EVENT_DEVINFO:
		if (dc_descriptor_get_model(devdata->descriptor) != devinfo->model) {
			fprintf(stderr, "EVENT_DEVINFO gave us the correct detected product (model %d instead of %d)\n",
				devinfo->model, dc_descriptor_get_model(devdata->descriptor));
			dc_descriptor_t *better_descriptor = get_descriptor(dc_descriptor_get_type(devdata->descriptor), devinfo->model);
			if (better_descriptor != NULL) {
				devdata->descriptor = better_descriptor;
				devdata->product = dc_descriptor_get_product(better_descriptor);
				devdata->vendor = dc_descriptor_get_vendor(better_descriptor);
				devdata->model = str_printf("%s %s", devdata->vendor, devdata->product);
			}
		}
		dev_info(devdata, translate("gettextFromC", "model=%s firmware=%u serial=%u"),
			 devdata->product, devinfo->firmware, devinfo->serial);
		if (devdata->libdc_logfile) {
			fprintf(devdata->libdc_logfile, "Event: model=%u (0x%08x), firmware=%u (0x%08x), serial=%u (0x%08x)\n",
				devinfo->model, devinfo->model,
				devinfo->firmware, devinfo->firmware,
				devinfo->serial, devinfo->serial);
		}
		/*
		 * libdivecomputer doesn't give serial numbers in the proper string form,
		 * so we have to see if we can do some vendor-specific munging.
		 */
		serial = devinfo->serial;
		if (!strcmp(devdata->vendor, "Suunto"))
			serial = fixup_suunto_versions(devdata, devinfo);
		devdata->deviceid = calculate_sha1(devinfo->model, devinfo->firmware, serial);
		/* really, serial and firmware version are NOT numbers. We'll try to save them here
		 * in something that might work, but this really needs to be handled with the
		 * DC_FIELD_STRING interface instead */
		devdata->libdc_serial = devinfo->serial;
		devdata->libdc_firmware = devinfo->firmware;
		break;
	case DC_EVENT_CLOCK:
		dev_info(devdata, translate("gettextFromC", "Event: systime=%" PRId64 ", devtime=%u\n"),
			 (uint64_t)clock->systime, clock->devtime);
		if (devdata->libdc_logfile) {
			fprintf(devdata->libdc_logfile, "Event: systime=%" PRId64 ", devtime=%u\n",
				(uint64_t)clock->systime, clock->devtime);
		}
		break;
	case DC_EVENT_VENDOR:
		if (devdata->libdc_logfile) {
			fprintf(devdata->libdc_logfile, "Event: vendor=");
			for (unsigned int i = 0; i < vendor->size; ++i)
				fprintf(devdata->libdc_logfile, "%02X", vendor->data[i]);
			fprintf(devdata->libdc_logfile, "\n");
		}
		break;
	default:
		break;
	}
}

int import_thread_cancelled;

static int cancel_cb(void *userdata)
{
	(void) userdata;
	return import_thread_cancelled;
}

static const char *do_device_import(device_data_t *data)
{
	dc_status_t rc;
	dc_device_t *device = data->device;

	data->model = str_printf("%s %s", data->vendor, data->product);

	// Register the event handler.
	int events = DC_EVENT_WAITING | DC_EVENT_PROGRESS | DC_EVENT_DEVINFO | DC_EVENT_CLOCK | DC_EVENT_VENDOR;
	rc = dc_device_set_events(device, events, event_cb, data);
	if (rc != DC_STATUS_SUCCESS)
		return translate("gettextFromC", "Error registering the event handler.");

	// Register the cancellation handler.
	rc = dc_device_set_cancel(device, cancel_cb, data);
	if (rc != DC_STATUS_SUCCESS)
		return translate("gettextFromC", "Error registering the cancellation handler.");

	if (data->libdc_dump) {
		dc_buffer_t *buffer = dc_buffer_new(0);

		rc = dc_device_dump(device, buffer);
		if (rc == DC_STATUS_SUCCESS && dumpfile_name) {
			FILE *fp = subsurface_fopen(dumpfile_name, "wb");
			if (fp != NULL) {
				fwrite(dc_buffer_get_data(buffer), 1, dc_buffer_get_size(buffer), fp);
				fclose(fp);
			}
		}

		dc_buffer_free(buffer);
	} else {
		rc = dc_device_foreach(device, dive_cb, data);
	}

	if (rc != DC_STATUS_SUCCESS) {
		progress_bar_fraction = 0.0;
		return translate("gettextFromC", "Dive data import error");
	}

	/* All good */
	return NULL;
}

void logfunc(dc_context_t *context, dc_loglevel_t loglevel, const char *file, unsigned int line, const char *function, const char *msg, void *userdata)
{
	(void) context;
	const char *loglevels[] = { "NONE", "ERROR", "WARNING", "INFO", "DEBUG", "ALL" };

	FILE *fp = (FILE *)userdata;

	if (loglevel == DC_LOGLEVEL_ERROR || loglevel == DC_LOGLEVEL_WARNING) {
		fprintf(fp, "%s: %s [in %s:%d (%s)]\n", loglevels[loglevel], msg, file, line, function);
	} else {
		fprintf(fp, "%s: %s\n", loglevels[loglevel], msg);
	}
}

dc_status_t divecomputer_device_open(device_data_t *data)
{
	dc_status_t rc;
	dc_descriptor_t *descriptor = data->descriptor;
	dc_context_t *context = data->context;
	unsigned int transports;
	transports = dc_descriptor_get_transports(descriptor);

#ifdef BLE_SUPPORT
	if (data->bluetooth_mode && (transports & DC_TRANSPORT_BLE)) {
		rc = ble_packet_open(&data->iostream, context, data->devname, data);
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}
#endif

#ifdef BT_SUPPORT
	if (data->bluetooth_mode && (transports & DC_TRANSPORT_BLUETOOTH)) {
		rc = rfcomm_stream_open(&data->iostream, context, data->devname);
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}
#endif

	if (transports & DC_TRANSPORT_USBHID) {
		// Discover the usbhid device.
		dc_iterator_t *iterator = NULL;
		dc_usbhid_device_t *device = NULL;
		dc_usbhid_iterator_new (&iterator, context, descriptor);
		while (dc_iterator_next (iterator, &device) == DC_STATUS_SUCCESS)
			break;
		dc_iterator_free (iterator);

		if (device) {
			rc = dc_usbhid_open(&data->iostream, context, device);
			dc_usbhid_device_free(device);
			if (rc == DC_STATUS_SUCCESS)
				return rc;
		}
	}

	/* The dive computer backend does this all internally */
	if (transports & DC_TRANSPORT_USB)
		return DC_STATUS_SUCCESS;

	if (transports & DC_TRANSPORT_SERIAL) {
		rc = dc_serial_open(&data->iostream, context, data->devname);
		if (rc == DC_STATUS_SUCCESS)
			return rc;

#ifdef SERIAL_FTDI
		if (!strcmp(data->devname, "ftdi")) {
			rc = ftdi_open(&data->iostream, context);
			if (rc == DC_STATUS_SUCCESS)
				return rc;
		}
#endif
	}

	if (transports & DC_TRANSPORT_IRDA) {
		unsigned int address = 0;

		dc_iterator_t *iterator = NULL;
		dc_irda_device_t *device = NULL;
		dc_irda_iterator_new (&iterator, context, descriptor);
		while (dc_iterator_next (iterator, &device) == DC_STATUS_SUCCESS) {
			address = dc_irda_device_get_address (device);
			dc_irda_device_free (device);
			break;
		}
		dc_iterator_free (iterator);

		if (!address)
			address = strtoul(data->devname, NULL, 0);

		rc = dc_irda_open(&data->iostream, context, address, 1);
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}

	return DC_STATUS_UNSUPPORTED;
}

const char *do_libdivecomputer_import(device_data_t *data)
{
	dc_status_t rc;
	const char *err;
	FILE *fp = NULL;

	import_dive_number = 0;
	first_temp_is_air = 0;
	data->device = NULL;
	data->context = NULL;
	data->iostream = NULL;

	if (data->libdc_log && logfile_name)
		fp = subsurface_fopen(logfile_name, "w");

	data->libdc_logfile = fp;

	rc = dc_context_new(&data->context);
	if (rc != DC_STATUS_SUCCESS)
		return translate("gettextFromC", "Unable to create libdivecomputer context");

	if (fp) {
		dc_context_set_loglevel(data->context, DC_LOGLEVEL_ALL);
		dc_context_set_logfunc(data->context, logfunc, fp);
		fprintf(data->libdc_logfile, "Subsurface: v%s, ", subsurface_git_version());
		fprintf(data->libdc_logfile, "built with libdivecomputer v%s\n", dc_version(NULL));
	}

	err = translate("gettextFromC", "Unable to open %s %s (%s)");

	rc = divecomputer_device_open(data);

	if (rc != DC_STATUS_SUCCESS) {
		report_error(errmsg(rc));
	} else {
		rc = dc_device_open(&data->device, data->context, data->descriptor, data->iostream);
		INFO(0, "dc_device_open error value of %d", rc);
		if (rc != DC_STATUS_SUCCESS && subsurface_access(data->devname, R_OK | W_OK) != 0)
			err = translate("gettextFromC", "Error opening the device %s %s (%s).\nIn most cases, in order to debug this issue, a libdivecomputer logfile will be useful.\nYou can create this logfile by selecting the corresponding checkbox in the download dialog.");
	}

	if (rc == DC_STATUS_SUCCESS) {
		err = do_device_import(data);
		/* TODO: Show the logfile to the user on error. */
		dc_device_close(data->device);
		data->device = NULL;
		dc_iostream_close(data->iostream);
		data->iostream = NULL;
		if (!downloadTable.nr)
			dev_info(data, translate("gettextFromC", "No new dives downloaded from dive computer"));
	}

	dc_context_free(data->context);
	data->context = NULL;

	if (fp) {
		fclose(fp);
	}

	return err;
}

/*
 * Parse data buffers instead of dc devices downloaded data.
 * Intended to be used to parse profile data from binary files during import tasks.
 * Actually included Uwatec families because of works on datatrak and smartrak logs
 * and OSTC families for OSTCTools logs import.
 * For others, simply include them in the switch  (check parameters).
 * Note that dc_descriptor_t in data  *must* have been filled using dc_descriptor_iterator()
 * calls.
 */
dc_status_t libdc_buffer_parser(struct dive *dive, device_data_t *data, unsigned char *buffer, int size)
{
	dc_status_t rc;
	dc_parser_t *parser = NULL;

	switch (dc_descriptor_get_type(data->descriptor)) {
	case DC_FAMILY_UWATEC_ALADIN:
	case DC_FAMILY_UWATEC_MEMOMOUSE:
	case DC_FAMILY_UWATEC_SMART:
	case DC_FAMILY_UWATEC_MERIDIAN:
	case DC_FAMILY_HW_OSTC:
	case DC_FAMILY_HW_FROG:
	case DC_FAMILY_HW_OSTC3:
		rc = dc_parser_new2(&parser, data->context, data->descriptor, 0, 0);
		break;
	default:
		report_error("Device type not handled!");
		return DC_STATUS_UNSUPPORTED;
	}
	if  (rc != DC_STATUS_SUCCESS) {
		report_error("Error creating parser.");
		dc_parser_destroy (parser);
		return rc;
	}
	rc = dc_parser_set_data(parser, buffer, size);
	if (rc != DC_STATUS_SUCCESS) {
		report_error("Error registering the data.");
		dc_parser_destroy (parser);
		return rc;
	}
	// Do not parse Aladin/Memomouse headers as they are fakes
	// Do not return on error, we can still parse the samples
	if (dc_descriptor_get_type(data->descriptor) != DC_FAMILY_UWATEC_ALADIN && dc_descriptor_get_type(data->descriptor) != DC_FAMILY_UWATEC_MEMOMOUSE) {
		rc = libdc_header_parser (parser, data, dive);
		if (rc != DC_STATUS_SUCCESS) {
			report_error("Error parsing the dive header data. Dive # %d\nStatus = %s", dive->number, errmsg(rc));
		}
	}
	rc = dc_parser_samples_foreach (parser, sample_cb, &dive->dc);
	if (rc != DC_STATUS_SUCCESS) {
		report_error("Error parsing the sample data. Dive # %d\nStatus = %s", dive->number, errmsg(rc));
		dc_parser_destroy (parser);
		return rc;
	}
	dc_parser_destroy(parser);
	return DC_STATUS_SUCCESS;
}

/*
 * Returns a dc_descriptor_t structure based on dc model's number and family.
 *
 * That dc_descriptor_t needs to be freed with dc_descriptor_free by the reciver.
 */
dc_descriptor_t *get_descriptor(dc_family_t type, unsigned int model)
{
	dc_descriptor_t *descriptor = NULL, *needle = NULL;
	dc_iterator_t *iterator = NULL;
	dc_status_t rc;

	rc = dc_descriptor_iterator(&iterator);
	if (rc != DC_STATUS_SUCCESS) {
		fprintf(stderr, "Error creating the device descriptor iterator.\n");
		return NULL;
	}
	while ((dc_iterator_next(iterator, &descriptor)) == DC_STATUS_SUCCESS) {
		unsigned int desc_model = dc_descriptor_get_model(descriptor);
		dc_family_t desc_type = dc_descriptor_get_type(descriptor);
		if (model == desc_model && type == desc_type) {
			needle = descriptor;
			break;
		}
		dc_descriptor_free(descriptor);
	}
	dc_iterator_free(iterator);
	return needle;
}
