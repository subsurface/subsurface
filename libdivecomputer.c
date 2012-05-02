#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "display-gtk.h"

#include "libdivecomputer.h"

/* Christ. Libdivecomputer has the worst configuration system ever. */
#ifdef HW_FROG_H
  #define NOT_FROG , 0
  #define LIBDIVECOMPUTER_SUPPORTS_FROG
#else
  #define NOT_FROG
#endif

static GError *error(const char *fmt, ...)
{
	va_list args;
	GError *error;

	va_start(args, fmt);
	error = g_error_new_valist(
		g_quark_from_string("subsurface"),
		DIVE_ERROR_PARSE, fmt, args);
	va_end(args);
	return error;
}

static parser_status_t create_parser(device_data_t *devdata, parser_t **parser)
{
	switch (devdata->type) {
	case DEVICE_TYPE_SUUNTO_SOLUTION:
		return suunto_solution_parser_create(parser);

	case DEVICE_TYPE_SUUNTO_EON:
		return suunto_eon_parser_create(parser, 0);

	case DEVICE_TYPE_SUUNTO_VYPER:
		if (devdata->devinfo.model == 0x01)
			return suunto_eon_parser_create(parser, 1);
		return suunto_vyper_parser_create(parser);

	case DEVICE_TYPE_SUUNTO_VYPER2:
	case DEVICE_TYPE_SUUNTO_D9:
		return suunto_d9_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_UWATEC_ALADIN:
	case DEVICE_TYPE_UWATEC_MEMOMOUSE:
		return uwatec_memomouse_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_UWATEC_SMART:
		return uwatec_smart_parser_create(parser, devdata->devinfo.model, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_REEFNET_SENSUS:
		return reefnet_sensus_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_REEFNET_SENSUSPRO:
		return reefnet_sensuspro_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_REEFNET_SENSUSULTRA:
		return reefnet_sensusultra_parser_create(parser, devdata->clock.devtime, devdata->clock.systime);

	case DEVICE_TYPE_OCEANIC_VTPRO:
		return oceanic_vtpro_parser_create(parser);

	case DEVICE_TYPE_OCEANIC_VEO250:
		return oceanic_veo250_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_OCEANIC_ATOM2:
		return oceanic_atom2_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_MARES_DARWIN:
		return mares_darwin_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_MARES_NEMO:
	case DEVICE_TYPE_MARES_PUCK:
		return mares_nemo_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_MARES_ICONHD:
		return mares_iconhd_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_HW_OSTC:
		return hw_ostc_parser_create(parser NOT_FROG);

#ifdef LIBDIVECOMPUTER_SUPPORTS_FROG
	case DEVICE_TYPE_HW_FROG:
		return hw_ostc_parser_create(parser, 1);
#endif

	case DEVICE_TYPE_CRESSI_EDY:
	case DEVICE_TYPE_ZEAGLE_N2ITION3:
		return cressi_edy_parser_create(parser, devdata->devinfo.model);

	case DEVICE_TYPE_ATOMICS_COBALT:
		return atomics_cobalt_parser_create(parser);

	default:
		return PARSER_STATUS_ERROR;
	}
}

static int parse_gasmixes(struct dive *dive, parser_t *parser, int ngases)
{
	int i;

	for (i = 0; i < ngases; i++) {
		int rc;
		gasmix_t gasmix = {0};
		int o2, he;

		rc = parser_get_field(parser, FIELD_TYPE_GASMIX, i, &gasmix);
		if (rc != PARSER_STATUS_SUCCESS && rc != PARSER_STATUS_UNSUPPORTED)
			return rc;

		if (i >= MAX_CYLINDERS)
			continue;

		o2 = gasmix.oxygen * 1000 + 0.5;
		he = gasmix.helium * 1000 + 0.5;

		/* Ignore bogus data - libdivecomputer does some crazy stuff */
		if (o2 <= AIR_PERMILLE || o2 >= 1000)
			o2 = 0;
		if (he < 0 || he >= 800 || o2+he >= 1000)
			he = 0;

		dive->cylinder[i].gasmix.o2.permille = o2;
		dive->cylinder[i].gasmix.he.permille = he;
	}
	return PARSER_STATUS_SUCCESS;
}

static void handle_event(struct dive *dive, struct sample *sample, parser_sample_value_t value)
{
	int type, time;
	static const char *events[] = {
		"none", "deco", "rbt", "ascent", "ceiling", "workload", "transmitter",
		"violation", "bookmark", "surface", "safety stop", "gaschange",
		"safety stop (voluntary)", "safety stop (mandatory)", "deepstop",
		"ceiling (safety stop)", "unknown", "divetime", "maxdepth",
		"OLF", "PO2", "airtime", "rgbm", "heading", "tissue level warning"
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
	name = "invalid event number";
	if (type < nr_events)
		name = events[type];

	time = value.event.time;
	if (sample)
		time += sample->time.seconds;

	add_event(dive, time, type, value.event.flags, value.event.value, name);
}

void
sample_cb(parser_sample_type_t type, parser_sample_value_t value, void *userdata)
{
	int i;
	struct dive **divep = userdata;
	struct dive *dive = *divep;
	struct sample *sample;

	/*
	 * We fill in the "previous" sample - except for SAMPLE_TYPE_TIME,
	 * which creates a new one.
	 */
	sample = dive->samples ? dive->sample+dive->samples-1 : NULL;

	switch (type) {
	case SAMPLE_TYPE_TIME:
		sample = prepare_sample(divep);
		sample->time.seconds = value.time;
		finish_sample(*divep);
		break;
	case SAMPLE_TYPE_DEPTH:
		sample->depth.mm = value.depth * 1000 + 0.5;
		break;
	case SAMPLE_TYPE_PRESSURE:
		sample->cylinderindex = value.pressure.tank;
		sample->cylinderpressure.mbar = value.pressure.value * 1000 + 0.5;
		break;
	case SAMPLE_TYPE_TEMPERATURE:
		sample->temperature.mkelvin = (value.temperature + 273.15) * 1000 + 0.5;
		break;
	case SAMPLE_TYPE_EVENT:
		handle_event(dive, sample, value);
		break;
	case SAMPLE_TYPE_RBT:
		printf("   <rbt>%u</rbt>\n", value.rbt);
		break;
	case SAMPLE_TYPE_HEARTBEAT:
		printf("   <heartbeat>%u</heartbeat>\n", value.heartbeat);
		break;
	case SAMPLE_TYPE_BEARING:
		printf("   <bearing>%u</bearing>\n", value.bearing);
		break;
	case SAMPLE_TYPE_VENDOR:
		printf("   <vendor type=\"%u\" size=\"%u\">", value.vendor.type, value.vendor.size);
		for (i = 0; i < value.vendor.size; ++i)
			printf("%02X", ((unsigned char *) value.vendor.data)[i]);
		printf("</vendor>\n");
		break;
	default:
		break;
	}
}


static int parse_samples(struct dive **divep, parser_t *parser)
{
	// Parse the sample data.
	printf("Parsing the sample data.\n");
	return parser_samples_foreach(parser, sample_cb, divep);
}

/*
 * Check if this dive already existed before the import
 */
static int find_dive(struct dive *dive, device_data_t *devdata)
{
	int i;

	for (i = 0; i < dive_table.preexisting; i++) {
		struct dive *old = dive_table.dives[i];

		if (dive->when != old->when)
			continue;
		return 1;
	}
	return 0;
}

static int dive_cb(const unsigned char *data, unsigned int size,
	const unsigned char *fingerprint, unsigned int fsize,
	void *userdata)
{
	int rc;
	parser_t *parser = NULL;
	device_data_t *devdata = userdata;
	dc_datetime_t dt = {0};
	struct tm tm;
	struct dive *dive;

	rc = create_parser(devdata, &parser);
	if (rc != PARSER_STATUS_SUCCESS) {
		fprintf(stderr, "Unable to create parser for %s", devdata->name);
		return rc;
	}

	rc = parser_set_data(parser, data, size);
	if (rc != PARSER_STATUS_SUCCESS) {
		fprintf(stderr, "Error registering the data.");
		parser_destroy(parser);
		return rc;
	}

	dive = alloc_dive();
	rc = parser_get_datetime(parser, &dt);
	if (rc != PARSER_STATUS_SUCCESS && rc != PARSER_STATUS_UNSUPPORTED) {
		fprintf(stderr, "Error parsing the datetime.");
		parser_destroy (parser);
		return rc;
	}

	tm.tm_year = dt.year;
	tm.tm_mon = dt.month-1;
	tm.tm_mday = dt.day;
	tm.tm_hour = dt.hour;
	tm.tm_min = dt.minute;
	tm.tm_sec = dt.second;
	dive->when = utc_mktime(&tm);

	// Parse the divetime.
	printf("Parsing the divetime.\n");
	unsigned int divetime = 0;
	rc = parser_get_field (parser, FIELD_TYPE_DIVETIME, 0, &divetime);
	if (rc != PARSER_STATUS_SUCCESS && rc != PARSER_STATUS_UNSUPPORTED) {
		fprintf(stderr, "Error parsing the divetime.");
		parser_destroy(parser);
		return rc;
	}
	dive->duration.seconds = divetime;

	// Parse the maxdepth.
	printf("Parsing the maxdepth.\n");
	double maxdepth = 0.0;
	rc = parser_get_field(parser, FIELD_TYPE_MAXDEPTH, 0, &maxdepth);
	if (rc != PARSER_STATUS_SUCCESS && rc != PARSER_STATUS_UNSUPPORTED) {
		fprintf(stderr, "Error parsing the maxdepth.");
		parser_destroy(parser);
		return rc;
	}
	dive->maxdepth.mm = maxdepth * 1000 + 0.5;

	// Parse the gas mixes.
	printf("Parsing the gas mixes.\n");
	unsigned int ngases = 0;
	rc = parser_get_field(parser, FIELD_TYPE_GASMIX_COUNT, 0, &ngases);
	if (rc != PARSER_STATUS_SUCCESS && rc != PARSER_STATUS_UNSUPPORTED) {
		fprintf(stderr, "Error parsing the gas mix count.");
		parser_destroy(parser);
		return rc;
	}

	rc = parse_gasmixes(dive, parser, ngases);
	if (rc != PARSER_STATUS_SUCCESS) {
		fprintf(stderr, "Error parsing the gas mix.");
		parser_destroy(parser);
		return rc;
	}

	// Initialize the sample data.
	rc = parse_samples(&dive, parser);
	if (rc != PARSER_STATUS_SUCCESS) {
		fprintf(stderr, "Error parsing the samples.");
		parser_destroy(parser);
		return rc;
	}

	parser_destroy(parser);

	/* If we already saw this dive, abort. */
	if (find_dive(dive, devdata))
		return 0;

	record_dive(dive);
	return 1;
}


static device_status_t import_device_data(device_t *device, device_data_t *devicedata)
{
	return device_foreach(device, dive_cb, devicedata);
}

static device_status_t device_open(const char *devname,
	device_type_t type,
	device_t **device)
{
	switch (type) {
	case DEVICE_TYPE_SUUNTO_SOLUTION:
		return suunto_solution_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_EON:
		return suunto_eon_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_VYPER:
		return suunto_vyper_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_VYPER2:
		return suunto_vyper2_device_open(device, devname);

	case DEVICE_TYPE_SUUNTO_D9:
		return suunto_d9_device_open(device, devname);

	case DEVICE_TYPE_UWATEC_ALADIN:
		return uwatec_aladin_device_open(device, devname);

	case DEVICE_TYPE_UWATEC_MEMOMOUSE:
		return uwatec_memomouse_device_open(device, devname);

	case DEVICE_TYPE_UWATEC_SMART:
		return uwatec_smart_device_open(device);

	case DEVICE_TYPE_REEFNET_SENSUS:
		return reefnet_sensus_device_open(device, devname);

	case DEVICE_TYPE_REEFNET_SENSUSPRO:
		return reefnet_sensuspro_device_open(device, devname);

	case DEVICE_TYPE_REEFNET_SENSUSULTRA:
		return reefnet_sensusultra_device_open(device, devname);

	case DEVICE_TYPE_OCEANIC_VTPRO:
		return oceanic_vtpro_device_open(device, devname);

	case DEVICE_TYPE_OCEANIC_VEO250:
		return oceanic_veo250_device_open(device, devname);

	case DEVICE_TYPE_OCEANIC_ATOM2:
		return oceanic_atom2_device_open(device, devname);

	case DEVICE_TYPE_MARES_DARWIN:
		return mares_darwin_device_open(device, devname, 0); /// last parameter is model type (taken from example), 0 seems to be standard, 1 is DARWIN_AIR => Darwin Air wont work if this is fixed here?

	case DEVICE_TYPE_MARES_NEMO:
		return mares_nemo_device_open(device, devname);

	case DEVICE_TYPE_MARES_PUCK:
		return mares_puck_device_open(device, devname);

	case DEVICE_TYPE_MARES_ICONHD:
		return mares_iconhd_device_open(device, devname);

	case DEVICE_TYPE_HW_OSTC:
		return hw_ostc_device_open(device, devname);

	case DEVICE_TYPE_CRESSI_EDY:
		return cressi_edy_device_open(device, devname);

	case DEVICE_TYPE_ZEAGLE_N2ITION3:
		return zeagle_n2ition3_device_open(device, devname);

	case DEVICE_TYPE_ATOMICS_COBALT:
		return atomics_cobalt_device_open(device);

	default:
		return DEVICE_STATUS_ERROR;
	}
}

static void event_cb(device_t *device, device_event_t event, const void *data, void *userdata)
{
	const device_progress_t *progress = data;
	const device_devinfo_t *devinfo = data;
	const device_clock_t *clock = data;
	device_data_t *devdata = userdata;

	switch (event) {
	case DEVICE_EVENT_WAITING:
		printf("Event: waiting for user action\n");
		break;
	case DEVICE_EVENT_PROGRESS:
		update_progressbar(&devdata->progress,
			(double) progress->current / (double) progress->maximum);
		break;
	case DEVICE_EVENT_DEVINFO:
		devdata->devinfo = *devinfo;
		printf("Event: model=%u (0x%08x), firmware=%u (0x%08x), serial=%u (0x%08x)\n",
			devinfo->model, devinfo->model,
			devinfo->firmware, devinfo->firmware,
			devinfo->serial, devinfo->serial);
		break;
	case DEVICE_EVENT_CLOCK:
		devdata->clock = *clock;
		printf("Event: systime=%"PRId64", devtime=%u\n",
			(uint64_t)clock->systime, clock->devtime);
		break;
	default:
		break;
	}
}

static int import_thread_done = 0, import_thread_cancelled;

static int
cancel_cb(void *userdata)
{
	return import_thread_cancelled;
}

static const char *do_libdivecomputer_import(device_data_t *data)
{
	device_t *device = NULL;
	device_status_t rc;

	rc = device_open(data->devname, data->type, &device);
	if (rc != DEVICE_STATUS_SUCCESS)
		return "Unable to open %s (%s)";

	// Register the event handler.
	int events = DEVICE_EVENT_WAITING | DEVICE_EVENT_PROGRESS | DEVICE_EVENT_DEVINFO | DEVICE_EVENT_CLOCK;
	rc = device_set_events(device, events, event_cb, data);
	if (rc != DEVICE_STATUS_SUCCESS) {
		device_close(device);
		return "Error registering the event handler.";
	}

	// Register the cancellation handler.
	rc = device_set_cancel(device, cancel_cb, data);
	if (rc != DEVICE_STATUS_SUCCESS) {
		device_close(device);
		return "Error registering the cancellation handler.";
	}

	rc = import_device_data(device, data);
	if (rc != DEVICE_STATUS_SUCCESS) {
		device_close(device);
		return "Dive data import error";
	}

	device_close(device);
	return NULL;
}

static void *pthread_wrapper(void *_data)
{
	device_data_t *data = _data;
	const char *err_string = do_libdivecomputer_import(data);
	import_thread_done = 1;
	return (void *)err_string;
}

GError *do_import(device_data_t *data)
{
	pthread_t pthread;
	void *retval;

	/* I'm sure there is some better interface for waiting on a thread in a UI main loop */
	import_thread_done = 0;
	pthread_create(&pthread, NULL, pthread_wrapper, data);
	while (!import_thread_done) {
		import_thread_cancelled = process_ui_events();
		usleep(100000);
	}
	if (pthread_join(pthread, &retval) < 0)
		retval = "Odd pthread error return";
	if (retval)
		return error(retval, data->name, data->devname);
	return NULL;
}

/*
 * Taken from 'example.c' in libdivecomputer.
 *
 * I really wish there was some way to just have
 * libdivecomputer tell us what devices it supports,
 * rather than have the application have to know..
 */
struct device_list device_list[] = {
	{ "Suunto Solution",	DEVICE_TYPE_SUUNTO_SOLUTION },
	{ "Suunto Eon",		DEVICE_TYPE_SUUNTO_EON },
	{ "Suunto Vyper",	DEVICE_TYPE_SUUNTO_VYPER },
	{ "Suunto Vyper Air",	DEVICE_TYPE_SUUNTO_VYPER2 },
	{ "Suunto D9",		DEVICE_TYPE_SUUNTO_D9 },
	{ "Uwatec Aladin",	DEVICE_TYPE_UWATEC_ALADIN },
	{ "Uwatec Memo Mouse",	DEVICE_TYPE_UWATEC_MEMOMOUSE },
	{ "Uwatec Smart",	DEVICE_TYPE_UWATEC_SMART },
	{ "ReefNet Sensus",	DEVICE_TYPE_REEFNET_SENSUS },
	{ "ReefNet Sensus Pro",	DEVICE_TYPE_REEFNET_SENSUSPRO },
	{ "ReefNet Sensus Ultra",DEVICE_TYPE_REEFNET_SENSUSULTRA },
	{ "Oceanic VT Pro",	DEVICE_TYPE_OCEANIC_VTPRO },
	{ "Oceanic Veo250",	DEVICE_TYPE_OCEANIC_VEO250 },
	{ "Oceanic Atom 2",	DEVICE_TYPE_OCEANIC_ATOM2 },
	{ "Mares Darwin, M1, M2, Airlab",	DEVICE_TYPE_MARES_DARWIN },
	{ "Mares Nemo, Excel, Apneist",	DEVICE_TYPE_MARES_NEMO },
	{ "Mares Puck, Nemo Air, Nemo Wide",	DEVICE_TYPE_MARES_PUCK },
	{ "Mares Icon HD",	DEVICE_TYPE_MARES_ICONHD },
	{ "OSTC",		DEVICE_TYPE_HW_OSTC },
#ifdef LIBDIVECOMPUTER_SUPPORTS_FROG
	{ "OSTC Frog",		DEVICE_TYPE_HW_FROG },
#endif
	{ "Cressi Edy",		DEVICE_TYPE_CRESSI_EDY },
	{ "Zeagle N2iTiON 3",	DEVICE_TYPE_ZEAGLE_N2ITION3 },
	{ "Atomics Cobalt",	DEVICE_TYPE_ATOMICS_COBALT },
	{ NULL }
};
