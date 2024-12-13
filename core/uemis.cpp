// SPDX-License-Identifier: MIT
/*
 * uemis.c
 *
 * UEMIS SDA file importer
 * AUTHOR:  Dirk Hohndel - Copyright 2011
 *
 * Licensed under the MIT license.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gettext.h"

#include "uemis.h"
#include "dive.h"
#include "divecomputer.h"
#include "divesite.h"
#include "errorhelper.h"
#include "format.h"
#include "sample.h"
#include <libdivecomputer/parser.h>
#include <libdivecomputer/version.h>

struct uemis_sample
{
	uint16_t dive_time;
	uint16_t water_pressure;   // (in cbar)
	uint16_t dive_temperature; // (in dC)
	uint8_t ascent_speed;      // (units unclear)
	uint8_t work_fact;
	uint8_t cold_fact;
	uint8_t bubble_fact;
	uint16_t ascent_time;
	uint16_t ascent_time_opt;
	uint16_t p_amb_tol;
	uint16_t satt;
	uint16_t hold_depth;
	uint16_t hold_time;
	uint8_t active_tank;
	// bloody glib, when compiled for Windows, forces the whole program to use
	// the Windows packing rules. So to avoid problems on Windows (and since
	// only tank_pressure is currently used and that exactly once) I give in and
	// make this silly low byte / high byte 8bit entries
	uint8_t tank_pressure_low; // (in cbar)
	uint8_t tank_pressure_high;
	uint8_t consumption_low; // (units unclear)
	uint8_t consumption_high;
	uint8_t rgt; // (remaining gas time in minutes)
	uint8_t cns;
	uint8_t flags[8];
} __attribute((packed));

/*
 * following code is based on code found in at base64.sourceforge.net/b64.c
 * AUTHOR:         Bob Trower 08/04/01
 * COPYRIGHT:      Copyright (c) Trantor Standard Systems Inc., 2001
 * NOTE:           This source code may be used as you wish, subject to
 *                 the MIT license.
 */
/*
 * Translation Table to decode (created by Bob Trower)
 */
static const char cd64[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
 * decodeblock -- decode 4 '6-bit' characters into 3 8-bit binary bytes
 */
static void decodeblock(unsigned char in[4], unsigned char out[3])
{
	out[0] = (unsigned char)(in[0] << 2 | in[1] >> 4);
	out[1] = (unsigned char)(in[1] << 4 | in[2] >> 2);
	out[2] = (unsigned char)(((in[2] << 6) & 0xc0) | in[3]);
}

/*
 * decode a base64 encoded stream discarding padding, line breaks and noise
 */
static void decode(uint8_t *inbuf, uint8_t *outbuf, int inbuf_len)
{
	uint8_t in[4], out[3], v;
	int i, len, indx_in = 0, indx_out = 0;

	while (indx_in < inbuf_len) {
		for (len = 0, i = 0; i < 4 && (indx_in < inbuf_len); i++) {
			v = 0;
			while ((indx_in < inbuf_len) && v == 0) {
				v = inbuf[indx_in++];
				v = ((v < 43 || v > 122) ? 0 : cd64[v - 43]);
				if (v)
					v = ((v == '$') ? 0 : v - 61);
			}
			if (indx_in < inbuf_len) {
				len++;
				if (v)
					in[i] = (v - 1);
			} else
				in[i] = 0;
		}
		if (len) {
			decodeblock(in, out);
			for (i = 0; i < len - 1; i++)
				outbuf[indx_out++] = out[i];
		}
	}
}
/* end code from Bob Trower */

/*
 * convert the base64 data blog
 */
static std::vector<uint8_t> convert_base64(std::string_view base64)
{
	int datalen;
	int len = (int)base64.size();

	datalen = (len / 4 + 1) * 3;
	if (datalen < 0x123 + 0x25)
		/* less than header + 1 sample??? */
		report_info("suspiciously short data block %d", datalen);

	std::vector<uint8_t> res(datalen);
	decode((unsigned char *)base64.begin(), res.data(), len);

	if (memcmp(res.data(), "Dive\01\00\00", 7))
		report_info("Missing Dive100 header");

	return res;
}

struct uemis::helper &uemis::get_helper(uint32_t diveid)
{
	return helper_table[diveid];
}

void uemis::weight_unit(int diveid, int lbs)
{
	struct uemis::helper &hp = get_helper(diveid);
	hp.lbs = lbs;
}

int uemis::get_weight_unit(uint32_t diveid) const
{
	auto it = helper_table.find(diveid);
	return it != helper_table.end() ? it->second.lbs : 0;
}

void uemis::mark_divelocation(int diveid, int divespot, struct dive_site *ds)
{
	struct uemis::helper &hp = get_helper(diveid);
	hp.divespot = divespot;
	hp.dive_site = ds;
}

/* support finding a dive spot based on the diveid */
int uemis::get_divespot_id_by_diveid(uint32_t diveid) const
{
	auto it = helper_table.find(diveid);
	return it != helper_table.end() ? it->second.divespot : -1;
}

void uemis::set_divelocation(int divespot, const std::string &text, double longitude, double latitude)
{
	for (auto it: helper_table) {
		if (it.second.divespot == divespot) {
			struct dive_site *ds = it.second.dive_site;
			if (ds) {
				ds->name = text;
				ds->location = create_location(latitude, longitude);
			}
		}
	}
}

/* Create events from the flag bits and other data in the sample;
 * These bits basically represent what is displayed on screen at sample time.
 * Many of these 'warnings' are way hyper-active and seriously clutter the
 * profile plot - so these are disabled by default
 *
 * we mark all the strings for translation, but we store the untranslated
 * strings and only convert them when displaying them on screen - this way
 * when we write them to the XML file we'll always have the English strings,
 * regardless of locale
 */
void uemis::event(struct dive *dive, struct divecomputer *dc, struct sample *sample, const uemis_sample *u_sample)
{
	const uint8_t *flags = u_sample->flags;
	int stopdepth;
	static int lastndl;

	if (flags[1] & 0x01)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Safety stop violation"));
	if (flags[1] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Speed alarm"));
#if WANT_CRAZY_WARNINGS
	if (flags[1] & 0x06) /* both bits 1 and 2 are a warning */
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Speed warning"));
	if (flags[1] & 0x10)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "pO₂ green warning"));
#endif
	if (flags[1] & 0x20)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "pO₂ ascend warning"));
	if (flags[1] & 0x40)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "pO₂ ascend alarm"));
	/* flags[2] reflects the deco / time bar
	 * flags[3] reflects more display details on deco and pO2 */
	if (flags[4] & 0x01)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Tank pressure info"));
	if (flags[4] & 0x04)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "RGT warning"));
	if (flags[4] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "RGT alert"));
	if (flags[4] & 0x40)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Tank change suggested"));
	if (flags[4] & 0x80)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Depth limit exceeded"));
	if (flags[5] & 0x01)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Max deco time warning"));
	if (flags[5] & 0x04)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Dive time info"));
	if (flags[5] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Dive time alert"));
	if (flags[5] & 0x10)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Marker"));
	if (flags[6] & 0x02)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "No tank data"));
	if (flags[6] & 0x04)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Low battery warning"));
	if (flags[6] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, QT_TRANSLATE_NOOP("gettextFromC", "Low battery alert"));
/* flags[7] reflects the little on screen icons that remind of previous
	 * warnings / alerts - not useful for events */

#if UEMIS_DEBUG & 32
	int i, j;
	for (i = 0; i < 8; i++) {
		printf(" %d: ", 29 + i);
		for (j = 7; j >= 0; j--)
			printf("%c", flags[i] & 1 << j ? '1' : '0');
	}
	printf("\n");
#endif
	/* now add deco / NDL
	 * we don't use events but store this in the sample - that makes much more sense
	 * for the way we display this information
	 * What we know about the encoding so far:
	 * flags[3].bit0 | flags[5].bit1 != 0 ==> in deco
	 * flags[0].bit7 == 1 ==> Safety Stop
	 * otherwise NDL */
	stopdepth = dive->rel_mbar_to_depth(u_sample->hold_depth).mm;
	if ((flags[3] & 1) | (flags[5] & 2)) {
		/* deco */
		sample->in_deco = true;
		sample->stopdepth.mm = stopdepth;
		sample->stoptime.seconds = u_sample->hold_time * 60;
		sample->ndl = 0_sec;
	} else if (flags[0] & 128) {
		/* safety stop - distinguished from deco stop by having
		 * both ndl and stop information */
		sample->in_deco = false;
		sample->stopdepth.mm = stopdepth;
		sample->stoptime.seconds = u_sample->hold_time * 60;
		sample->ndl.seconds = lastndl;
	} else {
		/* NDL */
		sample->in_deco = false;
		lastndl = sample->ndl.seconds = u_sample->hold_time * 60;
		sample->stopdepth = 0_m;
		sample->stoptime = 0_sec;
	}
#if UEMIS_DEBUG & 32
	printf("%dm:%ds: p_amb_tol:%d surface:%d holdtime:%d holddepth:%d/%d ---> stopdepth:%d stoptime:%d ndl:%d\n",
	       sample->time.seconds / 60, sample->time.seconds % 60, u_sample->p_amb_tol, dive->dcs[0].surface_pressure.mbar,
	       u_sample->hold_time, u_sample->hold_depth, stopdepth, sample->stopdepth.mm, sample->stoptime.seconds, sample->ndl.seconds);
#endif
}

/*
 * parse uemis base64 data blob into struct dive
 */
void uemis::parse_divelog_binary(std::string_view base64, struct dive *dive)
{
	struct sample *sample = NULL;
	uemis_sample *u_sample;
	struct divecomputer *dc = &dive->dcs[0];
	int dive_template, gasoffset;
	uint8_t active = 0;

	auto data = convert_base64(base64);
	dive->dcs[0].airtemp.mkelvin = C_to_mkelvin((*(uint16_t *)(data.data() + 45)) / 10.0);
	dive->dcs[0].surface_pressure.mbar = *(uint16_t *)(data.data() + 43);
	if (*(uint8_t *)(data.data() + 19))
		dive->dcs[0].salinity = SEAWATER_SALINITY; /* avg grams per 10l sea water */
	else
		dive->dcs[0].salinity = FRESHWATER_SALINITY; /* grams per 10l fresh water */

	/* this will allow us to find the last dive read so far from this computer */
	dc->model = "Uemis Zurich";
	dc->deviceid = *(uint32_t *)(data.data() + 9);
	dc->diveid = *(uint16_t *)(data.data() + 7);
	/* remember the weight units used in this dive - we may need this later when
	 * parsing the weight */
	weight_unit(dc->diveid, *(uint8_t *)(data.data() + 24));
	/* dive template in use:
	   0 = air
	   1 = nitrox (B)
	   2 = nitrox (B+D)
	   3 = nitrox (B+T+D)
	   uemis cylinder data is insane - it stores seven tank settings in a block
	   and the template tells us which of the four groups of tanks we need to look at
	 */
	gasoffset = dive_template = *(uint8_t *)(data.data() + 115);
	if (dive_template == 3)
		gasoffset = 4;
	if (dive_template == 0)
		dive_template = 1;
	for (int i = 0; i < dive_template; i++) {
		float volume = *(float *)(data.data() + 116 + 25 * (gasoffset + i)) * 1000.0f;
		/* uemis always assumes a working pressure of 202.6bar (!?!?) - I first thought
		 * it was 3000psi, but testing against all my dives gets me that strange number.
		 * Still, that's of course completely bogus and shows they don't get how
		 * cylinders are named in non-metric parts of the world...
		 * we store the incorrect working pressure to get the SAC calculations "close"
		 * but the user will have to correct this manually
		 */
		cylinder_t *cyl = dive->get_or_create_cylinder(i);
		cyl->type.size.mliter = lrintf(volume);
		cyl->type.workingpressure = 202600_mbar;
		cyl->gasmix.o2.permille = *(uint8_t *)(data.data() + 120 + 25 * (gasoffset + i)) * 10;
		cyl->gasmix.he = 0_percent;
	}
	/* first byte of divelog data is at offset 0x123 */
	size_t i = 0x123;
	u_sample = (uemis_sample *)(data.data() + i);
	while ((i <= data.size()) && (data[i] != 0 || data[i + 1] != 0)) {
		if (u_sample->active_tank != active) {
			if (u_sample->active_tank >= static_cast<int>(dive->cylinders.size())) {
				report_info("got invalid sensor #%d was #%d", u_sample->active_tank, active);
			} else {
				active = u_sample->active_tank;
				add_gas_switch_event(dive, dc, u_sample->dive_time, active);
			}
		}
		sample = prepare_sample(dc);
		sample->time.seconds = u_sample->dive_time;
		sample->depth = dive->rel_mbar_to_depth(u_sample->water_pressure);
		sample->temperature.mkelvin = C_to_mkelvin(u_sample->dive_temperature / 10.0);
		add_sample_pressure(sample, active, (u_sample->tank_pressure_high * 256 + u_sample->tank_pressure_low) * 10);
		sample->cns = u_sample->cns;
		event(dive, dc, sample, u_sample);
		i += 0x25;
		u_sample++;
	}
	if (sample)
		dive->dcs[0].duration = sample->time - 1_sec;

	/* get data from the footer */
	add_extra_data(dc, "FW Version",
		       format_string_std("%1u.%02u", data[18], data[17]));
	add_extra_data(dc, "Serial",
		       format_string_std("%08x", *(uint32_t *)(data.data() + 9)));
	add_extra_data(dc, "main battery after dive",
		       std::to_string(*(uint16_t *)(data.data() + i + 35)));
	add_extra_data(dc, "no fly time",
		       format_string_std("%0u:%02u", FRACTION_TUPLE(*(uint16_t *)(data.data() + i + 24), 60)));
	add_extra_data(dc, "no dive time",
		       format_string_std("%0u:%02u", FRACTION_TUPLE(*(uint16_t *)(data.data() + i + 26), 60)));
	add_extra_data(dc, "desat time",
		       format_string_std("%0u:%02u", FRACTION_TUPLE(*(uint16_t *)(data.data() + i + 28), 60)));
	add_extra_data(dc, "allowed altitude",
		       std::to_string(*(uint16_t *)(data.data() + i + 30)));

	return;
}
