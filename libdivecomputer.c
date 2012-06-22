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

static dc_status_t create_parser(device_data_t *devdata, dc_parser_t **parser)
{
	return dc_parser_new(parser, devdata->device);
}

static int parse_gasmixes(device_data_t *devdata, struct dive *dive, dc_parser_t *parser, int ngases)
{
	int i;

	for (i = 0; i < ngases; i++) {
		int rc;
		dc_gasmix_t gasmix = {0};
		int o2, he;

		rc = dc_parser_get_field(parser, DC_FIELD_GASMIX, i, &gasmix);
		if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED)
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
	return DC_STATUS_SUCCESS;
}

static void handle_event(struct dive *dive, struct sample *sample, dc_sample_value_t value)
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
sample_cb(dc_sample_type_t type, dc_sample_value_t value, void *userdata)
{
	int i;
	struct dive **divep = userdata;
	struct dive *dive = *divep;
	struct sample *sample;

	/*
	 * We fill in the "previous" sample - except for DC_SAMPLE_TIME,
	 * which creates a new one.
	 */
	sample = dive->samples ? dive->sample+dive->samples-1 : NULL;

	switch (type) {
	case DC_SAMPLE_TIME:
		sample = prepare_sample(divep);
		sample->time.seconds = value.time;
		finish_sample(*divep);
		break;
	case DC_SAMPLE_DEPTH:
		sample->depth.mm = value.depth * 1000 + 0.5;
		break;
	case DC_SAMPLE_PRESSURE:
		sample->cylinderindex = value.pressure.tank;
		sample->cylinderpressure.mbar = value.pressure.value * 1000 + 0.5;
		break;
	case DC_SAMPLE_TEMPERATURE:
		sample->temperature.mkelvin = (value.temperature + 273.15) * 1000 + 0.5;
		break;
	case DC_SAMPLE_EVENT:
		handle_event(dive, sample, value);
		break;
	case DC_SAMPLE_RBT:
		printf("   <rbt>%u</rbt>\n", value.rbt);
		break;
	case DC_SAMPLE_HEARTBEAT:
		printf("   <heartbeat>%u</heartbeat>\n", value.heartbeat);
		break;
	case DC_SAMPLE_BEARING:
		printf("   <bearing>%u</bearing>\n", value.bearing);
		break;
	case DC_SAMPLE_VENDOR:
		printf("   <vendor type=\"%u\" size=\"%u\">", value.vendor.type, value.vendor.size);
		for (i = 0; i < value.vendor.size; ++i)
			printf("%02X", ((unsigned char *) value.vendor.data)[i]);
		printf("</vendor>\n");
		break;
	default:
		break;
	}
}

static void dev_info(device_data_t *devdata, const char *fmt, ...)
{
	char buffer[32];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);
	update_progressbar_text(&devdata->progress, buffer);
}

static int import_dive_number = 0;

static int parse_samples(device_data_t *devdata, struct dive **divep, dc_parser_t *parser)
{
	// Parse the sample data.
	return dc_parser_samples_foreach(parser, sample_cb, divep);
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

static inline int year(int year)
{
	if (year < 70)
		return year + 2000;
	if (year < 100)
		return year + 1900;
	return year;
}

static int dive_cb(const unsigned char *data, unsigned int size,
	const unsigned char *fingerprint, unsigned int fsize,
	void *userdata)
{
	int rc;
	dc_parser_t *parser = NULL;
	device_data_t *devdata = userdata;
	dc_datetime_t dt = {0};
	struct tm tm;
	struct dive *dive;

	rc = create_parser(devdata, &parser);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, "Unable to create parser for %s %s", devdata->vendor, devdata->product);
		return rc;
	}

	rc = dc_parser_set_data(parser, data, size);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, "Error registering the data");
		dc_parser_destroy(parser);
		return rc;
	}

	import_dive_number++;
	dive = alloc_dive();
	rc = dc_parser_get_datetime(parser, &dt);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, "Error parsing the datetime");
		dc_parser_destroy(parser);
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
	dev_info(devdata, "Dive %d: %s %d %04d", import_dive_number,
		monthname(tm.tm_mon), tm.tm_mday, year(tm.tm_year));
	unsigned int divetime = 0;
	rc = dc_parser_get_field (parser, DC_FIELD_DIVETIME, 0, &divetime);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, "Error parsing the divetime");
		dc_parser_destroy(parser);
		return rc;
	}
	dive->duration.seconds = divetime;

	// Parse the maxdepth.
	double maxdepth = 0.0;
	rc = dc_parser_get_field(parser, DC_FIELD_MAXDEPTH, 0, &maxdepth);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, "Error parsing the maxdepth");
		dc_parser_destroy(parser);
		return rc;
	}
	dive->maxdepth.mm = maxdepth * 1000 + 0.5;

	// Parse the gas mixes.
	unsigned int ngases = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_GASMIX_COUNT, 0, &ngases);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, "Error parsing the gas mix count");
		dc_parser_destroy(parser);
		return rc;
	}

	rc = parse_gasmixes(devdata, dive, parser, ngases);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, "Error parsing the gas mix");
		dc_parser_destroy(parser);
		return rc;
	}

	// Initialize the sample data.
	rc = parse_samples(devdata, &dive, parser);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, "Error parsing the samples");
		dc_parser_destroy(parser);
		return rc;
	}

	dc_parser_destroy(parser);

	/* If we already saw this dive, abort. */
	if (find_dive(dive, devdata))
		return 0;

	record_dive(dive);
	return 1;
}


static dc_status_t import_device_data(dc_device_t *device, device_data_t *devicedata)
{
	return dc_device_foreach(device, dive_cb, devicedata);
}

static dc_status_t device_open(const char *devname,
	dc_descriptor_t *descriptor,
	dc_device_t **device)
{
	return dc_device_open(device, descriptor, devname);
}


static void event_cb(dc_device_t *device, dc_event_type_t event, const void *data, void *userdata)
{
	const dc_event_progress_t *progress = data;
	const dc_event_devinfo_t *devinfo = data;
	const dc_event_clock_t *clock = data;
	device_data_t *devdata = userdata;

	switch (event) {
	case DC_EVENT_WAITING:
		dev_info(devdata, "Event: waiting for user action");
		break;
	case DC_EVENT_PROGRESS:
		update_progressbar(&devdata->progress,
			(double) progress->current / (double) progress->maximum);
		break;
	case DC_EVENT_DEVINFO:
		dev_info(devdata, "model=%u (0x%08x), firmware=%u (0x%08x), serial=%u (0x%08x)",
			devinfo->model, devinfo->model,
			devinfo->firmware, devinfo->firmware,
			devinfo->serial, devinfo->serial);
		break;
	case DC_EVENT_CLOCK:
		dev_info(devdata, "Event: systime=%"PRId64", devtime=%u\n",
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
	dc_device_t *device = NULL;
	dc_status_t rc;

	import_dive_number = 0;
	rc = device_open(data->devname, data->descriptor, &device);
	if (rc != DC_STATUS_SUCCESS)
		return "Unable to open %s %s (%s)";
	data->device = device;

	// Register the event handler.
	int events = DC_EVENT_WAITING | DC_EVENT_PROGRESS | DC_EVENT_DEVINFO | DC_EVENT_CLOCK;
	rc = dc_device_set_events(device, events, event_cb, data);
	if (rc != DC_STATUS_SUCCESS) {
		dc_device_close(device);
		return "Error registering the event handler.";
	}

	// Register the cancellation handler.
	rc = dc_device_set_cancel(device, cancel_cb, data);
	if (rc != DC_STATUS_SUCCESS) {
		dc_device_close(device);
		return "Error registering the cancellation handler.";
	}

	rc = import_device_data(device, data);
	if (rc != DC_STATUS_SUCCESS) {
		dc_device_close(device);
		return "Dive data import error";
	}

	dc_device_close(device);
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
		return error(retval, data->vendor, data->product, data->devname);
	return NULL;
}
