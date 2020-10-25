// SPDX-License-Identifier: GPL-2.0
#ifndef SAMPLE_H
#define SAMPLE_H

#include "units.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SENSORS 2
struct sample                         // BASE TYPE BYTES  UNITS    RANGE               DESCRIPTION
{                                     // --------- -----  -----    -----               -----------
	duration_t time;                  // int32_t    4  seconds  (0-34 yrs)             elapsed dive time up to this sample
	duration_t stoptime;              // int32_t    4  seconds  (0-34 yrs)             time duration of next deco stop
	duration_t ndl;                   // int32_t    4  seconds  (-1 no val, 0-34 yrs)  time duration before no-deco limit
	duration_t tts;                   // int32_t    4  seconds  (0-34 yrs)             time duration to reach the surface
	duration_t rbt;                   // int32_t    4  seconds  (0-34 yrs)             remaining bottom time
	depth_t depth;                    // int32_t    4    mm     (0-2000 km)            dive depth of this sample
	depth_t stopdepth;                // int32_t    4    mm     (0-2000 km)            depth of next deco stop
	temperature_t temperature;        // uint32_t   4    mK     (0-4 MK)               ambient temperature
	pressure_t pressure[MAX_SENSORS]; // int32_t    4    mbar   (0-2 Mbar)             cylinder pressures (main and CCR o2)
	o2pressure_t setpoint;            // uint16_t   2    mbar   (0-65 bar)             O2 partial pressure (will be setpoint)
	o2pressure_t o2sensor[3];         // uint16_t   6    mbar   (0-65 bar)             Up to 3 PO2 sensor values (rebreather)
	bearing_t bearing;                // int16_t    2  degrees  (-1 no val, 0-360 deg) compass bearing
	uint8_t sensor[MAX_SENSORS];      // uint8_t    1  sensorID (0-255)                ID of cylinder pressure sensor
	uint16_t cns;                     // uint16_t   1     %     (0-64k %)              cns% accumulated
	uint8_t heartbeat;                // uint8_t    1  beats/m  (0-255)                heart rate measurement
	volume_t sac;                     //            4  ml/min                          predefined SAC
	bool in_deco;                     // bool       1    y/n      y/n                  this sample is part of deco
	bool manually_entered;            // bool       1    y/n      y/n                  this sample was entered by the user,
					  //                                               not calculated when planning a dive
};	                                  // Total size of structure: 57 bytes, excluding padding at end

extern void add_sample_pressure(struct sample *sample, int sensor, int mbar);

#ifdef __cplusplus
}
#endif

#endif
