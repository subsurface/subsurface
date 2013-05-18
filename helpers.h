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

QString get_depth_string(depth_t depth, bool showunit);
QString get_weight_string(weight_t weight, bool showunit);
QString get_temperature_string(temperature_t temp, bool showunit);
QString get_volume_string(volume_t volume, bool showunit);
QString get_pressure_string(pressure_t pressure, bool showunit);

#endif /* HELPER_H */
