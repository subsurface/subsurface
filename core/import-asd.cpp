// SPDX-License-Identifier: GPL-2.0

/* .asd file format is the dive import/export format for Scubapro/Uwatec software
 * divelogs. It enables export from SmartTrak and import/export from LogTrak.
 * It's a binary format with remembrances to ancient DataTrak format in the way
 * strings are stored, and it keeps the samples from the DC as they are, so we can
 * simply pass them to libdivecomputer to parse.
 * The bad news:
 * - Dives aren't fixed size, thus we can't avoid a sequential parsing.
 * - There is a header area for each dive but this header is not the same
 *   libdc expects as the expected headers are different based on DC model or family.
 *   Thus faking a header as it's expected by libdc is a must. This has to be done per
 *   dive, as a single file may have dives downloaded from different DCs.
 * - The byte sequence signaling the beginning of a new dive varies depending
 *   on the origin of the file (SmartTrak or LogTrak).
 * - Files coming from SmartTrak can include dives imported from yet older
 *   DataTrak software. I don't know if these dives coming from serial devices can
 *   even be imported into LogTrak. ATM we can safely assume we are only finding
 *   them in SmartTrak exported dives.
 * - ... Probably some more I just haven't found yet.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>

#include "dive.h"
#include "device.h"
#include "errorhelper.h"
#include "gettext.h"
#include "divelist.h"
#include "file.h"
#include "libdivecomputer.h"
#include "divesite.h"
#include "equipment.h"
#include "divelog.h"
#include "tag.h"

extern dc_status_t dt_libdc_buffer(unsigned char *ptr, int prf_length, int dc_model, unsigned char *compl_buffer);

/*
 * dc model definitions
 */
#define SMARTPRO          0x10
#define GALILEO           0x11
#define ALADINTEC         0x12
#define ALADINTEC2G       0x13
#define SMARTCOM          0x14
#define ALADIN2G          0x15
#define ALADINSPORTMATRIX 0x17
#define SMARTTEC          0x18
#define GALILEOTRIMIX     0x19
#define SMARTZ            0x1C
#define MERIDIAN          0x20
#define ALADINSQUARE      0x22
#define CHROMIS           0x24
#define ALADINA1          0x25
#define MANTIS2           0x26
#define ALADINA2          0x28
#define G2TEK             0x31
#define G2                0x32
#define G3                0x34
#define G2HUD             0x42
#define LUNA2AI           0x50
#define LUNA2             0x51

/*
 * Data positions in serial stream as expected by libdc
 */
#define LIBDC_MAX_DEPTH			22
#define LIBDC_DIVE_TIME			26
#define LIBDC_MAX_TEMP			28
#define LIBDC_MIN_TEMP			30
#define LIBDC_SURF_TEMP			32
#define LIBDC_GASMIX			44
#define LIBDC_TANK_PRESS		50
#define LIBDC_SETTINGS			92
#define LIBDC_MAX_DEPTH_SMART		18
#define LIBDC_DIVE_TIME_SMART		20
#define LIBDC_MIN_TEMP_SMART		22
#define LIBDC_GASMIX_SMART		24
#define LIBDC_GASMIX_SMARTZ		28
#define LIBDC_TANK_PRESS_SMARTZ		34
#define LIBDC_TANK_PRESS_SMARTCOM	30
#define LIBDC_DIVE_TIME_ALADINTEC	24
#define LIBDC_SETTINGS_ALADINTEC	52
#define LIBDC_MIN_TEMP_ALADINTEC	26
#define LIBDC_GASMIX_ALADINTEC		30
#define LIBDC_GASMIX_ALADINTEC2G	34
#define LIBDC_SETTINGS_ALADINTEC2G	60
#define LIBDC_SETTINGS_TRIMIX		68

#define LIBDC_SAMPLES_MANTIS		152
#define LIBDC_SAMPLES_G2		84
#define LIBDC_SAMPLES_ALADINTEC		108
#define LIBDC_SAMPLES_ALADINTEC2G	116
#define LIBDC_SAMPLES_SMART		92
#define LIBDC_SAMPLES_SMARTCOM		100
#define LIBDC_SAMPLES_SMARTZ		132

/*
 * Data positions in asd raw data buffer passed to build_dc_data().
 * There are 12 more bytes which include 2bytes for dc model and 4bytes for
 * device serial number.
 */
#define ASD_SAMPLES		183
#define ASD_DIVE_TIME		32
#define ASD_MAX_TEMP		21
#define ASD_MIN_TEMP		34
#define ASD_SURF_TEMP		18
#define ASD_GAS_MIX		36	// 2 bytes. Should be x3 at least. Assume consecutive.
#define ASD_MAXDEPTH		30
#define ASD_SETTINGS		70	// Get 4 bytes although only 2 are really needed
#define ASD_TANK_PRESS_INIT	42
#define ASD_TANK_PRESS_END	44

#define G2_MAX_TEMP		148

/*
 * Returns a dc_descriptor_t structure based on dc  model's number.
 * This ensures the model pased to libdc_buffer_parser() is a supported model and avoids
 * problems with shared model num devices by taking the family into account.
 * AFAIK only DC_FAMILY_UWATEC_SMART is avaliable in libdc; UWATEC_MERIDIAN kept just in case ...
 */
extern "C" dc_descriptor_t *get_data_descriptor(int data_model, dc_family_t data_fam)
{
	dc_descriptor_t *descriptor = NULL, *current = NULL;
	dc_iterator_t *iterator = NULL;
	dc_status_t rc;

	rc = dc_descriptor_iterator(&iterator);
	if (rc != DC_STATUS_SUCCESS) {
		report_error("[libdc]\t\t\tCreating the device descriptor iterator.\n");
		return current;
	}
	while ((dc_iterator_next(iterator, &descriptor)) == DC_STATUS_SUCCESS) {
		int desc_model = dc_descriptor_get_model(descriptor);
		dc_family_t desc_fam = dc_descriptor_get_type(descriptor);

		if (data_model == desc_model && (data_fam == desc_fam)) {
			current = descriptor;
			break;
		}
		dc_descriptor_free(descriptor);
	}
	dc_iterator_free(iterator);
	return current;
}

/*
 * Fills a device_data_t structure to pass to libdivecomputer.
 * Detects if a device is supported or not.
 */
extern "C" int prepare_data(int data_model, dc_family_t dc_fam, device_data_t *dev_data)
{
	dev_data->device = NULL;
	dev_data->context = NULL;
	dev_data->descriptor = get_data_descriptor(data_model, dc_fam);
	if (dev_data->descriptor) {
		dev_data->vendor = dc_descriptor_get_vendor(dev_data->descriptor);
		dev_data->product = dc_descriptor_get_product(dev_data->descriptor);
		std::string tmp (dev_data->vendor);
		tmp += " ";
		tmp += dev_data->product;
		dev_data->model = tmp;
		return DC_STATUS_SUCCESS;
	} else {
		report_info("Warning [prepare_data]: Unsupported DC model 0x%2x\n", data_model);
		return DC_STATUS_UNSUPPORTED;
	}
}

/*
 * ASD provides the size of the buffer ... well, most times. Sometimes it's not the full buffer
 * size, but just the samples size. I've been unable to find a pattern for this (although samples
 * size seems to be limited to galileo devices coming from SmartTrak exported dives). Thus, we can't
 * trust this data and we need to calculate the correct buffer size, this being the previously found
 * size of the ASD dive data minus the ASD header plus the device header.
 * The default option in the switch conditional shouldn't be reached. If so, we have forgotten to add
 * some device.
 */
extern "C" unsigned char *allocate_libdc_buffer(int model, int asd_size, int *size)
{
	unsigned char *buf;
	int sample_size = asd_size - ASD_SAMPLES;

	switch (model) {
		case GALILEO:
		case ALADIN2G:
		case MERIDIAN:
		case CHROMIS:
		case MANTIS2:
		case ALADINSQUARE:
			*size = sample_size + LIBDC_SAMPLES_MANTIS;
			break;
		case GALILEOTRIMIX:
		case G2:
		case G2HUD:
		case G2TEK:
		case G3:
		case ALADINSPORTMATRIX:
		case ALADINA1:
		case ALADINA2:
		case LUNA2AI:
		case LUNA2:
			*size = sample_size + LIBDC_SAMPLES_G2;
			break;
		case ALADINTEC2G:
			*size = sample_size + LIBDC_SAMPLES_ALADINTEC2G;
			break;
		case ALADINTEC:
			*size = sample_size + LIBDC_SAMPLES_ALADINTEC;
			break;
		case SMARTPRO:
			*size = sample_size + LIBDC_SAMPLES_SMART;
			break;
		case SMARTCOM:
			*size = sample_size + LIBDC_SAMPLES_SMARTCOM;
			break;
		default:
			report_error("[allocate_libdc_buffer]: Unsupported model 0x%.2X\n", model);
			return NULL;
	}
	buf = (unsigned char *)calloc(*size, 1);
	if (!buf)
		report_error("[allocate_libdc_buffer]: Failed to place enough memory for libdc buffer. %d bytes.\n", *size);
	return buf;
}

/*
 * This function returns an allocated memory buffer with the completed dc data.
 * The buffer has to be padded in the beginning with the header libdivecomputer expects to find,
 * this is,  a5a55a5a following two bytes with the buffer size.
 * BTW  we need to manually relocate those parts of the header needed by libdivecomputer. The
 * rest of the buffer is filled with the samples.
 * For older serial Aladin DCs, just use dt_libdc_buffer() from datatrak.cpp importer.
 */
extern "C" unsigned char *build_dc_data(int model, dc_family_t dc_fam, char *input, int max, int *out_size)
{
	unsigned char *ptr = (unsigned char*) input;
	unsigned char *buffer;
	const unsigned char head_begin[] = {0xa5, 0xa5, 0x5a, 0x5a};
	int buf_size = 0;

	// Older serial Aladin profile imported to
	// SmarTrak from Datatrak
	if (dc_fam == DC_FAMILY_UWATEC_ALADIN) {
		*out_size = max + 18;
		buffer = (unsigned char *)calloc(*out_size, 1);
		dt_libdc_buffer(ptr, max, model, buffer);
		return buffer;
	}

	buffer = allocate_libdc_buffer(model, max, &buf_size);
	if (!buffer)
		return NULL; // we are OOM or with unknown DC
	*out_size = buf_size;
	memcpy(buffer, &head_begin, 4);		// place header begining
	buffer[4] = buf_size & 0xFF;		// calculated buffer size
	buffer[5] = buf_size >> 8;
	memcpy(buffer + 6, ptr + 2, 11);	// initial block (unchanged)

	switch (model) {
	case GALILEO:				// untested with LogTrak native dives and somehow faulty
	case GALILEOTRIMIX:
		memcpy(buffer + LIBDC_MAX_DEPTH, ptr + ASD_MAXDEPTH, 2);
		memcpy(buffer + LIBDC_DIVE_TIME, ptr + ASD_DIVE_TIME, 2);
		memcpy(buffer + LIBDC_MAX_TEMP, ptr + ASD_MAX_TEMP, 2);
		memcpy(buffer + LIBDC_MIN_TEMP, ptr + ASD_MIN_TEMP, 2);
		memcpy(buffer + LIBDC_SURF_TEMP, ptr + ASD_SURF_TEMP, 2);
		memcpy(buffer + LIBDC_GASMIX, ptr + ASD_GAS_MIX, 2);
		memcpy(buffer + LIBDC_TANK_PRESS, ptr + ASD_TANK_PRESS_INIT, 2);
		if (model == GALILEOTRIMIX)
			buffer[43] = 0x80;	// libdc checks this byte to apply trimix's parsing model or galileo's
		(model == GALILEO) ? memcpy(buffer + LIBDC_SETTINGS, ptr + ASD_SETTINGS, 4) : memcpy(buffer + LIBDC_SETTINGS_TRIMIX, ptr + ASD_SETTINGS, 4);
		(model == GALILEO) ? memcpy(buffer + LIBDC_SAMPLES_MANTIS, ptr + ASD_SAMPLES, max - ASD_SAMPLES) : memcpy(buffer + LIBDC_SAMPLES_G2, ptr + ASD_SAMPLES, max - ASD_SAMPLES);
		break;
	case ALADINTEC2G:
		memcpy(buffer + LIBDC_MAX_DEPTH_SMART, ptr + ASD_MAXDEPTH, 2);
		memcpy(buffer + LIBDC_DIVE_TIME_SMART, ptr + ASD_DIVE_TIME, 2);
		memcpy(buffer + LIBDC_MAX_TEMP, ptr + ASD_MAX_TEMP, 2);
		memcpy(buffer + LIBDC_MIN_TEMP_ALADINTEC, ptr + ASD_MIN_TEMP, 2);
		memcpy(buffer + LIBDC_SURF_TEMP, ptr + ASD_SURF_TEMP, 2);
		memcpy(buffer + LIBDC_GASMIX_ALADINTEC2G, ptr + ASD_GAS_MIX, 6);
		memcpy(buffer + LIBDC_SETTINGS_ALADINTEC2G, ptr + ASD_SETTINGS, 4);
		memcpy(buffer + LIBDC_SAMPLES_ALADINTEC2G, ptr + ASD_SAMPLES, max - ASD_SAMPLES);
		break;
	case ALADIN2G:				// tested Logtrak native Aladin 2G and Mantis 2
	case MERIDIAN:
	case CHROMIS:
	case MANTIS2:
	case ALADINSQUARE:
		memcpy(buffer + LIBDC_MAX_DEPTH, ptr + ASD_MAXDEPTH, 2);
		memcpy(buffer + LIBDC_DIVE_TIME, ptr + ASD_DIVE_TIME, 2);
		memcpy(buffer + LIBDC_MAX_TEMP, ptr + ASD_MAX_TEMP, 2);
		memcpy(buffer + LIBDC_MIN_TEMP, ptr + ASD_MIN_TEMP, 2);
		memcpy(buffer + LIBDC_SURF_TEMP, ptr + ASD_SURF_TEMP, 2);
		memcpy(buffer + LIBDC_GASMIX, ptr + ASD_GAS_MIX, 2);
		memcpy(buffer + LIBDC_TANK_PRESS, ptr + ASD_TANK_PRESS_INIT, 2);
		memcpy(buffer + LIBDC_SETTINGS, ptr + ASD_SETTINGS, 4);
		memcpy(buffer + LIBDC_SAMPLES_MANTIS, ptr + ASD_SAMPLES, max - ASD_SAMPLES);
		break;
	case G2:				// only G2 tested, bold assumption
	case G2HUD:
	case G2TEK:
	case G3:
	case ALADINSPORTMATRIX:
	case ALADINA1:
	case ALADINA2:
	case LUNA2AI:
	case LUNA2:
		memcpy(buffer + LIBDC_MAX_DEPTH, ptr + ASD_MAXDEPTH, 2);
		memcpy(buffer + LIBDC_DIVE_TIME, ptr + ASD_DIVE_TIME, 2);
		memcpy(buffer + LIBDC_MAX_TEMP, ptr + G2_MAX_TEMP, 2);
		memcpy(buffer + LIBDC_MIN_TEMP, ptr + ASD_MIN_TEMP, 2);
		memcpy(buffer + LIBDC_SURF_TEMP, ptr + ASD_SURF_TEMP, 2);
		memcpy(buffer + LIBDC_SETTINGS_TRIMIX, ptr + ASD_SETTINGS, 4);
		memcpy(buffer + LIBDC_SAMPLES_G2, ptr + ASD_SAMPLES, max - ASD_SAMPLES);
		break;
	case ALADINTEC:
		memcpy(buffer + LIBDC_MAX_DEPTH, ptr + ASD_MAXDEPTH, 2);
		memcpy(buffer + LIBDC_DIVE_TIME_ALADINTEC, ptr + ASD_DIVE_TIME, 2);
		memcpy(buffer + LIBDC_MAX_TEMP, ptr + ASD_SURF_TEMP, 2);
		memcpy(buffer + LIBDC_MIN_TEMP_ALADINTEC, ptr + ASD_MIN_TEMP, 2);
		memcpy(buffer + LIBDC_GASMIX_ALADINTEC, ptr + ASD_GAS_MIX, 2);
		memcpy(buffer + LIBDC_SETTINGS_ALADINTEC, ptr + ASD_SETTINGS, 4);
		memcpy(buffer + LIBDC_SAMPLES_ALADINTEC, ptr + ASD_SAMPLES, max - ASD_SAMPLES);
		break;
	case SMARTPRO:
		memcpy(buffer + LIBDC_MAX_DEPTH_SMART, ptr + ASD_MAXDEPTH, 2);
		memcpy(buffer + LIBDC_DIVE_TIME_SMART, ptr + ASD_DIVE_TIME, 2);
		memcpy(buffer + LIBDC_MIN_TEMP_SMART, ptr + ASD_MIN_TEMP, 2);
		memcpy(buffer + LIBDC_GASMIX_SMART, ptr + ASD_GAS_MIX, 2);
		memcpy(buffer + LIBDC_SAMPLES_SMART, ptr + ASD_SAMPLES, max - ASD_SAMPLES);
		break;
	case SMARTCOM:
		memcpy(buffer + LIBDC_MAX_DEPTH_SMART, ptr + ASD_MAXDEPTH, 2);
		memcpy(buffer + LIBDC_DIVE_TIME_SMART, ptr + ASD_DIVE_TIME, 2);
		memcpy(buffer + LIBDC_MIN_TEMP_SMART, ptr + ASD_MIN_TEMP, 2);
		memcpy(buffer + LIBDC_GASMIX_SMART, ptr + ASD_GAS_MIX, 2);
		memcpy(buffer + LIBDC_TANK_PRESS_SMARTCOM, ptr + ASD_TANK_PRESS_INIT, 4);
		memcpy(buffer + LIBDC_SAMPLES_SMARTCOM, ptr + ASD_SAMPLES, max - ASD_SAMPLES);
		break;
	default:
		report_error("Unsupported DC model 0x%2x\n", model);
		free(buffer);
		buffer = NULL;
	}
	return buffer;
}

/*
 * Search for a given sequence of bytes in a string.
 * Returns the position of the first byte of the sequence or string::npos if
 * not found.
 * Parameter s is the size of the sequence.
 */

std::size_t uchar_find(const std::string &in, const unsigned char *seq, const int s)
{
	return in.find(std::string(reinterpret_cast<const char *>(seq), s));
}

/*
 * Return a utf-8 string from an .asd string.
 * String fields in .asd file begin with a 3 bytes BOM followed by a 1 byte
 * number, e.g.  FF FE FF nn, where nn is the length of string following,
 * in chars (two bytes unicode chars). May be zero, meaning an empty field.
 * If anything fails the pointer to next field will be set to "", signaling
 * a weirdness. The caller must handle it (aborting seems preferable as we can
 * have data corruption issues).
 */
std::string asd_to_string(const std::string &input, std::string &output)
{
	if (input.empty()) {
		output = input;
		return input;
	}

	const auto tmp = reinterpret_cast<const unsigned char *>(input.data()) ;
	const int size = tmp[3] * 2;	// worst case, all chars are 2 bytes long

	// this is not a failure but an empty string
	if (size == 0) {
		output = input.substr(4);
		return "";
	}

	// convert string to UTF-8
	std::vector<char> buffer(size, 0);
	for (int i = 0, j = 0; i < size; i += 2, ++j) {
		const unsigned char c = (tmp[i + 5] << 4) + tmp[i + 4];	// 4 is the BOM size
		if (c <= 0x7F) {
			buffer[j] = c;
		} else {
			buffer[j] = (c >> 6) | 0xC0;
			buffer[++j] = (c & 0x3F) | 0x80;
		}
	}
	output = input.substr(size + 4);

	buffer.erase(std::remove(buffer.begin(), buffer.end(), 0), buffer.end());
	const std::string ret = std::string(buffer.begin(), buffer.end());
	// drop meaningless strings comming from SmartTrak if any
	return ret.compare(" ") && ret.compare("-") && ret.compare("???") && ret.compare("---") ? ret : "";
}

/*
 * Build a dive site with coords and name if it doesn't exist yet and place it in the table.
 */
static void asd_build_dive_site(const std::string &instring, const std::string &coords, struct divelog *log, struct dive_site **asd_site)
{
	struct dive_site *site;
	double gpsX = 0, gpsY = 0;
	location_t gps_loc;

	if (!coords.empty())
		sscanf(coords.data(), "%lf %lf", &gpsX, &gpsY);

	gps_loc = create_location(gpsX, gpsY);
	site = log->sites.get_by_name(instring);
	if (!site) {
		if (!has_location(&gps_loc))
			site = log->sites.create(instring);
		else
			site = log->sites.create(instring, gps_loc);
	}
	*asd_site = site;
}

/*
 * Check if file is an export from SmartTrak divelog.
 * ASD files have a unicode string at the beginning with some info, including
 * the string "SmartTRAK" if generated with this software,
 */
bool is_smtk(const std::string &p)
{
	const unsigned char smart[10] = {0x53,0x00,0x6d,0x00,0x61,0x00,0x72,0x00,0x74,0x00};
	return uchar_find(p, smart, 10) < p.find("CLogEntry");
}

/*
 * A block of buddies or equipment consist of a integer n (2 bytes big endian) followed by
 * n asd strings, n = 0 meaning no strings follow. Caller func passes n (if n > 0) and points
 * to the beginning of strings. Buddies are placed in dive buddies list and equipment in a string,
 * returning a pointer to next byte after the parsed block.
 * These are only used in SmartTrak exported .asd files, as LogTrak doesn't support such fields.
 */
std::string asd_build_others(const std::string &input, int idx, std::string &output)
{
	std::string head;
	std::string ptr = input;
	int i = idx;

	if (input.empty()) {
		output = "";
		return "";
	}

	while (i > 0) {
		std::string tail = asd_to_string(ptr, ptr);
		if (! head.empty())
			head += ", ";
		head += tail;
		i--;
	}
	output = head;
	return ptr;
}

/*
 * Parse a dive in a mem buffer and return 0 for correct ending or negative
 * values signaling an error. -1 would be a recoverable error and -2
 * a weird issue (abort further parsing would be preferred).
 */
static int asd_dive_parser(const std::string &input, struct dive *asd_dive, struct divelog *log)//, const device_table &devices)
{
	int dc_model, s = 0, rc = 0, k, j;
	dc_family_t dc_fam;
	long dc_serial;
	const unsigned char str_seq[] = {0xff, 0xfe, 0xff}, dc_profile_begin[4] = {0x01, 0x00, 0x00, 0xFF};
	unsigned char *dc_data;
	auto devdata = std::make_unique<device_data_t>();
	devdata->log = log;
	asd_dive->dcs[0].serial.resize(64);
	weightsystem_t ws;
	std::string tmp, d_locat, d_point, d_coords, notes, viz, w_type, w_surf, weather, buddies, equipment;
	std::size_t size;

	//input should point to dc model integer
	dc_model = input[0];
	dc_fam = (input[1] > 0x00) ? DC_FAMILY_UWATEC_ALADIN : DC_FAMILY_UWATEC_SMART;
	tmp = input.substr(8);

	dc_serial = (tmp[3] << 24) + (tmp[2] << 16) + (tmp[1] << 8) + tmp[0];

	if (!dc_model) {	// this would be the manual dive case (coming from SmartTrak)
		asd_dive->dcs[0].model = "Manually entered dive";
	} else {
		rc = prepare_data(dc_model, dc_fam, devdata.get());
		if (rc != DC_STATUS_SUCCESS)
			goto bailout;
		asd_dive->dcs[0].model = devdata->model;
		tmp = tmp.substr(4);

		if (dc_fam == DC_FAMILY_UWATEC_ALADIN) {
			std::size_t pos = uchar_find(tmp, dc_profile_begin, 4);
			tmp = tmp.substr(pos + 4);
		}
		size = uchar_find(tmp, str_seq, 3); // size of DC data
		dc_data = build_dc_data(dc_model, dc_fam, tmp.data(), size, &s);
		if (!dc_data)
			goto bailout;

		rc = libdc_buffer_parser(asd_dive, devdata.get(), dc_data, s);
		free(dc_data);
		if (rc != DC_STATUS_SUCCESS)
			goto bailout;
		// set serial in DC info, and set a node for the device.
		asd_dive->dcs[0].serial = std::to_string(dc_serial);
		create_device_node(log->devices, asd_dive->dcs[0].model, asd_dive->dcs[0].serial, asd_dive->dcs[0].model);

		// Now the non DC data fields.
		tmp = tmp.substr(size);
	}

	// There are no string fields merged with string ones, making things a bit more dificult.
	d_locat = asd_to_string(tmp, tmp);
	if (tmp.empty())
		goto buffail;

	d_point = asd_to_string(tmp, tmp);
	if (tmp.empty())
		goto buffail;

	if (!d_point.empty() && d_locat.compare(d_point))
		d_locat += ", " + d_point;
	d_coords = asd_to_string(tmp, tmp);
	if (tmp.empty())
		goto buffail;

	asd_build_dive_site(d_locat, d_coords, log, &asd_dive->dive_site);
	// next two bytes are the tank volume (mililiters) and two following are
	// unknown (always zero, may be a 2º tank).
	asd_dive->get_or_create_cylinder(0)->type.size.mliter = (tmp[1] << 8) + tmp[0];
	tmp = tmp.substr(4);
	asd_dive->get_cylinder(0)->type.description = asd_to_string(tmp, tmp);
	if (tmp.empty())
		goto buffail;

	asd_dive->suit = asd_to_string(tmp, tmp);
	if (tmp.empty())
		goto buffail;

	// next two bytes are the weight in gr. Two following are zeroed.
	ws = { { .grams = (tmp[1] << 8) + tmp[0] }, translate("gettextFromC", "unknown"), false };
	asd_dive->weightsystems.push_back(std::move(ws));
	tmp = tmp.substr(4);

	// weather
	weather = asd_to_string(tmp, tmp);
	if (!weather.empty())
		taglist_add_tag(asd_dive->tags, weather);
	if (tmp.empty())
		goto buffail;

	// water surface
	w_surf = asd_to_string(tmp,tmp);
	if (!w_surf.empty())
		taglist_add_tag(asd_dive->tags, w_surf);
	if (tmp.empty())
		goto buffail;

	// water type (localized string; can't be used for salinity)
	w_type = asd_to_string(tmp, tmp);
	if (!w_type.empty())
		taglist_add_tag(asd_dive->tags, w_type);
	if (tmp.empty())
		goto buffail;

	// visibility (localized string; can't be used for rating viz)
	viz = asd_to_string(tmp, tmp);
	if (!viz.empty())
		taglist_add_tag(asd_dive->tags, viz);
	if (tmp.empty())
		goto buffail;

	tmp = tmp.substr(2);
	// Nº of buddies strings following
	j = (tmp[1] << 8 ) + tmp[0];
	tmp = tmp.substr(2);
	if (j > 0) {
		tmp = asd_build_others(tmp, j, buddies);
		asd_dive->buddy = buddies;
	}
	if (tmp.empty())
		goto buffail;

	// Nº of equipment items strings following
	k = (tmp[1] << 8 ) + tmp[0];
	tmp = tmp.substr(2);
	if (k > 0)
		tmp = asd_build_others(tmp, k, equipment);
	if (tmp.empty())
		goto buffail;

	// Notes if any. Include equipment in notes.
	notes = asd_to_string(tmp, tmp);
	if (! equipment.empty()) {
		if (notes.size() > 0) {
			notes += "\n";
		}
		notes += (translate("gettextFromC", "Equipment: ") + equipment);
	}
	asd_dive->notes = notes;
	if (tmp.empty())
		goto buffail;

	return 0;
bailout:						// give a pointer, we can parse other dives yet
	report_error("Warning [asd_dive_parser]: Non critical error in dive %d. Keep on parsing next dive.\n", asd_dive->number);
	return -1;
buffail:						// quit caller func, something weird is going on
	report_error("[asd_dive_parser()]\tMemory corruption or file damaged. Dive %d\n", asd_dive->number);
	return -2;
}

/*
 * Main function
 */
int scubapro_asd_import(const std::string &mem, struct divelog *log)
{
	std::string runner;
	const char init_seq[4] = {0x07, 0x00, 0x10, 0x00};
	std::string first_dive_seq = {"CLogEntry"};
	unsigned char dive_seq_lt[4] = {0x00, 0x00, 0x80, 0x01};
	unsigned char dive_seq_smtk[4] = {0x00, 0x00, 0x01, 0x80};
	const unsigned char *dive_seq = dive_seq_lt;
	int dive_count = 0;

	setlocale(LC_NUMERIC, "POSIX");
	setlocale(LC_CTYPE, "");

	// check header
	if (! mem.compare(0, 4, init_seq)) {
		report_error("This doesn't look like an .asd file. Please check it.\n");
		return 1;
	}
	// is this .asd coming from SmartTrak ?
	if (is_smtk(mem))
		dive_seq = dive_seq_smtk;

	// Jump to initial dive data secuence
	std::size_t pos = mem.find(first_dive_seq);
	if (pos != std::string::npos) {
		runner = mem.substr(pos + 9);
	} else {
		report_error("This doesn't look like an .asd file. Please check it.\n");
		return 1;
	}

	// We are on the first byte of the first dive (DC model). Subsequent dives in log
	// will begin with  0x80 0x01 or 0x01 0x80 depending on original software.
	while (pos < std::string::npos) {
		auto asd_dive = std::make_unique<dive>();
		dive_count++;
		asd_dive->number = dive_count;
		pos = 0;
		int rc = asd_dive_parser(runner, asd_dive.get(), log);
		switch (rc) {
		case 0: {
			log->dives.record_dive(std::move(asd_dive));
			break;
		}
		case -1: {							// recoverable error
			report_error("Error parsing dive %d\n", dive_count);
			break;
		}
		case -2: {							// no recoverable error
			report_error("Error parsing dive %d\n", dive_count);
			return 1;
		}
		}
		// Look for next dive and jump to it
		pos = uchar_find(runner, dive_seq, 4);
		runner = runner.substr(pos + 4);
	};
	log->dives.sort();
	return 0;
}
