// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "save-fit.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <vector>

#include "dive.h"
#include "divecomputer.h"
#include "membuffer.h"
#include "sample.h"
#include "subsurface-time.h"

/*
 * Hand-written Garmin-family FIT encoder.
 *
 * All wire-format facts below (CRC table, header/definition/data message
 * layout, base type bytes, global message numbers, field numbers and enum
 * values) are sourced only from the permissively-licensed tormoder/fit and
 * muktihari/fit Go SDKs, plus the in-tree GPL libdivecomputer Garmin parser
 * (libdivecomputer/src/garmin_parser.c) used to round-trip-verify this
 * encoder. None of this comes from the Garmin FIT SDK.
 */

namespace {

/*
 * Dynastream FIT CRC-16: a reflected nibble-table algorithm, NOT a generic
 * CCITT CRC-16. Table and per-byte update from the tormoder/fit Go SDK.
 */
constexpr uint16_t fit_crc_table[16] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

uint16_t fit_crc16_byte(uint16_t crc, uint8_t byte)
{
	uint16_t tmp = fit_crc_table[crc & 0xF];
	crc = (crc >> 4) & 0x0FFF;
	crc = crc ^ tmp ^ fit_crc_table[byte & 0xF];
	tmp = fit_crc_table[crc & 0xF];
	crc = (crc >> 4) & 0x0FFF;
	crc = crc ^ tmp ^ fit_crc_table[(byte >> 4) & 0xF];
	return crc;
}

uint16_t fit_crc16(const unsigned char *data, size_t len)
{
	uint16_t crc = 0;
	for (size_t i = 0; i < len; i++)
		crc = fit_crc16_byte(crc, (uint8_t)data[i]);
	return crc;
}

void put_u8(struct membuffer *b, uint8_t v)
{
	put_bytes(b, (const char *)&v, 1);
}

void put_s8(struct membuffer *b, int8_t v)
{
	put_u8(b, (uint8_t)v);
}

void put_u16le(struct membuffer *b, uint16_t v)
{
	unsigned char buf[2] = { (unsigned char)(v & 0xFF), (unsigned char)((v >> 8) & 0xFF) };
	put_bytes(b, (const char *)buf, 2);
}

void put_u32le(struct membuffer *b, uint32_t v)
{
	unsigned char buf[4] = {
		(unsigned char)(v & 0xFF), (unsigned char)((v >> 8) & 0xFF),
		(unsigned char)((v >> 16) & 0xFF), (unsigned char)((v >> 24) & 0xFF)
	};
	put_bytes(b, (const char *)buf, 4);
}

// FIT DateTime epoch: seconds since 1989-12-31 00:00:00 UTC.
constexpr uint32_t FIT_EPOCH_OFFSET = 631065600u;

uint32_t fit_timestamp(timestamp_t unix_ts)
{
	return (uint32_t)(unix_ts - (timestamp_t)FIT_EPOCH_OFFSET);
}

// FIT base type bytes (tormoder/fit base_type table).
constexpr uint8_t FIT_ENUM = 0x00;
constexpr uint8_t FIT_SINT8 = 0x01;
constexpr uint8_t FIT_UINT8 = 0x02;
constexpr uint8_t FIT_UINT16 = 0x84;
constexpr uint8_t FIT_UINT32 = 0x86;
constexpr uint8_t FIT_STRING = 0x07;
constexpr uint8_t FIT_UINT32Z = 0x8C;

// FIT "invalid value" sentinels for the base types used above.
constexpr uint16_t FIT_UINT16_INVALID = 0xFFFF;
constexpr uint32_t FIT_UINT32_INVALID = 0xFFFFFFFFu;
constexpr int8_t FIT_SINT8_INVALID = 0x7F;

/*
 * Global FIT message numbers. Cross-verified against
 * libdivecomputer/src/garmin_parser.c's message_array[] (the authoritative
 * in-tree decoder this encoder is round-trip-tested against), not the
 * Garmin FIT SDK.
 */
constexpr uint16_t MSG_FILE_ID = 0;
constexpr uint16_t MSG_SPORT = 12;
constexpr uint16_t MSG_SESSION = 18;
constexpr uint16_t MSG_LAP = 19;
constexpr uint16_t MSG_RECORD = 20;
constexpr uint16_t MSG_DEVICE_INFO = 23;
constexpr uint16_t MSG_ACTIVITY = 34;

enum local_type_t : uint8_t {
	LOCAL_FILE_ID = 0,
	LOCAL_SPORT = 1,
	LOCAL_DEVICE_INFO = 2,
	LOCAL_RECORD = 3,
	LOCAL_LAP = 4,
	LOCAL_SESSION = 5,
	LOCAL_ACTIVITY = 6,
};

// Enum values from the (non-Garmin) Go SDK FIT profile.
constexpr uint8_t FIT_FILE_TYPE_ACTIVITY = 4;
constexpr uint8_t FIT_SPORT_DIVING = 53;
constexpr uint8_t FIT_SUBSPORT_SINGLE_GAS_DIVING = 55;
constexpr uint8_t FIT_ACTIVITY_MANUAL = 0;
constexpr uint8_t FIT_EVENT_ACTIVITY = 26;
constexpr uint8_t FIT_EVENT_TYPE_STOP = 1;

struct field_def {
	uint8_t num;
	uint8_t size;
	uint8_t base_type;
};

void put_definition(struct membuffer *b, uint8_t local_type, uint16_t global_msg, const std::vector<field_def> &fields)
{
	put_u8(b, 0x40 | local_type);
	put_u8(b, 0x00); // reserved
	put_u8(b, 0x00); // architecture: little-endian
	put_u16le(b, global_msg);
	put_u8(b, (uint8_t)fields.size());
	for (const field_def &f : fields) {
		put_u8(b, f.num);
		put_u8(b, f.size);
		put_u8(b, f.base_type);
	}
}

void put_data_header(struct membuffer *b, uint8_t local_type)
{
	put_u8(b, local_type & 0x0F);
}

/* One point of the 1 Hz resampled record stream. */
struct fit_point {
	int rel = 0;			// seconds since dive.when
	int32_t depth_mm = 0;
	bool has_temp = false;
	int32_t temp_degc = 0;
	bool has_ndl = false;
	int32_t ndl_s = 0;
	bool has_tts = false;
	int32_t tts_s = 0;
};

/*
 * Resample the dive-computer sample series to 1 Hz: depth is linearly
 * interpolated between the bracketing native samples, while temperature/
 * ndl/tts are carried forward from the last known reading -- mirroring the
 * validated reference implementation (ssrf2fit.py's resample()/_interp()).
 */
std::vector<fit_point> resample_1hz(const std::vector<struct sample> &samples)
{
	std::vector<fit_point> out;
	if (samples.empty())
		return out;

	const int t0 = samples.front().time.seconds;
	const int tN = samples.back().time.seconds;

	size_t lo = 0;
	bool has_temp = false, has_ndl = false, has_tts = false;
	int32_t temp_degc = 0, ndl_s = 0, tts_s = 0;

	auto absorb = [&](size_t i) {
		if (samples[i].temperature.mkelvin) {
			has_temp = true;
			temp_degc = (int32_t)lround(samples[i].temperature.mkelvin / 1000.0 - 273.15);
		}
		if (samples[i].ndl.seconds >= 0) {
			has_ndl = true;
			ndl_s = samples[i].ndl.seconds;
		}
		if (samples[i].tts.seconds > 0) {
			has_tts = true;
			tts_s = samples[i].tts.seconds;
		}
	};
	absorb(0);

	out.reserve((size_t)std::max(0, tN - t0) + 1);
	for (int t = t0; t <= tN; t++) {
		while (lo + 1 < samples.size() && samples[lo + 1].time.seconds <= t) {
			lo++;
			absorb(lo);
		}
		size_t hi = std::min(lo + 1, samples.size() - 1);

		fit_point p;
		p.rel = t;
		p.has_temp = has_temp;
		p.temp_degc = temp_degc;
		p.has_ndl = has_ndl;
		p.ndl_s = ndl_s;
		p.has_tts = has_tts;
		p.tts_s = tts_s;

		int t_lo = samples[lo].time.seconds;
		int t_hi = samples[hi].time.seconds;
		if (hi == lo || t_hi <= t_lo) {
			p.depth_mm = samples[lo].depth.mm;
		} else {
			double f = double(t - t_lo) / double(t_hi - t_lo);
			p.depth_mm = (int32_t)lround(samples[lo].depth.mm + f * (samples[hi].depth.mm - samples[lo].depth.mm));
		}

		out.push_back(p);
	}
	return out;
}

} // namespace

uint16_t fit_manufacturer_id(const std::string &vendor)
{
	std::string v;
	v.reserve(vendor.size());
	for (char c : vendor)
		v.push_back((char)std::tolower((unsigned char)c));

	// Ids from the reference ssrf2fit.py tool (verified working against a
	// real Mimo import) and the muktihari/fit Go SDK manufacturer table --
	// not the Garmin FIT SDK. Most Subsurface-supported vendors have
	// no FIT manufacturer id; report that honestly rather than guessing.
	if (v == "garmin")
		return 1;
	if (v == "suunto")
		return 23;
	if (v == "shearwater")
		return 87;
	return FIT_UINT16_INVALID;
}

int fit_resolve_tz_offset(const struct divecomputer &dc)
{
	// The exported .fit's embedded timestamps are plain epoch seconds with no
	// zone info, so "timezone offset" here only ever shifts what wall-clock
	// time those timestamps decode to elsewhere (e.g. in DJI Mimo). Defaulting
	// to 0 keeps the export's absolute time equal to dive.when, which is what
	// every consumer other than a dive computer with a wrong internal clock
	// wants; pre-filling dc.timezone_offset or the device's local UTC offset
	// exported CEST dives hours off. The dc argument
	// and tz_offset_seconds knob in save_fit_to_buffer still exist so a user can
	// manually dial in a correction for a dive computer whose own clock was set
	// wrong -- they just no longer apply automatically.
	(void)dc;
	return 0;
}

int save_fit_to_buffer(const struct dive &dive, int tz_offset_seconds, uint16_t manufacturer_id, struct membuffer *b)
{
	const struct divecomputer *dc = dive.get_dc(0);
	if (!dc || dc->samples.empty())
		return 1;

	std::vector<fit_point> points = resample_1hz(dc->samples);
	if (points.empty())
		return 1;

	// Keep the tz offset applied in exactly one place: here, where the
	// absolute unix time for each emitted point is derived.
	auto point_ts = [&](const fit_point &p) {
		return fit_timestamp(dive.when + tz_offset_seconds + p.rel);
	};

	const uint32_t start_ts = point_ts(points.front());
	const uint32_t end_ts = point_ts(points.back());
	const uint32_t elapsed_ms = (uint32_t)(points.back().rel - points.front().rel) * 1000u;

	const std::string product_name = dc->model.empty() ? "Subsurface" : dc->model.substr(0, 19);

	membuffer messages;

	// file_id (global 0)
	put_definition(&messages, LOCAL_FILE_ID, MSG_FILE_ID, {
		{0, 1, FIT_ENUM},	// type
		{1, 2, FIT_UINT16},	// manufacturer
		{2, 2, FIT_UINT16},	// product
		{3, 4, FIT_UINT32Z},	// serial_number
		{4, 4, FIT_UINT32},	// time_created
	});
	put_data_header(&messages, LOCAL_FILE_ID);
	put_u8(&messages, FIT_FILE_TYPE_ACTIVITY);
	put_u16le(&messages, manufacturer_id);
	put_u16le(&messages, FIT_UINT16_INVALID); // product: unknown, don't fabricate
	put_u32le(&messages, (uint32_t)dc->deviceid);
	put_u32le(&messages, start_ts);

	// sport (global 12): the Garmin parser keys dive detection off this
	// message's sub_sport field (SET_FIELD(SPORT, 1, sub_sport, ENUM) in
	// garmin_parser.c), not off a SESSION field.
	put_definition(&messages, LOCAL_SPORT, MSG_SPORT, {
		{0, 1, FIT_ENUM},	// sport
		{1, 1, FIT_ENUM},	// sub_sport
	});
	put_data_header(&messages, LOCAL_SPORT);
	put_u8(&messages, FIT_SPORT_DIVING);
	put_u8(&messages, FIT_SUBSPORT_SINGLE_GAS_DIVING);

	// device_info (global 23)
	put_definition(&messages, LOCAL_DEVICE_INFO, MSG_DEVICE_INFO, {
		{253, 4, FIT_UINT32},				// timestamp
		{0, 1, FIT_UINT8},				// device_index
		{2, 2, FIT_UINT16},				// manufacturer
		{4, 2, FIT_UINT16},				// product
		{3, 4, FIT_UINT32Z},				// serial_number
		{27, (uint8_t)(product_name.size() + 1), FIT_STRING},	// product_name (NUL-terminated)
	});
	put_data_header(&messages, LOCAL_DEVICE_INFO);
	put_u32le(&messages, start_ts);
	put_u8(&messages, 0); // device_index: creator
	put_u16le(&messages, manufacturer_id);
	put_u16le(&messages, FIT_UINT16_INVALID); // product: unknown, don't fabricate
	put_u32le(&messages, (uint32_t)dc->deviceid);
	put_bytes(&messages, product_name.c_str(), (int)product_name.size() + 1);

	// record (global 20): one definition, then one data message per
	// resampled point. ndl/tts are only added to the definition (and thus
	// only ever emitted) when the dive computer actually reported them, so
	// every data message stays consistent with its most recent definition.
	const bool has_ndl = std::any_of(points.begin(), points.end(), [](const fit_point &p) { return p.has_ndl; });
	const bool has_tts = std::any_of(points.begin(), points.end(), [](const fit_point &p) { return p.has_tts; });
	std::vector<field_def> record_fields = {
		{253, 4, FIT_UINT32},	// timestamp
		{92, 4, FIT_UINT32},	// depth (mm)
		{13, 1, FIT_SINT8},	// temperature (degC)
	};
	if (has_ndl)
		record_fields.push_back({96, 4, FIT_UINT32}); // ndl (s)
	if (has_tts)
		record_fields.push_back({95, 4, FIT_UINT32}); // tts (s)
	put_definition(&messages, LOCAL_RECORD, MSG_RECORD, record_fields);
	for (const fit_point &p : points) {
		put_data_header(&messages, LOCAL_RECORD);
		put_u32le(&messages, point_ts(p));
		put_u32le(&messages, (uint32_t)p.depth_mm);
		put_s8(&messages, p.has_temp ? (int8_t)p.temp_degc : FIT_SINT8_INVALID);
		if (has_ndl)
			put_u32le(&messages, p.has_ndl ? (uint32_t)p.ndl_s : FIT_UINT32_INVALID);
		if (has_tts)
			put_u32le(&messages, p.has_tts ? (uint32_t)p.tts_s : FIT_UINT32_INVALID);
	}

	// lap (global 19)
	put_definition(&messages, LOCAL_LAP, MSG_LAP, {
		{254, 2, FIT_UINT16},	// message_index
		{253, 4, FIT_UINT32},	// timestamp
		{2, 4, FIT_UINT32},	// start_time
		{7, 4, FIT_UINT32},	// total_elapsed_time (scale 1000)
		{8, 4, FIT_UINT32},	// total_timer_time (scale 1000)
	});
	put_data_header(&messages, LOCAL_LAP);
	put_u16le(&messages, 0);
	put_u32le(&messages, end_ts);
	put_u32le(&messages, start_ts);
	put_u32le(&messages, elapsed_ms);
	put_u32le(&messages, elapsed_ms);

	// session (global 18). CRITICAL: start_time (field 2) MUST equal the
	// first record's FIT timestamp, and every record timestamp MUST be >=
	// start_time, or the Garmin parser drops every sample ("Timestamp
	// before dive start", garmin_parser.c:469-489,507).
	put_definition(&messages, LOCAL_SESSION, MSG_SESSION, {
		{254, 2, FIT_UINT16},	// message_index
		{253, 4, FIT_UINT32},	// timestamp
		{2, 4, FIT_UINT32},	// start_time
		{5, 1, FIT_ENUM},	// sport (spec-complete; not decoded by garmin_parser.c)
		{6, 1, FIT_ENUM},	// sub_sport (spec-complete; not decoded by garmin_parser.c)
		{7, 4, FIT_UINT32},	// total_elapsed_time (scale 1000)
		{8, 4, FIT_UINT32},	// total_timer_time (scale 1000)
		{25, 2, FIT_UINT16},	// first_lap_index
		{26, 2, FIT_UINT16},	// num_laps
	});
	put_data_header(&messages, LOCAL_SESSION);
	put_u16le(&messages, 0);
	put_u32le(&messages, end_ts);
	put_u32le(&messages, start_ts);
	put_u8(&messages, FIT_SPORT_DIVING);
	put_u8(&messages, FIT_SUBSPORT_SINGLE_GAS_DIVING);
	put_u32le(&messages, elapsed_ms);
	put_u32le(&messages, elapsed_ms);
	put_u16le(&messages, 0);
	put_u16le(&messages, 1);

	// activity (global 34)
	put_definition(&messages, LOCAL_ACTIVITY, MSG_ACTIVITY, {
		{253, 4, FIT_UINT32},	// timestamp
		{0, 4, FIT_UINT32},	// total_timer_time (scale 1000)
		{1, 2, FIT_UINT16},	// num_sessions
		{2, 1, FIT_ENUM},	// type
		{3, 1, FIT_ENUM},	// event
		{4, 1, FIT_ENUM},	// event_type
	});
	put_data_header(&messages, LOCAL_ACTIVITY);
	put_u32le(&messages, end_ts);
	put_u32le(&messages, elapsed_ms);
	put_u16le(&messages, 1);
	put_u8(&messages, FIT_ACTIVITY_MANUAL);
	put_u8(&messages, FIT_EVENT_ACTIVITY);
	put_u8(&messages, FIT_EVENT_TYPE_STOP);

	// Assemble: 14-byte header (with header CRC) + messages + trailing file CRC.
	unsigned char header[14];
	header[0] = 14;		// header size
	header[1] = 0x10;	// protocol version
	header[2] = 0x5C;	// profile version 2140 LE, low byte
	header[3] = 0x08;	// profile version 2140 LE, high byte
	uint32_t data_size = messages.len;
	header[4] = (unsigned char)(data_size & 0xFF);
	header[5] = (unsigned char)((data_size >> 8) & 0xFF);
	header[6] = (unsigned char)((data_size >> 16) & 0xFF);
	header[7] = (unsigned char)((data_size >> 24) & 0xFF);
	header[8] = '.';
	header[9] = 'F';
	header[10] = 'I';
	header[11] = 'T';
	uint16_t header_crc = fit_crc16(header, 12);
	header[12] = (unsigned char)(header_crc & 0xFF);
	header[13] = (unsigned char)((header_crc >> 8) & 0xFF);

	put_bytes(b, (const char *)header, 14);
	put_bytes(b, messages.buffer, messages.len);

	uint16_t file_crc = fit_crc16((const unsigned char *)b->buffer, b->len);
	put_u16le(b, file_crc);

	return 0;
}
