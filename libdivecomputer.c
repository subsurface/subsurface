#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include "gettext.h"
#include "dive.h"
#include "device.h"
#include "divelist.h"
#include "display.h"

#include "libdivecomputer.h"
#include <libdivecomputer/uwatec.h>
#include <libdivecomputer/hw.h>


/* Christ. Libdivecomputer has the worst configuration system ever. */
#ifdef HW_FROG_H
#define NOT_FROG , 0
#define LIBDIVECOMPUTER_SUPPORTS_FROG
#else
#define NOT_FROG
#endif

char *dumpfile_name;
char *logfile_name;
const char *progress_bar_text = "";
double progress_bar_fraction = 0.0;

static int stoptime, stopdepth, ndl, po2, cns;
static bool in_deco, first_temp_is_air;

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

static int parse_gasmixes(device_data_t *devdata, struct dive *dive, dc_parser_t *parser, int ngases)
{
	static bool shown_warning = false;
	int i, rc;

#if DC_VERSION_CHECK(0, 5, 0) && defined(DC_GASMIX_UNKNOWN)
	int ntanks = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_TANK_COUNT, 0, &ntanks);
	if (rc == DC_STATUS_SUCCESS) {
		if (ntanks != ngases) {
			shown_warning = true;
			report_error("different number of gases (%d) and tanks (%d)", ngases, ntanks);
		}
	}
	dc_tank_t tank = { 0 };
#endif

	for (i = 0; i < ngases; i++) {
		dc_gasmix_t gasmix = { 0 };
		int o2, he;
		bool no_volume = true;

		rc = dc_parser_get_field(parser, DC_FIELD_GASMIX, i, &gasmix);
		if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED)
			return rc;

		if (i >= MAX_CYLINDERS)
			continue;

		o2 = rint(gasmix.oxygen * 1000);
		he = rint(gasmix.helium * 1000);

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

#if DC_VERSION_CHECK(0, 5, 0) && defined(DC_GASMIX_UNKNOWN)
		tank.volume = 0.0;
		if (i < ntanks) {
			rc = dc_parser_get_field(parser, DC_FIELD_TANK, i, &tank);
			if (rc == DC_STATUS_SUCCESS) {
				if (tank.type == DC_TANKVOLUME_IMPERIAL) {
					dive->cylinder[i].type.size.mliter = rint(tank.volume * 1000);
					dive->cylinder[i].type.workingpressure.mbar = rint(tank.workpressure * 1000);
				} else if (tank.type == DC_TANKVOLUME_METRIC) {
					dive->cylinder[i].type.size.mliter = rint(tank.volume * 1000);
				}
				if (tank.gasmix != i) { // we don't handle this, yet
					shown_warning = true;
					report_error("gasmix %d for tank %d doesn't match", tank.gasmix, i);
				}
			}
		}
		if (!IS_FP_SAME(tank.volume, 0.0))
			no_volume = false;
#endif
		if (no_volume) {
			/* for the first tank, if there is no tanksize available from the
			 * dive computer, fill in the default tank information (if set) */
			fill_default_cylinder(&dive->cylinder[i]);
		}
		/* whatever happens, make sure there is a name for the cylinder */
		if (same_string(dive->cylinder[i].type.description, ""))
			dive->cylinder[i].type.description = strdup(translate("gettextFromC", "unknown"));
	}
	return DC_STATUS_SUCCESS;
}

static void handle_event(struct divecomputer *dc, struct sample *sample, dc_sample_value_t value)
{
	int type, time;
	/* we mark these for translation here, but we store the untranslated strings
	 * and only translate them when they are displayed on screen */
	static const char *events[] = {
		QT_TRANSLATE_NOOP("gettextFromC", "none"), QT_TRANSLATE_NOOP("gettextFromC", "deco stop"), QT_TRANSLATE_NOOP("gettextFromC", "rbt"), QT_TRANSLATE_NOOP("gettextFromC", "ascent"), QT_TRANSLATE_NOOP("gettextFromC", "ceiling"), QT_TRANSLATE_NOOP("gettextFromC", "workload"),
		QT_TRANSLATE_NOOP("gettextFromC", "transmitter"), QT_TRANSLATE_NOOP("gettextFromC", "violation"), QT_TRANSLATE_NOOP("gettextFromC", "bookmark"), QT_TRANSLATE_NOOP("gettextFromC", "surface"), QT_TRANSLATE_NOOP("gettextFromC", "safety stop"),
		QT_TRANSLATE_NOOP("gettextFromC", "gaschange"), QT_TRANSLATE_NOOP("gettextFromC", "safety stop (voluntary)"), QT_TRANSLATE_NOOP("gettextFromC", "safety stop (mandatory)"),
		QT_TRANSLATE_NOOP("gettextFromC", "deepstop"), QT_TRANSLATE_NOOP("gettextFromC", "ceiling (safety stop)"), QT_TRANSLATE_NOOP3("gettextFromC", "below floor", "event showing dive is below deco floor and adding deco time"), QT_TRANSLATE_NOOP("gettextFromC", "divetime"),
		QT_TRANSLATE_NOOP("gettextFromC", "maxdepth"), QT_TRANSLATE_NOOP("gettextFromC", "OLF"), QT_TRANSLATE_NOOP("gettextFromC", "pO₂"), QT_TRANSLATE_NOOP("gettextFromC", "airtime"), QT_TRANSLATE_NOOP("gettextFromC", "rgbm"), QT_TRANSLATE_NOOP("gettextFromC", "heading"),
		QT_TRANSLATE_NOOP("gettextFromC", "tissue level warning"), QT_TRANSLATE_NOOP("gettextFromC", "gaschange"), QT_TRANSLATE_NOOP("gettextFromC", "non stop time")
	};
	const int nr_events = sizeof(events) / sizeof(const char *);
	const char *name;
	/*
	 * Just ignore surface events.  They are pointless.  What "surface"
	 * means depends on the dive computer (and possibly even settings
	 * in the dive computer). It does *not* necessarily mean "depth 0",
	 * so don't even turn it into that.
	 */
	if (value.event.type == SAMPLE_EVENT_SURFACE)
		return;

	/*
	 * Other evens might be more interesting, but for now we just print them out.
	 */
	type = value.event.type;
	name = QT_TRANSLATE_NOOP("gettextFromC", "invalid event number");
	if (type < nr_events)
		name = events[type];

	time = value.event.time;
	if (sample)
		time += sample->time.seconds;

	add_event(dc, time, type, value.event.flags, value.event.value, name);
}

void
sample_cb(dc_sample_type_t type, dc_sample_value_t value, void *userdata)
{
	unsigned int mm;
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
		mm = 0;
		if (sample) {
			sample->in_deco = in_deco;
			sample->ndl.seconds = ndl;
			sample->stoptime.seconds = stoptime;
			sample->stopdepth.mm = stopdepth;
			sample->setpoint.mbar = po2;
			sample->cns = cns;
			mm = sample->depth.mm;
		}
		sample = prepare_sample(dc);
		sample->time.seconds = value.time;
		sample->depth.mm = mm;
		finish_sample(dc);
		break;
	case DC_SAMPLE_DEPTH:
		sample->depth.mm = rint(value.depth * 1000);
		break;
	case DC_SAMPLE_PRESSURE:
		sample->sensor = value.pressure.tank;
		sample->cylinderpressure.mbar = rint(value.pressure.value * 1000);
		break;
	case DC_SAMPLE_TEMPERATURE:
		sample->temperature.mkelvin = C_to_mkelvin(value.temperature);
		break;
	case DC_SAMPLE_EVENT:
		handle_event(dc, sample, value);
		break;
	case DC_SAMPLE_RBT:
		printf("   <rbt>%u</rbt>\n", value.rbt);
		break;
	case DC_SAMPLE_HEARTBEAT:
		sample->heartbeat = value.heartbeat;
		break;
	case DC_SAMPLE_BEARING:
		sample->bearing.degrees = value.bearing;
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
		sample->setpoint.mbar = po2 = rint(value.setpoint * 1000);
		break;
	case DC_SAMPLE_PPO2:
		sample->setpoint.mbar = po2 = rint(value.ppo2 * 1000);
		break;
	case DC_SAMPLE_CNS:
		sample->cns = cns = rint(value.cns * 100);
		break;
	case DC_SAMPLE_DECO:
		if (value.deco.type == DC_DECO_NDL) {
			sample->ndl.seconds = ndl = value.deco.time;
			sample->stopdepth.mm = stopdepth = rint(value.deco.depth * 1000.0);
			sample->in_deco = in_deco = false;
		} else if (value.deco.type == DC_DECO_DECOSTOP ||
			   value.deco.type == DC_DECO_DEEPSTOP) {
			sample->in_deco = in_deco = true;
			sample->stopdepth.mm = stopdepth = rint(value.deco.depth * 1000.0);
			sample->stoptime.seconds = stoptime = value.deco.time;
			ndl = 0;
		} else if (value.deco.type == DC_DECO_SAFETYSTOP) {
			sample->in_deco = in_deco = false;
			sample->stopdepth.mm = stopdepth = rint(value.deco.depth * 1000.0);
			sample->stoptime.seconds = stoptime = value.deco.time;
		}
#endif
	default:
		break;
	}
}

static void dev_info(device_data_t *devdata, const char *fmt, ...)
{
	static char buffer[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);
	progress_bar_text = buffer;
}

static int import_dive_number = 0;

static int parse_samples(device_data_t *devdata, struct divecomputer *dc, dc_parser_t *parser)
{
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

	for (i = 0; i < dive_table.preexisting; i++) {
		struct dive *old = dive_table.dives[i];

		if (match_one_dive(match, old))
			return 1;
	}
	return 0;
}

static inline int year(int year)
{
	if (year < 70)
		return year + 2000;
	if (year < 100)
		return year + 1900;
	return year;
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
		dive->dc.serial = strdup(str->value);
		/* should we just overwrite this whenever we have the "Serial" field?
		 * It's a much better deviceid then what we have so far... for now I'm leaving it as is */
		if (!dive->dc.deviceid)
			dive->dc.deviceid = calculate_string_hash(str->value);
		return;
	}
	if (!strcmp(str->desc, "FW Version")) {
		dive->dc.fw_version = strdup(str->value);
		return;
	}
}
#endif

static dc_status_t libdc_header_parser(dc_parser_t *parser, struct device_data_t *devdata, struct dive *dive)
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
	const char *date_string = get_dive_date_c_string(dive->when);
	dev_info(devdata, translate("gettextFromC", "Dive %d: %s"), import_dive_number, date_string);
	free((void *)date_string);

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
		dive->dc.maxdepth.mm = rint(maxdepth * 1000);

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
		dive->dc.salinity = rint(salinity.density * 10.0);

	double surface_pressure = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_ATMOSPHERIC, 0, &surface_pressure);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, translate("gettextFromC", "Error obtaining surface pressure"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dc.surface_pressure.mbar = rint(surface_pressure * 1000.0);
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
		dev_info(devdata, translate("gettextFromC", "Error obtaining divemode"));
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
		case DC_DIVEMODE_CC:  /* Closed circuit */
			dive->dc.divemode = CCR;
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
	dc_datetime_t dt = { 0 };
	struct tm tm;
	struct dive *dive = NULL;

	/* reset the deco / ndl data */
	ndl = stoptime = stopdepth = 0;
	in_deco = false;

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

	// Parse the dive's header data
	rc = libdc_header_parser (parser, devdata, dive);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, translate("getextFromC", "Error parsing the header"));
		goto error_exit;
	}

	dive->dc.model = strdup(devdata->model);
	dive->dc.diveid = calculate_diveid(fingerprint, fsize);

	// Initialize the sample data.
	rc = parse_samples(devdata, &dive->dc, parser);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, translate("gettextFromC", "Error parsing the samples"));
		goto error_exit;
	}

	/* If we already saw this dive, abort. */
	if (!devdata->force_download && find_dive(&dive->dc))
		goto error_exit;

	dc_parser_destroy(parser);

	/* Various libdivecomputer interface fixups */
	if (first_temp_is_air && dive->dc.samples) {
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
		dev_info(devdata, translate("gettextFromC", "model=%u (0x%08x), firmware=%u (0x%08x), serial=%u (0x%08x)"),
			 devinfo->model, devinfo->model,
			 devinfo->firmware, devinfo->firmware,
			 devinfo->serial, devinfo->serial);
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

static int
cancel_cb(void *userdata)
{
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

void
logfunc(dc_context_t *context, dc_loglevel_t loglevel, const char *file, unsigned int line, const char *function, const char *msg, void *userdata)
{
	const char *loglevels[] = { "NONE", "ERROR", "WARNING", "INFO", "DEBUG", "ALL" };

	FILE *fp = (FILE *)userdata;

	if (loglevel == DC_LOGLEVEL_ERROR || loglevel == DC_LOGLEVEL_WARNING) {
		fprintf(fp, "%s: %s [in %s:%d (%s)]\n", loglevels[loglevel], msg, file, line, function);
	} else {
		fprintf(fp, "%s: %s\n", loglevels[loglevel], msg);
	}
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

	if (data->libdc_log && logfile_name)
		fp = subsurface_fopen(logfile_name, "w");

	data->libdc_logfile = fp;

	rc = dc_context_new(&data->context);
	if (rc != DC_STATUS_SUCCESS)
		return translate("gettextFromC", "Unable to create libdivecomputer context");

	if (fp) {
		dc_context_set_loglevel(data->context, DC_LOGLEVEL_ALL);
		dc_context_set_logfunc(data->context, logfunc, fp);
	}

	err = translate("gettextFromC", "Unable to open %s %s (%s)");
	rc = dc_device_open(&data->device, data->context, data->descriptor, data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		err = do_device_import(data);
		/* TODO: Show the logfile to the user on error. */
		dc_device_close(data->device);
		data->device = NULL;
	} else if (subsurface_access(data->devname, R_OK | W_OK) != 0)
		err = translate("gettextFromC", "Insufficient privileges to open the device %s %s (%s)");

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

	switch (data->descriptor->type) {
	case DC_FAMILY_UWATEC_ALADIN:
	case DC_FAMILY_UWATEC_MEMOMOUSE:
		rc = uwatec_memomouse_parser_create(&parser, data->context, 0, 0);
		break;
	case DC_FAMILY_UWATEC_SMART:
	case DC_FAMILY_UWATEC_MERIDIAN:
		rc = uwatec_smart_parser_create (&parser, data->context, data->descriptor->model, 0, 0);
		break;
	case DC_FAMILY_HW_OSTC:
		rc = hw_ostc_parser_create (&parser, data->context, data->deviceid, 0);
		break;
	case DC_FAMILY_HW_FROG:
	case DC_FAMILY_HW_OSTC3:
		rc = hw_ostc_parser_create (&parser, data->context, data->deviceid, 1);
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
	if (data->descriptor->type != DC_FAMILY_UWATEC_ALADIN && data->descriptor->type != DC_FAMILY_UWATEC_MEMOMOUSE) {
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
	return(DC_STATUS_SUCCESS);
}
