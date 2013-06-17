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

QString get_depth_string(depth_t depth, bool showunit);
QString get_weight_string(weight_t weight, bool showunit);
QString get_temperature_string(temperature_t temp, bool showunit);
QString get_volume_string(volume_t volume, bool showunit);
QString get_pressure_string(pressure_t pressure, bool showunit);
void set_default_dive_computer(const char *vendor, const char *product);
void set_default_dive_computer_device(const char *name);
QString getSubsurfaceDataPath(QString folderToFind);
extern const QString get_dc_nickname(const char *model, uint32_t deviceid);

extern DiveComputerList dcList;

#endif /* HELPER_H */
