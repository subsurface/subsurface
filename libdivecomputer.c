#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <inttypes.h>
#include <glib/gi18n.h>

#include "dive.h"
#include "device.h"
#include "divelist.h"
#include "display.h"
#include "display-gtk.h"

#include "libdivecomputer.h"
#include "libdivecomputer/version.h"

/* Christ. Libdivecomputer has the worst configuration system ever. */
#ifdef HW_FROG_H
  #define NOT_FROG , 0
  #define LIBDIVECOMPUTER_SUPPORTS_FROG
#else
  #define NOT_FROG
#endif

static const char *progress_bar_text = "";
static double progress_bar_fraction = 0.0;
static int stoptime, stopdepth, ndl, po2, cns;
static gboolean in_deco, first_temp_is_air;

#if USE_GTK_UI
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
#endif

static dc_status_t create_parser(device_data_t *devdata, dc_parser_t **parser)
{
	return dc_parser_new(parser, devdata->device);
}

/* Atomics Aquatics Cobalt specific parsing of tank information
 * realistically this REALLY needs to be done in libdivecomputer - but the
 * current API doesn't even have the notion of tank size, so for now I do
 * this here, but I need to work with Jef to make sure this gets added in
 * the new libdivecomputer API */
#define COBALT_HEADER 228
struct atomics_gas_info {
	uint8_t gas_nr;
	uint8_t po2imit;
	uint8_t tankspecmethod;	/* 1: CF@psi 2: CF@bar 3: wet vol in deciliter */
	uint8_t gasmixtype;
	uint8_t fo2;
	uint8_t fhe;
	uint16_t startpressure;	/* in psi */
	uint16_t tanksize;	/* CF or dl */
	uint16_t workingpressure;
	uint16_t sensorid;
	uint16_t endpressure;	/* in psi */
	uint16_t totalconsumption;	/* in liters */
};
#define COBALT_CFATPSI 1
#define COBALT_CFATBAR 2
#define COBALT_WETINDL 3

static void get_tanksize(device_data_t *devdata, const unsigned char *data, cylinder_t *cyl, int idx)
{
	/* I don't like this kind of match... I'd love to have an ID and
	 * a firmware version or... something; and even better, just get
	 * this from libdivecomputer */
	if (!strcmp(devdata->vendor, "Atomic Aquatics") &&
	    !strcmp(devdata->product, "Cobalt")) {
		struct atomics_gas_info *atomics_gas_info;
		double airvolume;
		int mbar;

		/* at least some quick sanity check to make sure this is the
		 * right data */
		if (*(uint32_t *)data != 0xFFFEFFFE) {
			printf("incorrect header for Atomics dive\n");
			return;
		}
		atomics_gas_info = (void*)(data + COBALT_HEADER);
		switch (atomics_gas_info[idx].tankspecmethod) {
		case COBALT_CFATPSI:
			airvolume = cuft_to_l(atomics_gas_info[idx].tanksize) * 1000.0;
			mbar = psi_to_mbar(atomics_gas_info[idx].workingpressure);
			cyl[idx].type.size.mliter = airvolume / bar_to_atm(mbar / 1000.0) + 0.5;
			cyl[idx].type.workingpressure.mbar = mbar;
			break;
		case COBALT_CFATBAR:
			airvolume = cuft_to_l(atomics_gas_info[idx].tanksize) * 1000.0;
			mbar = atomics_gas_info[idx].workingpressure * 1000;
			cyl[idx].type.size.mliter = airvolume / bar_to_atm(mbar / 1000.0) + 0.5;
			cyl[idx].type.workingpressure.mbar = mbar;
			break;
		case COBALT_WETINDL:
			cyl[idx].type.size.mliter = atomics_gas_info[idx].tanksize * 100;
			break;
		}
	}
}

static int parse_gasmixes(device_data_t *devdata, struct dive *dive, dc_parser_t *parser, int ngases,
	const unsigned char *data)
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
		if (o2 + he <= O2_IN_AIR || o2 >= 1000)
			o2 = 0;
		if (he < 0 || he >= 800 || o2+he >= 1000)
			he = 0;

		dive->cylinder[i].gasmix.o2.permille = o2;
		dive->cylinder[i].gasmix.he.permille = he;

		get_tanksize(devdata, data, dive->cylinder, i);
	}
	return DC_STATUS_SUCCESS;
}

static void handle_event(struct divecomputer *dc, struct sample *sample, dc_sample_value_t value)
{
	int type, time;
	/* we mark these for translation here, but we store the untranslated strings
	 * and only translate them when they are displayed on screen */
	static const char *events[] = {
		N_("none"), N_("deco stop"), N_("rbt"), N_("ascent"), N_("ceiling"), N_("workload"),
		N_("transmitter"), N_("violation"), N_("bookmark"), N_("surface"), N_("safety stop"),
		N_("gaschange"), N_("safety stop (voluntary)"), N_("safety stop (mandatory)"),
		N_("deepstop"), N_("ceiling (safety stop)"), N_("unknown"), N_("divetime"),
		N_("maxdepth"), N_("OLF"), N_("PO2"), N_("airtime"), N_("rgbm"), N_("heading"),
		N_("tissue level warning"), N_("gaschange"), N_("non stop time")
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
	name = N_("invalid event number");
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
	int i;
	struct divecomputer *dc = userdata;
	struct sample *sample;

	/*
	 * We fill in the "previous" sample - except for DC_SAMPLE_TIME,
	 * which creates a new one.
	 */
	sample = dc->samples ? dc->sample+dc->samples-1 : NULL;

	switch (type) {
	case DC_SAMPLE_TIME:
		if (sample) {
			sample->in_deco = in_deco;
			sample->ndl.seconds = ndl;
			sample->stoptime.seconds = stoptime;
			sample->stopdepth.mm = stopdepth;
			sample->po2 = po2;
			sample->cns = cns;
		}
		sample = prepare_sample(dc);
		sample->time.seconds = value.time;
		finish_sample(dc);
		break;
	case DC_SAMPLE_DEPTH:
		sample->depth.mm = value.depth * 1000 + 0.5;
		break;
	case DC_SAMPLE_PRESSURE:
		sample->sensor = value.pressure.tank;
		sample->cylinderpressure.mbar = value.pressure.value * 1000 + 0.5;
		break;
	case DC_SAMPLE_TEMPERATURE:
		sample->temperature.mkelvin = value.temperature * 1000 + ZERO_C_IN_MKELVIN + 0.5;
		break;
	case DC_SAMPLE_EVENT:
		handle_event(dc, sample, value);
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
		printf("   <vendor time='%u:%02u' type=\"%u\" size=\"%u\">", FRACTION(sample->time.seconds, 60),
			value.vendor.type, value.vendor.size);
		for (i = 0; i < value.vendor.size; ++i)
			printf("%02X", ((unsigned char *) value.vendor.data)[i]);
		printf("</vendor>\n");
		break;
#if DC_VERSION_CHECK(0, 3, 0)
	case DC_SAMPLE_SETPOINT:
		/* for us a setpoint means constant pO2 from here */
		sample->po2 = po2 = value.setpoint * 1000 + 0.5;
		break;
	case DC_SAMPLE_PPO2:
		sample->po2 = po2 = value.ppo2 * 1000 + 0.5;
		break;
	case DC_SAMPLE_CNS:
		sample->cns = cns = value.cns * 100 + 0.5;
		break;
	case DC_SAMPLE_DECO:
		if (value.deco.type == DC_DECO_NDL) {
			sample->ndl.seconds = ndl = value.deco.time;
			sample->stopdepth.mm = stopdepth = value.deco.depth * 1000.0 + 0.5;
			sample->in_deco = in_deco = FALSE;
		} else if (value.deco.type == DC_DECO_DECOSTOP ||
			   value.deco.type == DC_DECO_DEEPSTOP) {
			sample->in_deco = in_deco = TRUE;
			sample->stopdepth.mm = stopdepth = value.deco.depth * 1000.0 + 0.5;
			sample->stoptime.seconds = stoptime = value.deco.time;
			ndl = 0;
		} else if (value.deco.type == DC_DECO_SAFETYSTOP) {
			sample->in_deco = in_deco = FALSE;
			sample->stopdepth.mm = stopdepth = value.deco.depth * 1000.0 + 0.5;
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
	vsnprintf(buf, sizeof(buf)-1, fmt, args);
	va_end(args);
	buf[sizeof(buf)-1] = 0;
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

	/* reset the deco / ndl data */
	ndl = stoptime = stopdepth = 0;
	in_deco = FALSE;

	rc = create_parser(devdata, &parser);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, _("Unable to create parser for %s %s"), devdata->vendor, devdata->product);
		return rc;
	}

	rc = dc_parser_set_data(parser, data, size);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, _("Error registering the data"));
		dc_parser_destroy(parser);
		return rc;
	}

	import_dive_number++;
	dive = alloc_dive();
	rc = dc_parser_get_datetime(parser, &dt);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, _("Error parsing the datetime"));
		dc_parser_destroy(parser);
		return rc;
	}
	dive->dc.model = strdup(devdata->model);
	dive->dc.deviceid = devdata->deviceid;
	dive->dc.diveid = calculate_diveid(fingerprint, fsize);

	tm.tm_year = dt.year;
	tm.tm_mon = dt.month-1;
	tm.tm_mday = dt.day;
	tm.tm_hour = dt.hour;
	tm.tm_min = dt.minute;
	tm.tm_sec = dt.second;
	dive->when = dive->dc.when = utc_mktime(&tm);

	// Parse the divetime.
	dev_info(devdata, _("Dive %d: %s %d %04d"), import_dive_number,
		monthname(tm.tm_mon), tm.tm_mday, year(tm.tm_year));
	unsigned int divetime = 0;
	rc = dc_parser_get_field (parser, DC_FIELD_DIVETIME, 0, &divetime);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, _("Error parsing the divetime"));
		dc_parser_destroy(parser);
		return rc;
	}
	dive->dc.duration.seconds = divetime;

	// Parse the maxdepth.
	double maxdepth = 0.0;
	rc = dc_parser_get_field(parser, DC_FIELD_MAXDEPTH, 0, &maxdepth);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, _("Error parsing the maxdepth"));
		dc_parser_destroy(parser);
		return rc;
	}
	dive->dc.maxdepth.mm = maxdepth * 1000 + 0.5;

	// Parse the gas mixes.
	unsigned int ngases = 0;
	rc = dc_parser_get_field(parser, DC_FIELD_GASMIX_COUNT, 0, &ngases);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, _("Error parsing the gas mix count"));
		dc_parser_destroy(parser);
		return rc;
	}

#ifdef DC_FIELD_SALINITY
	// Check if the libdivecomputer version already supports salinity
	double salinity = 1.03;
	rc = dc_parser_get_field(parser, DC_FIELD_SALINITY, 0, &salinity);
	if (rc != DC_STATUS_SUCCESS && rc != DC_STATUS_UNSUPPORTED) {
		dev_info(devdata, _("Error obtaining water salinity"));
		dc_parser_destroy(parser);
		return rc;
	}
	dive->salinity = salinity * 10000.0 + 0.5;
#endif

	rc = parse_gasmixes(devdata, dive, parser, ngases, data);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, _("Error parsing the gas mix"));
		dc_parser_destroy(parser);
		return rc;
	}

	// Initialize the sample data.
	rc = parse_samples(devdata, &dive->dc, parser);
	if (rc != DC_STATUS_SUCCESS) {
		dev_info(devdata, _("Error parsing the samples"));
		dc_parser_destroy(parser);
		return rc;
	}

	dc_parser_destroy(parser);

	/* If we already saw this dive, abort. */
	if (!devdata->force_download && find_dive(&dive->dc))
		return 0;

	/* Various libdivecomputer interface fixups */
	if (first_temp_is_air && dive->dc.samples) {
		dive->dc.airtemp = dive->dc.sample[0].temperature;
		dive->dc.sample[0].temperature.mkelvin = 0;
	}

	dive->downloaded = TRUE;
	record_dive(dive);
	mark_divelist_changed(TRUE);
	return 1;
}


static dc_status_t import_device_data(dc_device_t *device, device_data_t *devicedata)
{
	return dc_device_foreach(device, dive_cb, devicedata);
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
	b0 = serial % 100; serial /= 100;
	b1 = serial % 100; serial /= 100;
	b2 = serial % 100; serial /= 100;
	b3 = serial % 100; serial /= 100;

	serial = b0 + (b1 << 8) + (b2 << 16) + (b3 << 24);
	return serial;
}

static unsigned int fixup_suunto_versions(device_data_t *devdata, const dc_event_devinfo_t *devinfo)
{
	struct device_info *info;
	unsigned int serial = devinfo->serial;

	first_temp_is_air = 1;

	serial = undo_libdivecomputer_suunto_nr_changes(serial);

	info = create_device_info(devdata->model, devdata->deviceid);
	if (!info)
		return serial;

	if (!info->serial_nr && serial) {
		char serial_nr[13];

		snprintf(serial_nr, sizeof(serial_nr), "%02d%02d%02d%02d",
			(devinfo->serial >> 24) & 0xff,
			(devinfo->serial >> 16) & 0xff,
			(devinfo->serial >> 8)  & 0xff,
			(devinfo->serial >> 0)  & 0xff);
		info->serial_nr = strdup(serial_nr);
	}

	if (!info->firmware && devinfo->firmware) {
		char firmware[13];
		snprintf(firmware, sizeof(firmware), "%d.%d.%d",
			(devinfo->firmware >> 16) & 0xff,
			(devinfo->firmware >> 8)  & 0xff,
			(devinfo->firmware >> 0)  & 0xff);
		info->firmware = strdup(firmware);
	}
	return serial;
}

static void event_cb(dc_device_t *device, dc_event_type_t event, const void *data, void *userdata)
{
	const dc_event_progress_t *progress = data;
	const dc_event_devinfo_t *devinfo = data;
	const dc_event_clock_t *clock = data;
	device_data_t *devdata = userdata;
	unsigned int serial;

	switch (event) {
	case DC_EVENT_WAITING:
		dev_info(devdata, _("Event: waiting for user action"));
		break;
	case DC_EVENT_PROGRESS:
		if (!progress->maximum)
			break;
		progress_bar_fraction = (double) progress->current / (double) progress->maximum;
		break;
	case DC_EVENT_DEVINFO:
		dev_info(devdata, _("model=%u (0x%08x), firmware=%u (0x%08x), serial=%u (0x%08x)"),
			devinfo->model, devinfo->model,
			devinfo->firmware, devinfo->firmware,
			devinfo->serial, devinfo->serial);
		/*
		 * libdivecomputer doesn't give serial numbers in the proper string form,
		 * so we have to see if we can do some vendor-specific munging.
		 */
		serial = devinfo->serial;
		if (!strcmp(devdata->vendor, "Suunto"))
			serial = fixup_suunto_versions(devdata, devinfo);
		devdata->deviceid = calculate_sha1(devinfo->model, devinfo->firmware, serial);

		break;
	case DC_EVENT_CLOCK:
			dev_info(devdata, _("Event: systime=%"PRId64", devtime=%u\n"),
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

static const char *do_device_import(device_data_t *data)
{
	dc_status_t rc;
	dc_device_t *device = data->device;

	data->model = str_printf("%s %s", data->vendor, data->product);

	// Register the event handler.
	int events = DC_EVENT_WAITING | DC_EVENT_PROGRESS | DC_EVENT_DEVINFO | DC_EVENT_CLOCK;
	rc = dc_device_set_events(device, events, event_cb, data);
	if (rc != DC_STATUS_SUCCESS)
		return _("Error registering the event handler.");

	// Register the cancellation handler.
	rc = dc_device_set_cancel(device, cancel_cb, data);
	if (rc != DC_STATUS_SUCCESS)
		return _("Error registering the cancellation handler.");

	rc = import_device_data(device, data);
	if (rc != DC_STATUS_SUCCESS)
		return _("Dive data import error");

	/* All good */
	return NULL;
}

static const char *do_libdivecomputer_import(device_data_t *data)
{
	dc_status_t rc;
	const char *err;

	import_dive_number = 0;
	first_temp_is_air = 0;
	data->device = NULL;
	data->context = NULL;

	rc = dc_context_new(&data->context);
	if (rc != DC_STATUS_SUCCESS)
		return _("Unable to create libdivecomputer context");

	err = _("Unable to open %s %s (%s)");
	rc = dc_device_open(&data->device, data->context, data->descriptor, data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		err = do_device_import(data);
		dc_device_close(data->device);
	}
	dc_context_free(data->context);
	return err;
}

#if USE_GTK_UI
static void *pthread_wrapper(void *_data)
{
	device_data_t *data = _data;
	const char *err_string = do_libdivecomputer_import(data);
	import_thread_done = 1;
	return (void *)err_string;
}

/* this simply ends the dialog without a response and asks not to be fired again
 * as we set this function up in every loop while uemis_download is waiting for
 * the download to finish */
static gboolean timeout_func(gpointer _data)
{
	GtkDialog *dialog = _data;
	if (!import_thread_cancelled)
		gtk_dialog_response(dialog, GTK_RESPONSE_NONE);
	return FALSE;
}

GError *do_import(device_data_t *data)
{
	pthread_t pthread;
	void *retval;
	GtkDialog *dialog = data->dialog;

	/* I'm sure there is some better interface for waiting on a thread in a UI main loop */
	import_thread_done = 0;
	progress_bar_text = "";
	progress_bar_fraction = 0.0;
	pthread_create(&pthread, NULL, pthread_wrapper, data);
	/* loop here until the import is done or was cancelled by the user;
	 * in order to get control back from gtk we register a timeout function
	 * that ends the dialog with no response every 100ms; we then update the
	 * progressbar and setup the timeout again - unless of course the user
	 * pressed cancel, in which case we just wait for the download thread
	 * to react to that and exit */
	while (!import_thread_done) {
		if (!import_thread_cancelled) {
			int result;
			g_timeout_add(100, timeout_func, dialog);
			update_progressbar(&data->progress, progress_bar_fraction);
			update_progressbar_text(&data->progress, progress_bar_text);
			result = gtk_dialog_run(dialog);
			switch (result) {
			case GTK_RESPONSE_CANCEL:
				import_thread_cancelled = TRUE;
				progress_bar_text = _("Cancelled...");
				break;
			default:
				/* nothing */
				break;
			}
		} else {
			update_progressbar(&data->progress, progress_bar_fraction);
			update_progressbar_text(&data->progress, progress_bar_text);
			usleep(100000);
		}
	}
	if (pthread_join(pthread, &retval) < 0)
		retval = _("Odd pthread error return");
	if (retval)
		return error(retval, data->vendor, data->product, data->devname);
	return NULL;
}
#endif
