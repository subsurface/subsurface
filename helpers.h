/*
 * helpers.h
 *
 * header file for random helper functions of Subsurface
 *
 */
#ifndef HELPERS_H
#define HELPERS_H

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
QString get_volume_string(volume_t volume, bool showunit = false, int mbar = 0);
QString get_volume_unit();
QString get_pressure_string(pressure_t pressure, bool showunit = false);
QString get_pressure_unit();
void set_default_dive_computer(const char *vendor, const char *product);
void set_default_dive_computer_device(const char *name);
void set_default_dive_computer_download_mode(int downloadMode);
QString getSubsurfaceDataPath(QString folderToFind);
QString getPrintingTemplatePathUser();
QString getPrintingTemplatePathBundle();
void copyPath(QString src, QString dst);
extern const QString get_dc_nickname(const char *model, uint32_t deviceid);
int gettimezoneoffset(timestamp_t when = 0);
int parseTemperatureToMkelvin(const QString &text);
QString get_dive_duration_string(timestamp_t when, QString hourText, QString minutesText);
QString get_dive_date_string(timestamp_t when);
QString get_short_dive_date_string(timestamp_t when);
bool is_same_day (timestamp_t trip_when, timestamp_t dive_when);
QString get_trip_date_string(timestamp_t when, int nr, bool getday);
QString uiLanguage(QLocale *callerLoc);
QLocale getLocale();
QString getDateFormat();
void selectedDivesGasUsed(QVector<QPair<QString, int> > &gasUsed);
QString getUserAgent();

#if defined __APPLE__
#define TITLE_OR_TEXT(_t, _m) "", _t + "\n" + _m
#else
#define TITLE_OR_TEXT(_t, _m) _t, _m
#endif
#endif // HELPERS_H
