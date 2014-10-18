/*
 * subsurface
 *
 * Copyright (C) 2014 John Van Ostrand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

// 512 bytes for each dive in the log book
struct cochran_emc_log_t {
	// Pre-dive 256 bytes
	unsigned char seconds, minutes, hour;		// 3 bytes
	unsigned char day, month, year;				// 3 bytes
	unsigned char sample_start_offset[4];		// 4 bytes
	unsigned char start_timestamp[4];			// 4 bytes [secs from jan 1,92]
	unsigned char pre_dive_timestamp[4];		// 4 bytes [secs from Jan 1,92]
	unsigned char unknown1[6];					// 6 bytes
	unsigned char water_conductivity;			// 1 byte  [0 =low, 2-high]
	unsigned char unknown2[5];					// 5 bytes
//30
	unsigned char sample_pre_event_offset[4];	// 4 bytes
	unsigned char config_bitfield[6];			// 6 bytes
	unsigned char unknown3[2];					// 2 bytes
	unsigned char start_depth[2];				// 2 bytes [/256]
	unsigned char unknown4[2];					// 2 bytes
	unsigned char start_battery_voltage[2];		// 2 bytes [/256]
//48
	unsigned char unknown5[7];					// 7 bytes
	unsigned char start_temperature;			// 1 byte  [F]
	unsigned char unknown6[28];					// 28 bytes
	unsigned char sit[2];						// 2 bytes [minutes]
	unsigned char number[2];					// 2 bytes
	unsigned char unknown7[1]; 					// 1 bytes
	unsigned char altitude;						// 1 byte [/4 = kft]
	unsigned char start_nofly[2];				// 2 bytes [/256 = hours]
//92
	unsigned char unknown8[18]; 				// 18 bytes
	unsigned char post_dive_sit[2];				// 2 bytes  [seconds]
	unsigned char po2_set_point[9][2];			// 18 bytes	[/256 = %]
	unsigned char unknown9[12]; 				// 12 bytes
	unsigned char po2_alarm[2]; 				// 2 bytes  [/256 = %]
//144
	unsigned char o2_percent[10][2];			// 20 bytes	[/256 = %]
	unsigned char he_percent[10][2];			// 20 bytes	[/256 = %]
	unsigned char alarm_depth[2];				// 2 bytes
	unsigned char unknown10[14]; 				// 14 bytes
	unsigned char conservatism;					// 1 bytes [/256 = fraction]
	unsigned char unknown11[2];					// 2 bytes
	unsigned char repetitive_dive;				// 1 byte
	unsigned char unknown12[12];				// 12 bytes
	unsigned char start_tissue_nsat[20][2];		// 40 bytes [/256]

	// Post-dive 256 bytes
	unsigned char sample_end_offset[4];			// 4 bytes
	unsigned char unknown13[33];				// 33 bytes
	unsigned char temp;							// 1 byte  [F]
	unsigned char unknown14[10];				// 10 bytes
// 48
	unsigned char bt[2];						// 2 bytes [minutes]
	unsigned char max_depth[2];					// 2 bytes [/4 = ft]
	unsigned char unknown15[2];					// 2 bytes
	unsigned char avg_depth[2];					// 2 bytes [/4 = ft]
	unsigned char min_ndc[2];					// 2 bytes [minutes]
	unsigned char min_ndx_bt[2];				// 2 bytes [minutes]
	unsigned char max_forecast_deco[2];			// 2 bytes [minutes]
	unsigned char max_forecast_deco_bt[2];		// 2 bytes [minutes]
//64
	unsigned char max_ceiling[2];				// 2 bytes [*10 = ft]
	unsigned char max_ceiling_bt[2];			// 2 bytes [minutes]
	unsigned char unknown16[10];				// 18 bytes
	unsigned char max_ascent_rate; 				// 1 byte  [ft/min]
	unsigned char unknown17[3];					// 3 bytes
	unsigned char max_ascent_rate_bt[2];		// 2 bytes [seconds]
//84
	unsigned char unknown18[54];				// 54 bytes
//138
	unsigned char end_battery_voltage[2];		// 2 bytes [/256 = v]
	unsigned char unknown19[8];					// 8 bytes
	unsigned char min_temp_bt[2];    			// 2 bytes [seconds]
//150
	unsigned char unknown20[22];				// 22 bytes
//172
	unsigned char end_nofly[2];					// 2 bytes	[/256 = hours]
	unsigned char alarm_count[2];				// 2 byte
	unsigned char actual_deco_time[2];			// 2 bytes [seconds]
//178
	unsigned char unknown21[38];				// 38 bytes
//216
	unsigned char end_tissue_nsat[20][2];		// 40 bytes [/256 = fraction]
} __attribute__((packed));

typedef enum cochran_emc_bitfield_config_t {
	BF_TEMP_DEPENDENT_N2,
	BF_ASCENT_RATE_BAR_GRAPH,
	BF_BLEND_2_SWITCHING,
	BF_ALTITUDE_AS_ONE_ZONE,
	BF_DECOMPRESSION_TIME_DISPLAY,
	BF_BLEND_3_SWITCHING,
	BF_VARIABLE_ASCENT_RATE_ALARM,
	BF_ASCENT_RATE_RESPONSE,
	BF_REPETITIVE_DIVE_DEPENDENT_N2,
	BF_TRAINING_MODE,
	BF_CONSTANT_MODE_COMPUTATIONS,
	BF_DISPLAYED_UNITS,
	BF_AUDIBLE_ALARM,
	BF_CLOCK,
	BF_CEILING_DISPLAY_DIV_BY_10,
	BF_GAS_2_AS_FIRST_GAS,
	BF_ENABLE_HELIUM_COMPUTATIONS,
	BF_AUTOMATIC_PO2_FO2_SWITCHING,
	BF_TOUCH_PROGRAMMING_PO2_FO2_SWITCH,
} cochran_emc_bitfield_config_t;


struct cochran_emc_bitfield_t {
	cochran_emc_bitfield_config_t config;
	unsigned char word;
	unsigned char byte;
	unsigned char mask;
	unsigned char shift;
} cochran_emc_bitfield_t;

static struct cochran_emc_bitfield_t cochran_emc_bits[] = {
// Word BD
	{ BF_TEMP_DEPENDENT_N2, 0xBD, 0, 0x40, 6 }, 			// 0=normal, 1=reduced
	{ BF_ASCENT_RATE_BAR_GRAPH, 0xBD, 0, 0x20, 5 },			// 0=fixed, 1=proportional
	{ BF_BLEND_2_SWITCHING, 0xBD, 0, 0x04, 2 },			// 0=dis, 1=ena
	{ BF_ALTITUDE_AS_ONE_ZONE, 0xBD, 0, 0x02, 1},			// 0=off, 1=on

	{ BF_DECOMPRESSION_TIME_DISPLAY, 0xBD, 1, 0xC0, 5},		// 111=both, 011=stop, 001=total
	{ BF_BLEND_3_SWITCHING, 0xBD, 1, 0x10, 4 },			// 0=dis, 1=ena
	{ BF_VARIABLE_ASCENT_RATE_ALARM, 0xBD, 1, 0x04, 3},		// 0=off, 1=on
	{ BF_ASCENT_RATE_RESPONSE, 0xBD, 1, 0x07, 0},

//WORD BE
	{ BF_REPETITIVE_DIVE_DEPENDENT_N2, 0xBE, 0, 0x80, 7 },		// 0=off, 1=on
	{ BF_TRAINING_MODE, 0xBE, 0, 0x04, 2 }, 			// 0=off, 1=on
	{ BF_CONSTANT_MODE_COMPUTATIONS, 0xBE, 0, 0x04, 2 },		// 0=FO2, 1=PO2
	{ BF_DISPLAYED_UNITS, 0xBE, 0, 0x01, 0 },			// 1=metric, 0=imperial

// WORD BF
	{ BF_AUDIBLE_ALARM, 0xBF, 0, 0x40, 6 }, 			// 0=on, 1=off ***
	{ BF_CLOCK, 0xBF, 0, 0x20, 5 },					// 0=off, 1=on
	{ BF_CEILING_DISPLAY_DIV_BY_10, 0xBF, 0, 0x10, 4 },		// 0=off, 1=on
	{ BF_GAS_2_AS_FIRST_GAS, 0xBF, 0, 0x02, 1 },			// 0=dis, 1=ena
	{ BF_ENABLE_HELIUM_COMPUTATIONS, 0xBF, 0, 0x01, 0 },		// 0=dis, 1=ena

	{ BF_AUTOMATIC_PO2_FO2_SWITCHING, 0xBF, 1, 0x04, 2 },		// 0=dis, 1=ena
	{ BF_TOUCH_PROGRAMMING_PO2_FO2_SWITCH, 0xBF, 1, 0x02, 1 },	// 0=dis, 1=ena
};

struct cochran_emc_config1_t {
	unsigned char unknown1[209];
	unsigned short int dive_count;
	unsigned char unknown2[274];
	unsigned short int serial_num;
	unsigned char unknown3[24];
} __attribute__((packed));
