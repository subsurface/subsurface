/*
 * helpers.h
 *
 * header file for random helper functions of Subsurface
 *
 */
#ifndef HELPER_H
#define HELPER_H

#include <QString>
#include "dive.h"
#include "qthelper.h"

QString get_depth_string(depth_t depth, bool showunit = false, bool showdecimal = true);
QString get_depth_string(int mm, bool showunit = false, bool showdecimal = true);
QString get_depth_unit();
QString get_weight_string(weight_t weight, bool showunit = false);
QString get_weight_unit();
QString get_cylinder_used_gas_string(cylinder_t *cyl, bool showunit = false);
QString get_temperature_string(temperature_t temp, bool showunit = false);
QString get_temp_unit();
QString get_volume_string(volume_t volume, bool showunit = false, unsigned int mbar = 0);
QString get_volume_unit();
QString get_pressure_string(pressure_t pressure, bool showunit = false);
QString get_pressure_unit();
void set_default_dive_computer(const char *vendor, const char *product);
void set_default_dive_computer_device(const char *name);
QString getSubsurfaceDataPath(QString folderToFind);
extern const QString get_dc_nickname(const char *model, uint32_t deviceid);
int gettimezoneoffset();
int parseTemperatureToMkelvin(const QString& text);
QString get_dive_date_string(timestamp_t when);
QString get_short_dive_date_string(timestamp_t when);
QString get_trip_date_string(timestamp_t when, int nr);

extern DiveComputerList dcList;

#define M_OR_FT(_m,_f) ((prefs.units.length == units::METERS) ? ((_m) * 1000) : (feet_to_mm(_f)))

#if defined __APPLE__
#define TITLE_OR_TEXT(_t,_m) "", _t + "\n" + _m
#else
#define TITLE_OR_TEXT(_t,_m) _t, _m
#endif
#endif /* HELPER_H */
