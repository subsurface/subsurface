/*
 * defines and prototypes for the uemis Zurich SDA file parser
 */

#ifndef UEMIS_H
#define UEMIS_H

#include <stdint.h>
#include "dive.h"

#ifdef __cplusplus
extern "C" {
#endif

void uemis_parse_divelog_binary(char *base64, void *divep);
int uemis_get_weight_unit(int diveid);
void uemis_mark_divelocation(int diveid, int divespot, uint32_t dive_site_uuid);
void uemis_set_divelocation(int divespot, char *text, double longitude, double latitude);
int uemis_get_divespot_id_by_diveid(uint32_t diveid);

typedef struct
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
} __attribute((packed)) uemis_sample_t;

#ifdef __cplusplus
}
#endif

#endif // UEMIS_H
