// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "testfitexport.h"

#include <cstring>

#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/libdivecomputer.h"
#include "core/membuffer.h"
#include "core/sample.h"
#include "core/save-fit.h"

namespace {

/*
 * A representative in-memory dive: five 1 Hz samples with distinct time,
 * depth and temperature values, spaced so the encoder's 1 Hz resampling is
 * a no-op and depth/temperature survive the round trip exactly.
 */
struct dive make_test_dive()
{
	struct dive d;
	d.when = 1700000000;

	struct divecomputer *dc = d.get_dc(0);
	dc->model = "Test Dive Computer";
	dc->deviceid = 0x12345678;

	static const int32_t depths_mm[] = { 0, 5000, 10000, 8000, 2000 };
	static const uint32_t temps_mkelvin[] = { 293150, 291150, 289150, 290150, 292150 }; // 20, 18, 16, 17, 19 C
	for (int i = 0; i < 5; i++) {
		struct sample s;
		s.time.seconds = i;
		s.depth.mm = depths_mm[i];
		s.temperature.mkelvin = temps_mkelvin[i];
		append_sample(s, dc);
	}
	return d;
}

/*
 * Independent reimplementation of the Dynastream FIT CRC-16 (same table as
 * save-fit.cpp's fit_crc16), used to verify the encoder's checksums without
 * depending on its anonymous-namespace internals.
 */
uint16_t golden_crc16(const unsigned char *data, size_t len)
{
	static const uint16_t table[16] = {
		0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
		0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
	};
	uint16_t crc = 0;
	for (size_t i = 0; i < len; i++) {
		uint8_t byte = data[i];
		uint16_t tmp = table[crc & 0xF];
		crc = (crc >> 4) & 0x0FFF;
		crc = crc ^ tmp ^ table[byte & 0xF];
		tmp = table[crc & 0xF];
		crc = (crc >> 4) & 0x0FFF;
		crc = crc ^ tmp ^ table[(byte >> 4) & 0xF];
	}
	return crc;
}

} // namespace

void TestFitExport::testRoundTrip()
{
	struct dive d = make_test_dive();
	const struct divecomputer *dc = d.get_dc(0);

	uint16_t manufacturer_id = fit_manufacturer_id("Garmin");
	QCOMPARE(manufacturer_id, (uint16_t)1);

	membuffer mb;
	int rc = save_fit_to_buffer(d, 0, manufacturer_id, &mb);
	QCOMPARE(rc, 0);
	QVERIFY(mb.len > 0);

	device_data_t devdata;
	QVERIFY(prepare_device_descriptor(0, DC_FAMILY_GARMIN, devdata) != 0);

	struct dive decoded;
	dc_status_t status = libdc_buffer_parser(&decoded, &devdata, (const unsigned char *)mb.buffer, mb.len);
	QCOMPARE(status, DC_STATUS_SUCCESS);

	const struct divecomputer *decoded_dc = decoded.get_dc(0);
	QVERIFY(decoded_dc != nullptr);
	QCOMPARE(decoded_dc->samples.size(), dc->samples.size());

	for (size_t i = 0; i < dc->samples.size(); i++) {
		QCOMPARE(decoded_dc->samples[i].depth.mm, dc->samples[i].depth.mm);
		QCOMPARE(decoded_dc->samples[i].temperature.mkelvin, dc->samples[i].temperature.mkelvin);
	}

	// The Garmin parser (garmin_parser.c's ANY_timestamp handler) makes
	// sample times relative to session.start_time with a fixed +1 offset,
	// so we assert the sequence and spacing rather than the raw values.
	QCOMPARE(decoded_dc->samples[0].time.seconds, 1);
	for (size_t i = 1; i < decoded_dc->samples.size(); i++)
		QCOMPARE(decoded_dc->samples[i].time.seconds - decoded_dc->samples[i - 1].time.seconds, 1);
}

void TestFitExport::testNoSamplesFails()
{
	struct dive d; // default dive has a dive computer with no samples
	membuffer mb;
	int rc = save_fit_to_buffer(d, 0, fit_manufacturer_id("Garmin"), &mb);
	QVERIFY(rc != 0);
	QCOMPARE(mb.len, 0u);
}

void TestFitExport::testManufacturerMapping()
{
	QCOMPARE(fit_manufacturer_id("Garmin"), (uint16_t)1);
	QCOMPARE(fit_manufacturer_id("Suunto"), (uint16_t)23);
	QCOMPARE(fit_manufacturer_id("Shearwater"), (uint16_t)87);

	// Mapping is specified case-insensitive.
	QCOMPARE(fit_manufacturer_id("shearwater"), (uint16_t)87);

	// Vendors with no known FIT manufacturer id must fall back to the FIT
	// "invalid" uint16 sentinel, never a guessed/false vendor.
	QCOMPARE(fit_manufacturer_id("Mares"), (uint16_t)0xFFFF);
	QCOMPARE(fit_manufacturer_id("Cressi"), (uint16_t)0xFFFF);
	QCOMPARE(fit_manufacturer_id(""), (uint16_t)0xFFFF);
	QCOMPARE(fit_manufacturer_id("Oceanic"), (uint16_t)0xFFFF);
}

void TestFitExport::testTzResolution()
{
	struct dive d = make_test_dive();
	struct divecomputer *dc = d.get_dc(0);

	// The default is always 0, regardless of what the DC reports -- FIT
	// timestamps carry no zone, so pre-filling from dc.timezone_offset or the
	// device's local UTC offset produced exports that were hours off.
	dc->timezone_offset = -3 * 3600;
	QCOMPARE(fit_resolve_tz_offset(*dc), 0);

	dc->timezone_offset = TIMEZONE_OFFSET_INVALID;
	QCOMPARE(fit_resolve_tz_offset(*dc), 0);
}

void TestFitExport::testTzOverrideAffectsBytes()
{
	struct dive d = make_test_dive();
	uint16_t manufacturer_id = fit_manufacturer_id("Garmin");

	const int tz1 = 0;
	const int tz2 = 2 * 3600; // +2h, distinct from tz1

	membuffer mb1, mb2;
	QCOMPARE(save_fit_to_buffer(d, tz1, manufacturer_id, &mb1), 0);
	QCOMPARE(save_fit_to_buffer(d, tz2, manufacturer_id, &mb2), 0);

	// Same dive, same manufacturer id -> identical message layout; only the
	// embedded timestamps (and therefore the header/file CRCs) differ.
	QCOMPARE(mb1.len, mb2.len);
	QVERIFY(memcmp(mb1.buffer, mb2.buffer, mb1.len) != 0);

	device_data_t devdata1, devdata2;
	QVERIFY(prepare_device_descriptor(0, DC_FAMILY_GARMIN, devdata1) != 0);
	QVERIFY(prepare_device_descriptor(0, DC_FAMILY_GARMIN, devdata2) != 0);

	struct dive decoded1, decoded2;
	QCOMPARE(libdc_buffer_parser(&decoded1, &devdata1, (const unsigned char *)mb1.buffer, mb1.len), DC_STATUS_SUCCESS);
	QCOMPARE(libdc_buffer_parser(&decoded2, &devdata2, (const unsigned char *)mb2.buffer, mb2.len), DC_STATUS_SUCCESS);

	// Regression guard: with the default offset of 0 the first record's
	// timestamp round-trips to exactly
	// dive.when -- not dive.when shifted by a DC-reported or local UTC
	// offset, which is what previously exported dives up to hours late.
	QCOMPARE(decoded1.when, d.when);

	// FIT timestamps are plain epoch seconds with no embedded zone, so the
	// decoded absolute start time must shift by exactly the tz delta applied
	// at encode time.
	QCOMPARE(decoded2.when - decoded1.when, (timestamp_t)(tz2 - tz1));
}

void TestFitExport::testGoldenByteVectors()
{
	struct dive d = make_test_dive();
	uint16_t manufacturer_id = fit_manufacturer_id("Garmin");

	membuffer mb;
	QCOMPARE(save_fit_to_buffer(d, 0, manufacturer_id, &mb), 0);
	QVERIFY(mb.len > 14 + 2);

	const unsigned char *buf = (const unsigned char *)mb.buffer;
	const uint32_t total_len = (uint32_t)mb.len;

	// -- Header (14 bytes) --
	QCOMPARE(buf[0], (unsigned char)14);   // header size
	QCOMPARE(buf[1], (unsigned char)0x10); // protocol version

	uint32_t data_size = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) |
			     ((uint32_t)buf[6] << 16) | ((uint32_t)buf[7] << 24);
	QCOMPARE(data_size, total_len - 14 - 2);

	QCOMPARE(buf[8], (unsigned char)0x2E);  // '.'
	QCOMPARE(buf[9], (unsigned char)0x46);  // 'F'
	QCOMPARE(buf[10], (unsigned char)0x49); // 'I'
	QCOMPARE(buf[11], (unsigned char)0x54); // 'T'

	uint16_t expected_header_crc = golden_crc16(buf, 12);
	uint16_t actual_header_crc = (uint16_t)buf[12] | ((uint16_t)buf[13] << 8);
	QCOMPARE(actual_header_crc, expected_header_crc);

	// -- Trailing file CRC, over the whole file minus the trailing CRC itself --
	uint16_t expected_file_crc = golden_crc16(buf, total_len - 2);
	uint16_t actual_file_crc = (uint16_t)buf[total_len - 2] | ((uint16_t)buf[total_len - 1] << 8);
	QCOMPARE(actual_file_crc, expected_file_crc);

	// -- First definition message: file_id (global 0), immediately after the
	// 14-byte header per the encoder's fixed message ordering. --
	size_t off = 14;
	while (off < total_len && !(buf[off] & 0x40))
		off++;
	QVERIFY(off < total_len);
	QCOMPARE(buf[off], (unsigned char)0x40); // 0x40 | local_type 0 (file_id)

	static const unsigned char expected_file_id_def[] = {
		0x40,			// definition header: local_type 0
		0x00,			// reserved
		0x00,			// architecture: little-endian
		0x00, 0x00,		// global message number 0 (file_id) LE
		0x05,			// field count
		0x00, 0x01, 0x00,	// type: field 0, size 1, base type enum
		0x01, 0x02, 0x84,	// manufacturer: field 1, size 2, base type uint16
		0x02, 0x02, 0x84,	// product: field 2, size 2, base type uint16
		0x03, 0x04, 0x8C,	// serial_number: field 3, size 4, base type uint32z
		0x04, 0x04, 0x86,	// time_created: field 4, size 4, base type uint32
	};
	QVERIFY(off + sizeof(expected_file_id_def) <= total_len);
	QCOMPARE(memcmp(buf + off, expected_file_id_def, sizeof(expected_file_id_def)), 0);
}

QTEST_GUILESS_MAIN(TestFitExport)
