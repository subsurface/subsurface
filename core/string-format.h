// SPDX-License-Identifier: GPL-2.0
// Various functions that format data into QStrings or QStringLists
#ifndef STRING_FORMAT_H
#define STRING_FORMAT_H

#include "units.h"
#include <QStringList>
#include <string>

struct dive;
struct dive_trip;
struct event;

// Functions that format dive data
QString formatSac(const dive *d);
QString formatNotes(const dive *d);
QString format_gps_decimal(const dive *d);
QStringList formatGetCylinder(const dive *d);
QStringList formatStartPressure(const dive *d);
QStringList formatEndPressure(const dive *d);
QStringList formatFirstGas(const dive *d);
QString formatGas(const dive *d);
QStringList formatFullCylinderList();
QStringList formatCylinders(const dive *d);
QString formatSumWeight(const dive *d);
QString formatWeightList(const dive *d);
QStringList formatWeights(const dive *d);
QString formatDiveDuration(const dive *d);
QString formatDiveGPS(const dive *d);
QString formatDiveDate(const dive *d);
QString formatDiveTime(const dive *d);
QString formatDiveDateTime(const dive *d);
QString formatDiveGasString(const dive *d);
QString formatDayOfWeek(int day);
QString formatMinutes(int seconds);
QString formatTripTitle(const dive_trip &trip);
QString formatTripTitleWithDives(const dive_trip &trip);

// Functions that format arbitrary data
QString get_utc_offset_string(std::optional<int> offset);
QString get_depth_string(depth_t depth, bool showunit = false, bool showdecimal = true);
QString get_depth_string(int mm, bool showunit = false, bool showdecimal = true);
QString get_depth_unit(bool metric);
QString get_depth_unit(); // use preferences unit
QString get_weight_string(weight_t weight, bool showunit = false);
QString get_weight_unit(bool metric);
QString get_weight_unit(); // use preferences unit
QString get_temperature_string(temperature_t temp, bool showunit = false);
QString get_temp_unit(bool metric);
QString get_temp_unit(); // use preferences unit
QString get_volume_string(volume_t volume, bool showunit = false);
QString get_volume_string(int mliter, bool showunit = false);
QString get_volume_unit(bool metric);
QString get_volume_unit(); // use preferences unit
QString get_pressure_string(pressure_t pressure, bool showunit = false);
QString get_salinity_string(int salinity);
QString get_water_type_string(int salinity);
QStringList get_water_types_as_string();
QString get_dive_duration_string(timestamp_t when, QString hoursText, QString minutesText, QString secondsText, QString separator = ":", bool isFreeDive = false);
QString get_dive_duration_string(timestamp_t when, QString hoursText, QString minutesText); // Default values for seconds, separator and isFreeDive
QString get_dive_surfint_string(timestamp_t when, QString daysText, QString hoursText, QString minutesText, QString separator = " ", int maxdays = 4);
QString get_dive_date_string(timestamp_t when);
QString get_duration_string_short(duration_t duration);
std::string get_dive_date_c_string(timestamp_t when);
QString get_dive_datetime_string(datetime_t when);
QString distance_string(int distanceInMeters);
QString printGPSCoords(const location_t *loc);
std::string printGPSCoordsC(const location_t *loc);

#endif
