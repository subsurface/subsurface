/*
 * uemis.c
 *
 * UEMIS SDA file importer
 * AUTHOR:  Dirk Hohndel - Copyright 2011
 *
 * Licensed under the MIT license.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#define __USE_XOPEN
#include <time.h>

#include "dive.h"
#include "uemis.h"
#include <libdivecomputer/parser.h>
#include <libdivecomputer/version.h>

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
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*
 * decodeblock -- decode 4 '6-bit' characters into 3 8-bit binary bytes
 */
static void decodeblock( unsigned char in[4], unsigned char out[3] ) {
	out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
	out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
	out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/*
 * decode a base64 encoded stream discarding padding, line breaks and noise
 */
void decode( uint8_t *inbuf, uint8_t *outbuf, int inbuf_len ) {
	uint8_t in[4], out[3], v;
	int i,len,indx_in=0,indx_out=0;

	while (indx_in < inbuf_len) {
		for (len = 0, i = 0; i < 4 && (indx_in < inbuf_len); i++ ) {
			v = 0;
			while ((indx_in < inbuf_len) && v == 0) {
				v = inbuf[indx_in++];
				v = ((v < 43 || v > 122) ? 0 : cd64[ v - 43 ]);
				if (v)
					v = ((v == '$') ? 0 : v - 61);
			}
			if (indx_in < inbuf_len) {
				len++;
				if (v)
					in[i] = (v - 1);
			}
			else
				in[i] = 0;
		}
		if( len ) {
			decodeblock( in, out );
			for( i = 0; i < len - 1; i++ )
				outbuf[indx_out++] = out[i];
		}
	}
}
/* end code from Bob Trower */

/*
 * convert the base64 data blog
 */
int uemis_convert_base64(char *base64, uint8_t **data) {
	int len,datalen;

	len = strlen(base64);
	datalen = (len/4 + 1)*3;
	if (datalen < 0x123+0x25) {
		/* less than header + 1 sample??? */
		fprintf(stderr,"suspiciously short data block\n");
	}
	*data = malloc(datalen);
	if (! *data) {
		datalen = 0;
		fprintf(stderr,"Out of memory\n");
		goto bail;
	}
	decode(base64, *data, len);

	if (memcmp(*data,"Dive\01\00\00",7))
		fprintf(stderr,"Missing Dive100 header\n");

bail:
	return datalen;
}

struct uemis_helper {
	int diveid;
	int lbs;
	int divespot;
	char **location;
	double *latitude;
	double *longitude;
	struct uemis_helper *next;
};
static struct uemis_helper *uemis_helper = NULL;

static struct uemis_helper *uemis_get_helper(int diveid)
{
	struct uemis_helper **php = &uemis_helper;
	struct uemis_helper *hp = *php;

	while(hp) {
		if (hp->diveid == diveid)
			return hp;
		if (hp->next) {
			hp = hp->next;
			continue;
		}
		php = &hp->next;
		break;
	}
	hp = *php = malloc(sizeof(struct uemis_helper));
	hp->diveid = diveid;
	hp->next = NULL;
	return hp;
}

static void uemis_weight_unit(int diveid, int lbs)
{
	struct uemis_helper *hp = uemis_get_helper(diveid);
	if (hp)
		hp->lbs = lbs;
}

int uemis_get_weight_unit(int diveid)
{
	struct uemis_helper *hp = uemis_helper;
	while (hp) {
		if (hp->diveid == diveid)
			return hp->lbs;
		hp = hp->next;
	}
	/* odd - we should have found this; default to kg */
	return 0;
}

void uemis_mark_divelocation(int diveid, int divespot, char **location, double *longitude, double *latitude)
{
	struct uemis_helper *hp = uemis_get_helper(diveid);
	hp->divespot = divespot;
	hp->location = location;
	hp->longitude = longitude;
	hp->latitude = latitude;
}

void uemis_set_divelocation(int divespot, char *text, double longitude, double latitude)
{
	struct uemis_helper *hp = uemis_helper;
	while (hp) {
		if (hp->divespot == divespot && hp->location) {
			*hp->location = text;
			*hp->longitude = longitude;
			*hp->latitude = latitude;
		}
		hp = hp->next;
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
static void uemis_event(struct dive *dive, struct divecomputer *dc, struct sample *sample, uemis_sample_t *u_sample)
{
	uint8_t *flags = u_sample->flags;

	if (flags[1] & 0x01)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Safety Stop Violation"));
	if (flags[1] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Speed Alarm"));
#if WANT_CRAZY_WARNINGS
	if (flags[1] & 0x06) /* both bits 1 and 2 are a warning */
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Speed Warning"));
	if (flags[1] & 0x10)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("PO2 Green Warning"));
#endif
	if (flags[1] & 0x20)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("PO2 Ascend Warning"));
	if (flags[1] & 0x40)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("PO2 Ascend Alarm"));
	/* flags[2] reflects the deco / time bar
	 * flags[3] reflects more display details on deco and pO2 */
	if (flags[4] & 0x01)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Tank Pressure Info"));
	if (flags[4] & 0x04)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("RGT Warning"));
	if (flags[4] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("RGT Alert"));
	if (flags[4] & 0x40)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Tank Change Suggested"));
	if (flags[4] & 0x80)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Depth Limit Exceeded"));
	if (flags[5] & 0x01)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Max Deco Time Warning"));
	if (flags[5] & 0x04)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Dive Time Info"));
	if (flags[5] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Dive Time Alert"));
	if (flags[5] & 0x10)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Marker"));
	if (flags[6] & 0x02)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("No Tank Data"));
	if (flags[6] & 0x04)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Low Battery Warning"));
	if (flags[6] & 0x08)
		add_event(dc, sample->time.seconds, 0, 0, 0, N_("Low Battery Alert"));
	/* flags[7] reflects the little on screen icons that remind of previous
	 * warnings / alerts - not useful for events */

	/* now add deco / NDL
	 * This will create an event for every sample (SUCK) but the
	 * code in the parser will then do the right thing and just
	 * keep the relevant ones */
	if (u_sample->p_amb_tol > dive->surface_pressure.mbar) {
		if (u_sample->hold_time && u_sample->hold_time < 99) {
			int hold_depth = u_sample->hold_depth / 100.0 + 0.5;
			add_event(dc, sample->time.seconds, SAMPLE_EVENT_DECOSTOP, 0,
				hold_depth | (u_sample->hold_time * 60) << 16 , N_("deco stop"));
		}
#if DC_VERSION_CHECK(0, 3, 0)
	} else {
		add_event(dc, sample->time.seconds, SAMPLE_EVENT_NDL, 0,
			u_sample->hold_time * 60, N_("non stop time"));
#endif
	}
}

/*
 * parse uemis base64 data blob into struct dive
 */
void uemis_parse_divelog_binary(char *base64, void *datap) {
	int datalen;
	int i;
	uint8_t *data;
	struct sample *sample;
	uemis_sample_t *u_sample;
	struct dive *dive = datap;
	struct divecomputer *dc = &dive->dc;
	int template, gasoffset;

	datalen = uemis_convert_base64(base64, &data);

	dive->airtemp.mkelvin = *(uint16_t *)(data + 45) * 100 + 273150;
	dive->surface_pressure.mbar = *(uint16_t *)(data + 43);
	if (*(uint8_t *)(data + 19))
		dive->salinity = 10300; /* avg grams per 10l sea water */
	else
		dive->salinity = 10000; /* grams per 10l fresh water */

	/* this will allow us to find the last dive read so far from this computer */
	dc->model = strdup("Uemis Zurich");
	dc->deviceid = *(uint32_t *)(data + 9);
	dc->diveid = *(uint16_t *)(data + 7);
	/* remember the weight units used in this dive - we may need this later when
	 * parsing the weight */
	uemis_weight_unit(dc->diveid, *(uint8_t *)(data + 24));
	/* dive template in use:
	   0 = air
	   1 = nitrox (B)
	   2 = nitrox (B+D)
	   3 = nitrox (B+T+D)
	   uemis cylinder data is insane - it stores seven tank settings in a block
	   and the template tells us which of the four groups of tanks we need to look at
	 */
	gasoffset = template = *(uint8_t *)(data+115);
	if (template == 3)
		gasoffset = 4;
	if (template == 0)
		template = 1;
	for (i = 0; i < template; i++) {
		float volume = *(float *)(data+116+25*(gasoffset + i)) * 1000.0;
		/* uemis always assumes a working pressure of 202.6bar (!?!?) - I first thought
		 * it was 3000psi, but testing against all my dives gets me that strange number.
		 * Still, that's of course completely bogus and shows they don't get how
		 * cylinders are named in non-metric parts of the world...
		 * we store the incorrect working pressure to get the SAC calculations "close"
		 * but the user will have to correct this manually
		 */
		dive->cylinder[i].type.size.mliter = volume;
		dive->cylinder[i].type.workingpressure.mbar = 202600;
		dive->cylinder[i].gasmix.o2.permille = *(uint8_t *)(data+120+25*(gasoffset + i)) * 10 + 0.5;
		dive->cylinder[i].gasmix.he.permille = 0;
	}
	/* first byte of divelog data is at offset 0x123 */
	i = 0x123;
	u_sample = (uemis_sample_t *)(data + i);
	while ((i < datalen) && (u_sample->dive_time)) {
		/* it seems that a dive_time of 0 indicates the end of the valid readings */
		/* the SDA usually records more samples after the end of the dive --
		 * we want to discard those, but not cut the dive short; sadly the dive
		 * duration in the header is a) in minutes and b) up to 3 minutes short */
		if (u_sample->dive_time > dive->duration.seconds + 180)
			break;
		sample = prepare_sample(dc);
		sample->time.seconds = u_sample->dive_time;
		sample->depth.mm = rel_mbar_to_depth(u_sample->water_pressure, dive);
		sample->temperature.mkelvin = (u_sample->dive_temperature * 100) + 273150;
		sample->cylinderindex = u_sample->active_tank;
		sample->cylinderpressure.mbar =
			(u_sample->tank_pressure_high * 256 + u_sample->tank_pressure_low) * 10;
		uemis_event(dive, dc, sample, u_sample);
		finish_sample(dc);
		i += 0x25;
		u_sample++;
	}
	dive->duration.seconds = sample->time.seconds - 1;
	return;
}
