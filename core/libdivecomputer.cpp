// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <chrono>
#include "gettext.h"
#include "divelog.h"
#include "divesite.h"
#include "sample.h"
#include "subsurface-float.h"
#include "subsurface-string.h"
#include "format.h"
#include "device.h"
#include "dive.h"
#include "errorhelper.h"
#include "event.h"
#include "sha1.h"
#include "subsurface-time.h"

#include <libdivecomputer/version.h>
#include <libdivecomputer/usbhid.h>
#include <libdivecomputer/usb.h>
#include <libdivecomputer/serial.h>
#include <libdivecomputer/irda.h>
#include <libdivecomputer/bluetooth.h>

#include "libdivecomputer.h"
#include "core/version.h"
#include "core/qthelper.h"
#include "core/file.h"
#include <array>
#include <charconv>

std::string dumpfile_name;
std::string logfile_name;
std::string progress_bar_text;
void (*progress_callback)(const std::string &text) = NULL;
double progress_bar_fraction = 0.0;

static int stoptime, stopdepth, ndl, po2, cns, heartbeat, bearing;
static bool in_deco, first_temp_is_air;
static int current_gas_index;

#define INFO(fmt, ...) report_info("INFO: " fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...)	report_info("ERROR: " fmt, ##__VA_ARGS__)

device_data_t::device_data_t()
{
}

device_data_t::~device_data_t()
{
	if (descriptor)
		dc_descriptor_free(descriptor);
}

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

/**
 * @brief get_deeper_gasmix Returns the gas mix with the deeper MOD.
 * NOTE: Parameters are passed by value in order to use them as local working
 * storage.
 * Invalid gas mixes are converted to air for the purpose of this operation.
 * The gas mix with the lower MOD is taken as the one with the lower O2 content,
 * or, if equal, the one with the higher HE content. No actual MOD calculations
 * are performed.
 * @param a The first gas mix to compare.
 * @param b The second gas mix to compare.
 * @return The gas mix with the deeper MOD.
 */
static struct gasmix get_deeper_gasmix(struct gasmix a, struct gasmix b)
{
	if (same_gasmix(a, gasmix_invalid)) {
		a = gasmix_air;
	}
	if (same_gasmix(b, gasmix_invalid)) {
		b = gasmix_air;
	}

	if (get_o2(a) < get_o2(b)) {
		return a;
	}
	if (get_o2(a) > get_o2(b)) {
		return b;
	}
	return get_he(a) < get_he(b) ? b : a;
}

/**
 * @brief parse_gasmixes matches gas mixes with cylinders
 * This function retrieves all tanks and gas mixes reported by libdivecomputer
 * and attepmts to match them. The matching logic assigns the mixes to the
 * tanks in a 1:1 ordering.
 * If there are more gas mixes than tanks, additional tanks are created.
 * If there are fewer gas mixes than tanks, the remaining tanks are assigned to
 * the gas mix with the lowest (deepest) MOD.
 * @param devdata The dive computer data.
 * @param dive The dive to which these tanks and gas mixes will be assigned.
 * @param parser The libdivecomputer parser data.
 * @param ngases The number of gas mixes to process.
 * @return DC_STATUS_SUCCESS on success, otherwise an error code.
 */
static dc_status_t parse_gasmixes(device_data_t *devdata, struct dive *dive, dc_parser_t *parser, unsigned int ngases)
{
	static bool shown_warning = false;
	unsigned int i;
	dc_status_t rc;

	unsigned int ntanks = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_TANK_COUNT, 0, &ntanks);
	if (rc == DC_STATUS_SUCCESS) {
		if (ntanks && ntanks < ngases) {
			shown_warning = true;
			report_error("Warning: different number of gases (%d) and cylinders (%d)", ngases, ntanks);
		} else if (ntanks > ngases) {
			shown_warning = true;
			report_error("Warning: smaller number of gases (%d) than cylinders (%d).", ngases, ntanks);
		}
	}
	bool no_volume = true;
	struct gasmix bottom_gas = { 100_percent, 0_percent }; /* Default to pure O2, or air if there are no mixes defined */
	if (ngases == 0) {
		bottom_gas = gasmix_air;
	}

	dive->cylinders.clear();
	for (i = 0; i < std::max(ngases, ntanks); i++) {
		cylinder_t cyl;
		cyl.cylinder_use = NOT_USED;

		if (i < ngases) {
			dc_gasmix_t gasmix = { 0 };
			int o2, he;

			rc = dc_parser_get_field(parser, DC_FIELD_GASMIX, i, &gasmix);
			if (rc == DC_STATUS_SUCCESS) {
				o2 = lrint(gasmix.oxygen * 1000);
				he = lrint(gasmix.helium * 1000);

				/* Ignore bogus data - libdivecomputer does some crazy stuff */
				if (o2 + he <= O2_IN_AIR || o2 > 1000) {
					if (!shown_warning) {
						shown_warning = true;
						report_error("unlikely dive gas data from libdivecomputer: o2 = %.3f he = %.3f", gasmix.oxygen, gasmix.helium);
					}
					o2 = 0;
				}
				if (he < 0 || o2 + he > 1000) {
					if (!shown_warning) {
						shown_warning = true;
						report_error("unlikely dive gas data from libdivecomputer: o2 = %.3f he = %.3f", gasmix.oxygen, gasmix.helium);
					}
					he = 0;
				}
				cyl.gasmix.o2.permille = o2;
				cyl.gasmix.he.permille = he;
				bottom_gas = get_deeper_gasmix(bottom_gas, cyl.gasmix);

				switch (gasmix.usage) {
				case DC_USAGE_DILUENT:
					cyl.cylinder_use = DILUENT;

					break;
				case DC_USAGE_OXYGEN:
					cyl.cylinder_use = OXYGEN;

					break;
				case DC_USAGE_OPEN_CIRCUIT:
					cyl.cylinder_use = OC_GAS;

					break;
				default:
					if (dive->dcs[0].divemode == CCR)
						cyl.cylinder_use = DILUENT;
					else
						cyl.cylinder_use = OC_GAS;

					break;
				}
			}
		}

		if (i < ntanks) {
			// If we've run out of gas mixes, assign this cylinder to bottom
			// gas. Note that this can be overridden below if the dive computer
			// explicitly reports a gas mix for this tank.
			if (i >= ngases) {
				cyl.gasmix = bottom_gas;
			}
			dc_tank_t tank = { 0 };
			rc = dc_parser_get_field(parser, DC_FIELD_TANK, i, &tank);
			if (rc == DC_STATUS_SUCCESS) {
				cyl.type.size.mliter = lrint(tank.volume * 1000);
				cyl.type.workingpressure.mbar = lrint(tank.workpressure * 1000);

				if (tank.type & DC_TANKVOLUME_IMPERIAL) {
					if (devdata->model == "Suunto EON Steel") {
						/* Suunto EON Steele gets this wrong. Badly.
						 * but on the plus side it only supports a few imperial sizes,
						 * so let's try and guess at least the most common ones.
						 * First, the pressures are off by a constant factor. WTF?
						 * Then we can round the wet sizes so we get to multiples of 10
						 * for cuft sizes (as that's all that you can enter) */
						cyl.type.workingpressure.mbar = lrint(
							cyl.type.workingpressure.mbar * 206.843 / 206.7 );
						char name_buffer[17];
						int rounded_size = lrint(ml_to_cuft(cyl.gas_volume(
							cyl.type.workingpressure).mliter));
						rounded_size = (int)((rounded_size + 5) / 10) * 10;
						switch (cyl.type.workingpressure.mbar) {
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
						cyl.type.description = name_buffer;
						cyl.type.size.mliter = lrint(cuft_to_l(rounded_size) * 1000 /
											mbar_to_atm(cyl.type.workingpressure.mbar));
					}
				}
				if (tank.gasmix != DC_GASMIX_UNKNOWN && tank.gasmix != i) { // we don't handle this, yet
					shown_warning = true;
					report_error("gasmix %d for tank %d doesn't match", tank.gasmix, i);
				}
			}
			if (!nearly_0(tank.volume))
				no_volume = false;

			// this new API also gives us the beginning and end pressure for the tank
			// normally 0 is not a valid pressure, but for some Uwatec dive computers we
			// don't get the actual start and end pressure, but instead a start pressure
			// that matches the consumption and an end pressure of always 0
			// In order to make this work, we arbitrary shift this up by 30bar so the
			// rest of the code treats this as if they were valid values
			if (!nearly_0(tank.beginpressure)) {
				if (!nearly_0(tank.endpressure)) {
					cyl.start.mbar = lrint(tank.beginpressure * 1000);
					cyl.end.mbar = lrint(tank.endpressure * 1000);
				} else if (devdata->vendor == "Uwatec") {
					cyl.start.mbar = lrint(tank.beginpressure * 1000 + 30000);
					cyl.end = 30_bar;
				}
			}
		}
		if (no_volume) {
			/* for the first tank, if there is no tanksize available from the
			 * dive computer, fill in the default tank information (if set) */
			fill_default_cylinder(dive, &cyl);
		}
		/* whatever happens, make sure there is a name for the cylinder */
		if (cyl.type.description.empty())
			cyl.type.description = translate("gettextFromC", "unknown");

		dive->cylinders.push_back(std::move(cyl));
	}
	return DC_STATUS_SUCCESS;
}

static void handle_event(struct divecomputer *dc, const struct sample &sample, dc_sample_value_t value)
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
		[SAMPLE_EVENT_FLOOR]			= std::array<const char *, 2>{QT_TRANSLATE_NOOP3("gettextFromC", "below floor", "event showing dive is below deco floor and adding deco time")}[1],
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
	time += sample.time.seconds;

	ev = add_event(dc, time, type, value.event.flags, value.event.value, name);
	if (ev->is_gaschange() && ev->gas.index >= 0)
		current_gas_index = ev->gas.index;
}

static void handle_gasmix(struct divecomputer *dc, const struct sample &sample, int idx)
{
	/* TODO: Verify that index is not higher than the number of cylinders */
	if (idx < 0)
		return;
	add_event(dc, sample.time.seconds, SAMPLE_EVENT_GASCHANGE2, idx+1, 0, "gaschange");
	current_gas_index = idx;
}

void
sample_cb(dc_sample_type_t type, const dc_sample_value_t *pvalue, void *userdata)
{
	struct divecomputer *dc = (divecomputer *)userdata;
	dc_sample_value_t value = *pvalue;

	/*
	 * DC_SAMPLE_TIME is special: it creates a new sample.
	 * Other types fill in an existing sample.
	 */
	if (type == DC_SAMPLE_TIME) {
		// Create a new sample.
		// Mark depth as negative
		struct sample *sample = prepare_sample(dc);
		sample->time.seconds = value.time / 1000;
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
		return;
	}

	if (dc->samples.empty())
		prepare_sample(dc);
	struct sample &sample = dc->samples.back();

	switch (type) {
	case DC_SAMPLE_DEPTH:
		sample.depth.mm = lrint(value.depth * 1000);
		break;
	case DC_SAMPLE_PRESSURE:
		add_sample_pressure(&sample, value.pressure.tank, lrint(value.pressure.value * 1000));
		break;
	case DC_SAMPLE_GASMIX:
		handle_gasmix(dc, sample, value.gasmix);
		break;
	case DC_SAMPLE_TEMPERATURE:
		sample.temperature.mkelvin = C_to_mkelvin(value.temperature);
		break;
	case DC_SAMPLE_EVENT:
		handle_event(dc, sample, value);
		break;
	case DC_SAMPLE_RBT:
		sample.rbt.seconds = (!strncasecmp(dc->model.c_str(), "suunto", 6)) ? value.rbt : value.rbt * 60;
		break;
#ifdef DC_SAMPLE_TTS
	case DC_SAMPLE_TTS:
		sample.tts.seconds = value.time;
		break;
#endif
	case DC_SAMPLE_HEARTBEAT:
		sample.heartbeat = heartbeat = value.heartbeat;
		break;
	case DC_SAMPLE_BEARING:
		sample.bearing.degrees = bearing = value.bearing;
		break;
#ifdef DEBUG_DC_VENDOR
	case DC_SAMPLE_VENDOR:
		printf("   <vendor time='%u:%02u' type=\"%u\" size=\"%u\">", FRACTION_TUPLE(sample.time.seconds, 60),
		       value.vendor.type, value.vendor.size);
		for (int i = 0; i < value.vendor.size; ++i)
			printf("%02X", ((unsigned char *)value.vendor.data)[i]);
		printf("</vendor>\n");
		break;
#endif
	case DC_SAMPLE_SETPOINT:
		/* for us a setpoint means constant pO2 from here */
		sample.setpoint.mbar = po2 = lrint(value.setpoint * 1000);
		break;
	case DC_SAMPLE_PPO2: {
		unsigned int sensor_id = value.ppo2.sensor;
		if (sensor_id == DC_SENSOR_NONE) {
			sample.o2sensor[DC_REPORTED_PPO2].mbar = lrint(value.ppo2.value * 1000);
		} else if (sensor_id >= MAX_O2_SENSORS) {
			report_error("%d is more o2 sensors than we can handle", sensor_id + 1);

			break;
		} else {
			sample.o2sensor[sensor_id].mbar = lrint(value.ppo2.value * 1000);

			// Set the amount of detected o2 sensors
			if (sensor_id + 1 > dc->no_o2sensors)
				dc->no_o2sensors = sensor_id + 1;
		}
		break;
	}
	case DC_SAMPLE_CNS:
		sample.cns = cns = lrint(value.cns * 100);
		break;
	case DC_SAMPLE_DECO:
		if (value.deco.type == DC_DECO_NDL) {
			sample.ndl.seconds = ndl = value.deco.time;
			sample.stopdepth.mm = stopdepth = lrint(value.deco.depth * 1000.0);
			sample.in_deco = in_deco = false;
		} else if (value.deco.type == DC_DECO_DECOSTOP ||
			   value.deco.type == DC_DECO_DEEPSTOP) {
			sample.stopdepth.mm = stopdepth = lrint(value.deco.depth * 1000.0);
			sample.stoptime.seconds = stoptime = value.deco.time;
			sample.in_deco = in_deco = stopdepth > 0;
			ndl = 0;
		} else if (value.deco.type == DC_DECO_SAFETYSTOP) {
			sample.in_deco = in_deco = false;
			sample.stopdepth.mm = stopdepth = lrint(value.deco.depth * 1000.0);
			sample.stoptime.seconds = stoptime = value.deco.time;
		}
		sample.tts.seconds = value.deco.tts;
	default:
		break;
	}
}

static void dev_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	progress_bar_text = vformat_string_std(fmt, ap);
	va_end(ap);
	if (verbose)
		INFO("dev_info: %s", progress_bar_text.c_str());

	if (progress_callback)
		(*progress_callback)(progress_bar_text);
}

static int import_dive_number = 0;

static void download_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	std::string buffer = vformat_string_std(fmt, ap);
	va_end(ap);
	report_error("Dive %d: %s", import_dive_number, buffer.c_str());
}

static dc_status_t parse_samples(device_data_t *, struct divecomputer *dc, dc_parser_t *parser)
{
	// Parse the sample data.
	return dc_parser_samples_foreach(parser, sample_cb, dc);
}

static int might_be_same_dc(const struct divecomputer &a, const struct divecomputer &b)
{
	if (a.model.empty() || b.model.empty())
		return 1;
	if (strcasecmp(a.model.c_str(), b.model.c_str()))
		return 0;
	if (!a.deviceid || !b.deviceid)
		return 1;
	return a.deviceid == b.deviceid;
}

static bool match_one_dive(const struct divecomputer &a, const struct dive &dive)
{
	/*
	 * Walk the existing dive computer data,
	 * see if we have a match (or an anti-match:
	 * the same dive computer but a different
	 * dive ID).
	 */
	for (auto &b: dive.dcs) {
		if (match_one_dc(a, b) > 0)
			return true;
	}

	/* Ok, no exact dive computer match. Does the date match? */
	for (auto &b: dive.dcs) {
		if (a.when == b.when && might_be_same_dc(a, b))
			return true;
	}

	return false;
}

/*
 * Check if this dive already existed before the import
 */
static bool find_dive(const struct divecomputer &match)
{
	return std::any_of(divelog.dives.rbegin(), divelog.dives.rend(),
			   [&match] (auto &old) { return match_one_dive(match, *old);} );
}

/*
 * The dive ID for libdivecomputer dives is the first word of the
 * SHA1 of the fingerprint, if it exists.
 *
 * NOTE! This is byte-order dependent, and I don't care.
 */
static uint32_t calculate_diveid(const unsigned char *fingerprint, unsigned int fsize)
{
	if (!fingerprint || !fsize)
		return 0;

	return SHA1_uint32(fingerprint, fsize);
}

uint32_t calculate_string_hash(const char *str)
{
	return calculate_diveid((const unsigned char *)str, strlen(str));
}

static void parse_string_field(device_data_t *devdata, struct dive *dive, dc_field_string_t *str)
{
	// Our dive ID is the string hash of the "Dive ID" string
	if (!strcmp(str->desc, "Dive ID")) {
		if (!dive->dcs[0].diveid)
			dive->dcs[0].diveid = calculate_string_hash(str->value);
		return;
	}

	// This will pick up serial number and firmware data
	add_extra_data(&dive->dcs[0], str->desc, str->value);

	/* GPS data? */
	if (!strncmp(str->desc, "GPS", 3)) {
		char *line = (char *) str->value;
		location_t location;

		/* Do we already have a divesite? */
		if (dive->dive_site) {
			/*
			 * "GPS1" always takes precedence, anything else
			 * we'll just pick the first "GPS*" that matches.
			 */
			if (strcmp(str->desc, "GPS1") != 0)
				return;
		}
		parse_location(line, &location);

		if (location.lat.udeg && location.lon.udeg) {
			unregister_dive_from_dive_site(dive);
			devdata->log->sites.create(std::string(str->value), location)->add_dive(dive);
		}
	}
}

static dc_status_t libdc_header_parser(dc_parser_t *parser, device_data_t *devdata, struct dive *dive)
{
	dc_status_t rc = static_cast<dc_status_t>(0);
	dc_datetime_t dt = { 0 };
	struct tm tm;

	rc = dc_parser_get_datetime(parser, &dt);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error parsing the datetime"));
		return rc;
	}

	// Our deviceid is the hash of the serial number
	dive->dcs[0].deviceid = 0;

	if (rc == DC_STATUS_SUCCESS) {
		tm.tm_year = dt.year;
		tm.tm_mon = dt.month - 1;
		tm.tm_mday = dt.day;
		tm.tm_hour = dt.hour;
		tm.tm_min = dt.minute;
		tm.tm_sec = dt.second;
		dive->when = dive->dcs[0].when = utc_mktime(&tm);
	}

	// Parse the divetime.
	std::string date_string = get_dive_date_c_string(dive->when);
	dev_info(translate("gettextFromC", "Dive %d: %s"), import_dive_number, date_string.c_str());

	unsigned int divetime = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_DIVETIME, 0, &divetime);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error parsing the divetime"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dcs[0].duration.seconds = divetime;

	// Parse the maxdepth.
	double maxdepth = 0.0;
	rc = dc_parser_get_field(parser, DC_FIELD_MAXDEPTH, 0, &maxdepth);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error parsing the maxdepth"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dcs[0].maxdepth.mm = lrint(maxdepth * 1000);

	// Parse temperatures
	double temperature;
	dc_field_type_t temp_fields[] = {DC_FIELD_TEMPERATURE_SURFACE,
					 DC_FIELD_TEMPERATURE_MAXIMUM,
					 DC_FIELD_TEMPERATURE_MINIMUM};
	for (int i = 0; i < 3; i++) {
		rc = dc_parser_get_field(parser, temp_fields[i], 0, &temperature);
		if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
			download_error(translate("gettextFromC", "Error parsing temperature"));
			return rc;
		}
		if (rc == DC_STATUS_SUCCESS)
			switch(i) {
			case 0:
				dive->dcs[0].airtemp.mkelvin = C_to_mkelvin(temperature);
				break;
			case 1: // we don't distinguish min and max water temp here, so take min if given, max otherwise
			case 2:
				dive->dcs[0].watertemp.mkelvin = C_to_mkelvin(temperature);
				break;
			}
	}

	// Parse the gas mixes.
	unsigned int ngases = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_GASMIX_COUNT, 0, &ngases);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error parsing the gas mix count"));
		return rc;
	}

	// Check if the libdivecomputer version already supports salinity & atmospheric
	dc_salinity_t salinity = {
		.type = DC_WATER_SALT,
		.density = SEAWATER_SALINITY / 10.0
	};
	rc = dc_parser_get_field(parser, DC_FIELD_SALINITY, 0, &salinity);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error obtaining water salinity"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS) {
		dive->dcs[0].salinity = lrint(salinity.density * 10.0);
		if (dive->dcs[0].salinity == 0) {
			// sometimes libdivecomputer gives us density values, sometimes just
			// a water type and a density of zero; let's make this work as best as we can
			switch (salinity.type) {
			case DC_WATER_FRESH:
				dive->dcs[0].salinity = FRESHWATER_SALINITY;
				break;
			default:
				dive->dcs[0].salinity = SEAWATER_SALINITY;
				break;
			}
		}
	}

	double surface_pressure = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_ATMOSPHERIC, 0, &surface_pressure);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error obtaining surface pressure"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		dive->dcs[0].surface_pressure.mbar = lrint(surface_pressure * 1000.0);

	// The dive parsing may give us more device information
	int idx;
	for (idx = 0; idx < 100; idx++) {
		dc_field_string_t str = { NULL };
		rc = dc_parser_get_field(parser, DC_FIELD_STRING, idx, &str);
		if (rc != DC_STATUS_SUCCESS)
			break;
		if (!str.desc || !str.value)
			break;
		parse_string_field(devdata, dive, &str);
		free((void *)str.value); // libdc gives us copies of the value-string.
	}

	dc_divemode_t divemode;
	rc = dc_parser_get_field(parser, DC_FIELD_DIVEMODE, 0, &divemode);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error obtaining dive mode"));
		return rc;
	}
	if (rc == DC_STATUS_SUCCESS)
		switch(divemode) {
		case DC_DIVEMODE_FREEDIVE:
			dive->dcs[0].divemode = FREEDIVE;
			break;
		case DC_DIVEMODE_GAUGE:
		case DC_DIVEMODE_OC: /* Open circuit */
			dive->dcs[0].divemode = OC;
			break;
		case DC_DIVEMODE_CCR:  /* Closed circuit rebreather*/
			dive->dcs[0].divemode = CCR;
			break;
		case DC_DIVEMODE_SCR:  /* Semi-closed circuit rebreather */
			dive->dcs[0].divemode = PSCR;
			break;
		}

	rc = parse_gasmixes(devdata, dive, parser, ngases);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		download_error(translate("gettextFromC", "Error parsing the gas mix"));
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
	dc_status_t rc;
	dc_parser_t *parser = NULL;
	device_data_t *devdata = (device_data_t *)userdata;

	/* reset static data, that is only valid per dive */
	stoptime = stopdepth = po2 = cns = heartbeat = 0;
	ndl = bearing = -1;
	in_deco = false;
	current_gas_index = -1;

	import_dive_number++;

	rc = dc_parser_new(&parser, devdata->device, data, size);
	if (rc != DC_STATUS_SUCCESS) {
		download_error(translate("gettextFromC", "Unable to create parser for %s %s: %d"), devdata->vendor.c_str(), devdata->product.c_str(), errmsg(rc));
		return true;
	}

	auto dive = std::make_unique<struct dive>();

	// Fill in basic fields
	dive->dcs[0].model = devdata->model;
	dive->dcs[0].diveid = calculate_diveid(fingerprint, fsize);

	// Parse the dive's header data
	rc = libdc_header_parser (parser, devdata, dive.get());
	if (rc != DC_STATUS_SUCCESS) {
		download_error(translate("getextFromC", "Error parsing the header: %s"), errmsg(rc));
		goto error_exit;
	}

	// Initialize the sample data.
	rc = parse_samples(devdata, &dive->dcs[0], parser);
	if (rc != DC_STATUS_SUCCESS) {
		download_error(translate("gettextFromC", "Error parsing the samples: %s"), errmsg(rc));
		goto error_exit;
	}

	dc_parser_destroy(parser);

	/*
	 * Save off fingerprint data.
	 *
	 * NOTE! We do this after parsing the dive fully, so that
	 * we have the final deviceid here.
	 */
	if (fingerprint && fsize && !devdata->fingerprint) {
		devdata->fingerprint = (unsigned char *)calloc(fsize, 1);
		if (devdata->fingerprint) {
			devdata->fsize = fsize;
			devdata->fdeviceid = dive->dcs[0].deviceid;
			devdata->fdiveid = dive->dcs[0].diveid;
			memcpy(devdata->fingerprint, fingerprint, fsize);
		}
	}

	/* If we already saw this dive, abort. */
	if (!devdata->force_download && find_dive(dive->dcs[0])) {
		std::string date_string = get_dive_date_c_string(dive->when);
		dev_info(translate("gettextFromC", "Already downloaded dive at %s"), date_string.c_str());
		return false;
	}

	/* Various libdivecomputer interface fixups */
	if (dive->dcs[0].airtemp.mkelvin == 0 && first_temp_is_air && !dive->dcs[0].samples.empty()) {
		dive->dcs[0].airtemp = dive->dcs[0].samples[0].temperature;
		dive->dcs[0].samples[0].temperature = 0_K;
	}

	/* special case for bug in Tecdiving DiveComputer.eu
	 * often the first sample has a water temperature of 0C, followed by the correct
	 * temperature in the next sample */
	if (dive->dcs[0].model == "Tecdiving DiveComputer.eu" && !dive->dcs[0].samples.empty() &&
	    dive->dcs[0].samples[0].temperature.mkelvin == ZERO_C_IN_MKELVIN &&
	    dive->dcs[0].samples[1].temperature.mkelvin > dive->dcs[0].samples[0].temperature.mkelvin)
		dive->dcs[0].samples[0].temperature.mkelvin = dive->dcs[0].samples[1].temperature.mkelvin;

	devdata->log->dives.record_dive(std::move(dive));
	return true;

error_exit:
	dc_parser_destroy(parser);
	return true;
}

#ifndef O_BINARY
  #define O_BINARY 0
#endif
static void do_save_fingerprint(device_data_t *devdata, const char *tmp, const char *final)
{
	int fd, written = -1;

	fd = subsurface_open(tmp, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0)
		return;

	if (verbose)
		dev_info("Saving fingerprint for %08x:%08x to '%s'",
			 devdata->fdeviceid, devdata->fdiveid, final);

	/* The fingerprint itself.. */
	written = write(fd, devdata->fingerprint, devdata->fsize);

	/* ..followed by the device ID and dive ID of the fingerprinted dive */
	if (write(fd, &devdata->fdeviceid, 4) != 4 || write(fd, &devdata->fdiveid, 4) != 4)
		written = -1;

	/* I'd like to do fsync() here too, but does Windows support it? */
	if (close(fd) < 0)
		written = -1;

	if (written == (int)devdata->fsize) {
		if (!subsurface_rename(tmp, final))
			return;
	}
	unlink(tmp);
}

static std::string fingerprint_file(device_data_t *devdata)
{
	uint32_t model, serial;

	// Model hash and libdivecomputer 32-bit 'serial number' for the file name
	model = calculate_string_hash(devdata->model.c_str());
	serial = devdata->devinfo.serial;

	return format_string_std("%s/fingerprints/%04x.%u",
		system_default_directory().c_str(),
		model, serial);
}

/*
 * Save the fingerprint after a successful download
 *
 * NOTE! At this point, we have the final device ID for the divecomputer
 * we downloaded from. But that 'deviceid' is actually not useful, because
 * at the point where we want to _load_ this, we only have the libdivecomputer
 * DC_EVENT_DEVINFO state (devdata->devinfo).
 *
 * Now, we do have the devdata->devinfo at save time, but at load time we
 * need to verify not only that it's the proper fingerprint file: we also
 * need to check that we actually have the particular dive that was
 * associated with that fingerprint state.
 *
 * That means that the fingerprint save file needs to include not only the
 * fingerprint data itself, but also enough data to look up a dive unambiguously
 * when loading the fingerprint. And the fingerprint data needs to be looked
 * up using the DC_EVENT_DEVINFO data.
 *
 * End result:
 *
 *   - fingerprint filename depends on the model name and 'devinfo.serial'
 *     so that we can look it up at DC_EVENT_DEVINFO time before the full
 *     info has been parsed.
 *
 *   - the fingerprint file contains the 'diveid' of the fingerprinted dive,
 *     which is just a hash of the fingerprint itself.
 *
 *   - we also save the final 'deviceid' in the fingerprint file, so that
 *     looking up the dive associated with the fingerprint is possible.
 */
static void save_fingerprint(device_data_t *devdata)
{
	// Don't try to save nonexistent fingerprint data
	if (!devdata->fingerprint || !devdata->fdiveid)
		return;

	// Make sure the fingerprints directory exists
	std::string dir = system_default_directory() + "/fingerprints";
	subsurface_mkdir(dir.c_str());

	std::string final = fingerprint_file(devdata);
	std::string tmp = final + ".tmp";

	do_save_fingerprint(devdata, tmp.c_str(), final.c_str());
}

/*
 * The fingerprint cache files contain the actual libdivecomputer
 * fingerprint, followed by 8 bytes of (deviceid,diveid) data.
 *
 * Before we use the fingerprint data, verify that we actually
 * do have that fingerprinted dive.
 */
static void verify_fingerprint(dc_device_t *device, device_data_t *devdata, const unsigned char *buffer, size_t size)
{
	uint32_t diveid, deviceid;

	if (size <= 8)
		return;
	size -= 8;

	/* Get the dive ID from the end of the fingerprint cache file.. */
	memcpy(&deviceid, buffer + size, 4);
	memcpy(&diveid, buffer + size + 4, 4);

	if (verbose)
		dev_info(" ... fingerprinted dive %08x:%08x", deviceid, diveid);
	/* Only use it if we *have* that dive! */
	if (!divelog.dives.has_dive(deviceid, diveid)) {
		if (verbose)
			dev_info(" ... dive not found");
		return;
	}
	dc_device_set_fingerprint(device, buffer, size);
	if (verbose)
		dev_info(" ... fingerprint of size %zu", size);
}

/*
 * Look up the fingerprint from the fingerprint caches, and
 * give it to libdivecomputer to avoid downloading already
 * downloaded dives.
 */
static void lookup_fingerprint(dc_device_t *device, device_data_t *devdata)
{
	if (devdata->force_download)
		return;

	/* first try our in memory data - raw_data is owned by the table, the dc_device_set_fingerprint function copies the data */
	auto [fsize, raw_data] = get_fingerprint_data(fingerprints, calculate_string_hash(devdata->model.c_str()), devdata->devinfo.serial);
	if (fsize) {
		if (verbose)
			dev_info("... found fingerprint in dive table");
		dc_device_set_fingerprint(device, raw_data, fsize);
		return;
	}
	/* now check if we have a fingerprint on disk */
	std::string cachename = fingerprint_file(devdata);
	if (verbose)
		dev_info("Looking for fingerprint in '%s'", cachename.c_str());
	auto [mem, err] = readfile(cachename.c_str());
	if (err > 0) {
		if (verbose)
			dev_info(" ... got %zu bytes", mem.size());
		verify_fingerprint(device, devdata, (unsigned char *)mem.data(), mem.size());
	}
}

static void event_cb(dc_device_t *device, dc_event_type_t event, const void *data, void *userdata)
{
	static unsigned int last = 0;
	const dc_event_progress_t *progress = (dc_event_progress_t *)data;
	const dc_event_devinfo_t *devinfo = (dc_event_devinfo_t *)data;
	const dc_event_clock_t *clock = (dc_event_clock_t *)data;
	const dc_event_vendor_t *vendor = (dc_event_vendor_t *)data;
	device_data_t *devdata = (device_data_t *)userdata;

	switch (event) {
	case DC_EVENT_WAITING:
		dev_info(translate("gettextFromC", "Event: waiting for user action"));
		break;
	case DC_EVENT_PROGRESS:
		/* this seems really dumb... but having no idea what is happening on long
		 * downloads makes people think that the app is hung;
		 * since the progress is in bytes downloaded (usually), simply give updates in 10k increments
		 */
		if (progress->current < last)
			/* this is a new communication with the divecomputer */
			last = progress->current;
		if (progress->current > last + 10240) {
			last = progress->current;
			dev_info(translate("gettextFromC", "read %dkb"), progress->current / 1024);
		}
		if (progress->maximum)
			progress_bar_fraction = (double)progress->current / (double)progress->maximum;
		break;
	case DC_EVENT_DEVINFO:
		if (dc_descriptor_get_model(devdata->descriptor) != devinfo->model) {
			dc_descriptor_t *better_descriptor = get_descriptor(dc_descriptor_get_type(devdata->descriptor), devinfo->model);
			if (better_descriptor != NULL) {
				report_info("EVENT_DEVINFO gave us a different detected product (model %d instead of %d), which we are using now.",
					devinfo->model, dc_descriptor_get_model(devdata->descriptor));
				devdata->descriptor = better_descriptor;
				devdata->product = dc_descriptor_get_product(better_descriptor);
				devdata->vendor = dc_descriptor_get_vendor(better_descriptor);
				devdata->model = devdata->vendor + " " + devdata->product;
			} else {
				report_info("EVENT_DEVINFO gave us a different detected product (model %d instead of %d), but that one is unknown.",
					devinfo->model, dc_descriptor_get_model(devdata->descriptor));
			}
		}
		dev_info(translate("gettextFromC", "model=%s firmware=%u serial=%u"),
			 devdata->product.c_str(), devinfo->firmware, devinfo->serial);
		if (devdata->libdc_logfile) {
			fprintf(devdata->libdc_logfile, "Event: model=%u (0x%08x), firmware=%u (0x%08x), serial=%u (0x%08x)\n",
				devinfo->model, devinfo->model,
				devinfo->firmware, devinfo->firmware,
				devinfo->serial, devinfo->serial);
		}

		devdata->devinfo = *devinfo;
		lookup_fingerprint(device, devdata);

		break;
	case DC_EVENT_CLOCK:
		dev_info(translate("gettextFromC", "Event: systime=%" PRId64 ", devtime=%u\n"),
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

static int cancel_cb(void *)
{
	return import_thread_cancelled;
}

static std::string do_device_import(device_data_t *data)
{
	dc_status_t rc;
	dc_device_t *device = data->device;

	data->model = data->vendor + " " + data->product;

	// Register the event handler.
	int events = DC_EVENT_WAITING | DC_EVENT_PROGRESS | DC_EVENT_DEVINFO | DC_EVENT_CLOCK | DC_EVENT_VENDOR;
	rc = dc_device_set_events(device, events, event_cb, data);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info("Import error: %s", errmsg(rc));

		return translate("gettextFromC", "Error registering the event handler.");
	}

	// Register the cancellation handler.
	rc = dc_device_set_cancel(device, cancel_cb, data);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info("Import error: %s", errmsg(rc));

		return translate("gettextFromC", "Error registering the cancellation handler.");
	}

	if (data->libdc_dump) {
		dc_buffer_t *buffer = dc_buffer_new(0);

		rc = dc_device_dump(device, buffer);
		if (rc == DC_STATUS_SUCCESS && !dumpfile_name.empty()) {
			FILE *fp = subsurface_fopen(dumpfile_name.c_str(), "wb");
			if (fp != NULL) {
				fwrite(dc_buffer_get_data(buffer), 1, dc_buffer_get_size(buffer), fp);
				fclose(fp);
			}
		}

		dc_buffer_free(buffer);

		if (rc != DC_STATUS_SUCCESS) {
			progress_bar_fraction = 0.0;

			if (rc == DC_STATUS_UNSUPPORTED)
				return translate("gettextFromC", "Dumping not supported on this device");

			dev_info("Import error: %s", errmsg(rc));

			return translate("gettextFromC", "Dive data dumping error");
		}
	} else {
		rc = dc_device_foreach(device, dive_cb, data);

		if (rc != DC_STATUS_SUCCESS) {
			progress_bar_fraction = 0.0;

			dev_info("Import error: %s", errmsg(rc));

			return translate("gettextFromC", "Dive data import error");
		}
	}

	/* All good */
	return std::string();
}

void logfunc(dc_context_t *, dc_loglevel_t loglevel, const char *file, unsigned int line, const char *function, const char *msg, void *userdata)
{
	const char *loglevels[] = { "NONE", "ERROR", "WARNING", "INFO", "DEBUG", "ALL" };

	static const auto start(std::chrono::steady_clock::now());
	auto now(std::chrono::steady_clock::now());
	double elapsed_seconds = std::chrono::duration<double>(now - start).count();

	FILE *fp = (FILE *)userdata;

	if (loglevel == DC_LOGLEVEL_ERROR || loglevel == DC_LOGLEVEL_WARNING) {
		fprintf(fp, "[%.6f] %s: %s [in %s:%d (%s)]\n",
			elapsed_seconds,
			loglevels[loglevel], msg, file, line, function);
	} else {
		fprintf(fp, "[%6f] %s: %s\n", elapsed_seconds, loglevels[loglevel], msg);
	}
}

/*
 * Get the transports supported by us (as opposed to
 * the list of transports supported by a particular
 * dive computer).
 *
 * This could have various platform rules too..
 */
unsigned int get_supported_transports(device_data_t *data)
{
#if defined(Q_OS_IOS)
	// BLE only - don't bother with being clever.
	return DC_TRANSPORT_BLE;
#endif
	// start out with the list of transports that libdivecomputer claims to support
	// dc_context_get_transports ignores its context argument...
	unsigned int supported = dc_context_get_transports(NULL);

	// then add the ones that we have our own implementations for
#if defined(BT_SUPPORT)
	supported |= DC_TRANSPORT_BLUETOOTH;
#endif
#if defined(BLE_SUPPORT)
	supported |= DC_TRANSPORT_BLE;
#endif
#if defined(Q_OS_ANDROID)
	// we cannot support transports that need libusb, hid, filesystem access, or IRDA on Android
	supported &= ~(DC_TRANSPORT_USB | DC_TRANSPORT_USBHID | DC_TRANSPORT_IRDA | DC_TRANSPORT_USBSTORAGE);
#endif
	if (data) {
		/*
		 * If we have device data available, we can refine this:
		 * We don't support BT or BLE unless bluetooth_mode was set,
		 * and if it was we won't try any of the other transports.
		 */
		if (data->bluetooth_mode) {
			supported &= (DC_TRANSPORT_BLUETOOTH | DC_TRANSPORT_BLE);
			if (starts_with(data->devname, "LE:"))
				supported &= DC_TRANSPORT_BLE;
		} else {
			supported &= ~(DC_TRANSPORT_BLUETOOTH | DC_TRANSPORT_BLE);
		}
	}
	return supported;
}

static dc_status_t usbhid_device_open(dc_iostream_t **iostream, dc_context_t *context, device_data_t *data)
{
	dc_status_t rc;
	dc_iterator_t *iterator = NULL;
	dc_usbhid_device_t *device = NULL;

	// Discover the usbhid device.
	dc_usbhid_iterator_new (&iterator, context, data->descriptor);
	while (dc_iterator_next (iterator, &device) == DC_STATUS_SUCCESS)
		break;
	dc_iterator_free (iterator);

	if (!device) {
		ERROR("didn't find HID device");
		return DC_STATUS_NODEVICE;
	}
	dev_info("Opening USB HID device for %04x:%04x",
		dc_usbhid_device_get_vid(device),
		dc_usbhid_device_get_pid(device));
	rc = dc_usbhid_open(iostream, context, device);
	dc_usbhid_device_free(device);
	return rc;
}

static dc_status_t usb_device_open(dc_iostream_t **iostream, dc_context_t *context, device_data_t *data)
{
	dc_status_t rc;
	dc_iterator_t *iterator = NULL;
	dc_usb_device_t *device = NULL;

	// Discover the usb device.
	dc_usb_iterator_new (&iterator, context, data->descriptor);
	while (dc_iterator_next (iterator, &device) == DC_STATUS_SUCCESS)
		break;
	dc_iterator_free (iterator);

	if (!device)
		return DC_STATUS_NODEVICE;

	dev_info("Opening USB device for %04x:%04x",
		dc_usb_device_get_vid(device),
		dc_usb_device_get_pid(device));
	rc = dc_usb_open(iostream, context, device);
	dc_usb_device_free(device);
	return rc;
}

static dc_status_t irda_device_open(dc_iostream_t **iostream, dc_context_t *context, device_data_t *data)
{
	unsigned int address = 0;
	dc_iterator_t *iterator = NULL;
	dc_irda_device_t *device = NULL;

	// Try to find the IRDA address
	dc_irda_iterator_new (&iterator, context, data->descriptor);
	while (dc_iterator_next (iterator, &device) == DC_STATUS_SUCCESS) {
		address = dc_irda_device_get_address (device);
		dc_irda_device_free (device);
		break;
	}
	dc_iterator_free (iterator);

	// If that fails, use the device name. This will
	// use address 0 if it's not a number.
	if (!address)
		std::from_chars(data->devname.c_str(), data->devname.c_str() + data->devname.size(), address);

	dev_info("Opening IRDA address %u", address);
	return dc_irda_open(&data->iostream, context, address, 1);
}

#if defined(BT_SUPPORT) && !defined(__ANDROID__) && !defined(__APPLE__)
static dc_status_t bluetooth_device_open(dc_context_t *context, device_data_t *data)
{
	dc_bluetooth_address_t address = 0;
	dc_iterator_t *iterator = NULL;
	dc_bluetooth_device_t *device = NULL;

	// Try to find the rfcomm device address
	dc_bluetooth_iterator_new (&iterator, context, data->descriptor);
	while (dc_iterator_next (iterator, &device) == DC_STATUS_SUCCESS) {
		address = dc_bluetooth_device_get_address (device);
		dc_bluetooth_device_free (device);
		break;
	}
	dc_iterator_free (iterator);

	if (!address) {
		dev_info("No rfcomm device found");
		return DC_STATUS_NODEVICE;
	}

	dev_info("Opening rfcomm address %llu", address);
	return dc_bluetooth_open(&data->iostream, context, address, 0);
}
#endif

dc_status_t divecomputer_device_open(device_data_t *data)
{
	dc_status_t rc = DC_STATUS_UNSUPPORTED;
	dc_context_t *context = data->context;
	unsigned int transports, supported;

	transports = dc_descriptor_get_transports(data->descriptor);
	supported = get_supported_transports(data);

	transports &= supported;
	if (!transports) {
		dev_info("Dive computer transport not supported");
		return DC_STATUS_UNSUPPORTED;
	}

#ifdef BT_SUPPORT
	if (transports & DC_TRANSPORT_BLUETOOTH) {
		dev_info("Opening rfcomm stream %s", data->devname.c_str());
#if defined(__ANDROID__) || defined(__APPLE__)
		// we don't have BT on iOS in the first place, so this is for Android and macOS
		rc = rfcomm_stream_open(&data->iostream, context, data->devname.c_str());
#else
		rc = bluetooth_device_open(context, data);
#endif
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}
#endif

#ifdef BLE_SUPPORT
	if (transports & DC_TRANSPORT_BLE) {
		dev_info("Connecting to BLE device %s", data->devname.c_str());
		rc = ble_packet_open(&data->iostream, context, data->devname.c_str(), data);
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}
#endif

	if (transports & DC_TRANSPORT_USBHID) {
		dev_info("Connecting to USB HID device");
		rc = usbhid_device_open(&data->iostream, context, data);
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}

	if (transports & DC_TRANSPORT_USB) {
		dev_info("Connecting to native USB device");
		rc = usb_device_open(&data->iostream, context, data);
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}

	if (transports & DC_TRANSPORT_SERIAL) {
		dev_info("Opening serial device %s", data->devname.c_str());
#ifdef SERIAL_FTDI
		if (!strcasecmp(data->devname.c_str(), "ftdi"))
			return ftdi_open(&data->iostream, context);
#endif
#ifdef __ANDROID__
		if (data->androidUsbDeviceDescriptor)
			return serial_usb_android_open(&data->iostream, context, data->androidUsbDeviceDescriptor);
#endif
		rc = dc_serial_open(&data->iostream, context, data->devname.c_str());
		if (rc == DC_STATUS_SUCCESS)
			return rc;

	}

	if (transports & DC_TRANSPORT_IRDA) {
		dev_info("Connecting to IRDA device");
		rc = irda_device_open(&data->iostream, context, data);
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}

	if (transports & DC_TRANSPORT_USBSTORAGE) {
		dev_info("Opening USB storage at %s", data->devname.c_str());
		rc = dc_usb_storage_open(&data->iostream, context, data->devname.c_str());
		if (rc == DC_STATUS_SUCCESS)
			return rc;
	}

	return rc;
}

dc_status_t divecomputer_sync_time(const device_data_t &data)
{
	if (!data.sync_time)
		return DC_STATUS_SUCCESS;

	dc_datetime_t now;
	dc_datetime_localtime(&now, dc_datetime_now());

	return dc_device_timesync(data.device, &now);
}

dc_loglevel_t get_libdivecomputer_loglevel()
{
	switch (verbose) {
	case 0:
		return DC_LOGLEVEL_ERROR;
	case 1:
		return DC_LOGLEVEL_WARNING;
	case 2:
		return DC_LOGLEVEL_INFO;
	case 3:
	default:
		return DC_LOGLEVEL_DEBUG;
	}
}

std::string do_libdivecomputer_import(device_data_t *data)
{
	dc_status_t rc;
	FILE *fp = NULL;

	import_dive_number = 0;
	first_temp_is_air = 0;
	data->device = NULL;
	data->context = NULL;
	data->iostream = NULL;
	data->fingerprint = NULL;
	data->fsize = 0;

	if (data->libdc_log && !logfile_name.empty())
		fp = subsurface_fopen(logfile_name.c_str(), "w");

	data->libdc_logfile = fp;

	rc = dc_context_new(&data->context);
	if (rc != DC_STATUS_SUCCESS)
		return translate("gettextFromC", "Unable to create libdivecomputer context");

	if (fp) {
		dc_context_set_loglevel(data->context, DC_LOGLEVEL_ALL);
		dc_context_set_logfunc(data->context, logfunc, fp);
		fprintf(data->libdc_logfile, "Subsurface: v%s, ", subsurface_git_version());
		fprintf(data->libdc_logfile, "built with libdivecomputer v%s\n", dc_version(NULL));
	} else {
		dc_context_set_loglevel(data->context, get_libdivecomputer_loglevel());
	}

	std::string err = translate("gettextFromC", "Unable to open %s %s (%s)");

	rc = divecomputer_device_open(data);

	if (rc != DC_STATUS_SUCCESS) {
		dev_info("Import error: %s", errmsg(rc));
	} else {
		dev_info("Connecting ...");
		rc = dc_device_open(&data->device, data->context, data->descriptor, data->iostream);
		if (rc != DC_STATUS_SUCCESS) {
			INFO("dc_device_open error value of %d", rc);
			if (subsurface_access(data->devname.c_str(), R_OK | W_OK) != 0)
#if defined(SUBSURFACE_MOBILE)
				err = translate("gettextFromC", "Error opening the device %s %s (%s).\nIn most cases, in order to debug this issue, it is useful to send the developers the log files. You can copy them to the clipboard in the About dialog.");
#else
				err = translate("gettextFromC", "Error opening the device %s %s (%s).\nIn most cases, in order to debug this issue, a libdivecomputer logfile will be useful.\nYou can create this logfile by selecting the corresponding checkbox in the download dialog.");
#endif
		} else {
			dev_info("Starting import ...");
			err = do_device_import(data);
			/* TODO: Show the logfile to the user on error. */
			dev_info("Import complete");

			if (err.empty() && data->sync_time) {
				dev_info("Syncing dive computer time ...");
				rc = divecomputer_sync_time(*data);

				switch (rc) {
				case DC_STATUS_SUCCESS:
					dev_info("Time sync complete");

					break;
				case DC_STATUS_UNSUPPORTED:
					dev_info("Time sync not supported by dive computer");

					break;
				default:
					dev_info("Time sync failed");

					break;
				}
			}

			dc_device_close(data->device);
			data->device = NULL;
			if (data->log->dives.empty())
				dev_info(translate("gettextFromC", "No new dives downloaded from dive computer"));
		}
		dc_iostream_close(data->iostream);
		data->iostream = NULL;
	}

	dc_context_free(data->context);
	data->context = NULL;

	if (fp) {
		fclose(fp);
	}

	/*
	 * Note that we save the fingerprint unconditionally.
	 * This is ok because we only have fingerprint data if
	 * we got a dive header, and because we will use the
	 * dive id to verify that we actually have the dive
	 * it refers to before we use the fingerprint data.
	 *
	 * For now we save the fingerprint both to the local file system
	 * and to the global fingerprint table (to be then saved out with
	 * the dive log data).
	 */
	save_fingerprint(data);
	if (data->fingerprint && data->fdiveid)
		create_fingerprint_node(fingerprints, calculate_string_hash(data->model.c_str()), data->devinfo.serial,
					data->fingerprint, data->fsize, data->fdeviceid, data->fdiveid);
	free(data->fingerprint);
	data->fingerprint = NULL;

	return err;
}

/*
 * Fills a device_data_t structure with known dc data and a descriptor.
 */
int prepare_device_descriptor(int data_model, dc_family_t dc_fam, device_data_t &dev_data)
{
	dev_data.device = NULL;
	dev_data.context = NULL;

	dc_descriptor_t *data_descriptor = get_descriptor(dc_fam, data_model);
	if (data_descriptor) {
		dev_data.descriptor = data_descriptor;
		dev_data.vendor = dc_descriptor_get_vendor(data_descriptor);
		dev_data.model = dc_descriptor_get_product(data_descriptor);
	} else {
		return 0;
	}

	return 1;
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
dc_status_t libdc_buffer_parser(struct dive *dive, device_data_t *data, const unsigned char *buffer, int size)
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
	case DC_FAMILY_DIVESOFT_FREEDOM:
	case DC_FAMILY_GARMIN:
		rc = dc_parser_new2(&parser, data->context, data->descriptor, buffer, size);
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
	// Do not parse Aladin/Memomouse headers as they are fakes
	// Do not return on error, we can still parse the samples
	if (dc_descriptor_get_type(data->descriptor) != DC_FAMILY_UWATEC_ALADIN && dc_descriptor_get_type(data->descriptor) != DC_FAMILY_UWATEC_MEMOMOUSE) {
		rc = libdc_header_parser (parser, data, dive);
		if (rc != DC_STATUS_SUCCESS) {
			report_error("Error parsing the dive header data. Dive # %d: %s", dive->number, errmsg(rc));
		}
	}
	rc = dc_parser_samples_foreach (parser, sample_cb, &dive->dcs[0]);
	if (rc != DC_STATUS_SUCCESS) {
		report_error("Error parsing the sample data. Dive # %d: %s", dive->number, errmsg(rc));
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
		report_info("Error creating the device descriptor iterator: %s", errmsg(rc));
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
