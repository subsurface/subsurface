// SPDX-License-Identifier: GPL-2.0
#include "qPrefUnit.h"
#include "qPrefPrivate.h"


static const QString group = QStringLiteral("Units");

qPrefUnits *qPrefUnits::instance()
{
	static qPrefUnits *self = new qPrefUnits;
	return self;
}


void qPrefUnits::loadSync(bool doSync)
{
	disk_coordinates_traditional(doSync);
	disk_duration_units(doSync);
	disk_length(doSync);
	disk_pressure(doSync);
	disk_show_units_table(doSync);
	disk_temperature(doSync);
	disk_unit_system(doSync);
	disk_vertical_speed_time(doSync);
	disk_volume(doSync);
	disk_weight(doSync);
}

HANDLE_PREFERENCE_BOOL(Units, "coordinates", coordinates_traditional);

SET_PREFERENCE_ENUM_EXT(Units, units::DURATION, duration_units, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "duration_units", units::DURATION, duration_units, units.);

SET_PREFERENCE_ENUM_EXT(Units, units::LENGTH, length, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "length", units::LENGTH, length, units.);

SET_PREFERENCE_ENUM_EXT(Units, units::PRESSURE, pressure, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "pressure", units::PRESSURE, pressure, units.);

HANDLE_PREFERENCE_BOOL_EXT(Units, "show_units_table", show_units_table, units.);

SET_PREFERENCE_ENUM_EXT(Units, units::TEMPERATURE, temperature, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "temperature", units::TEMPERATURE, temperature, units.);

void qPrefUnits::set_unit_system(unit_system_values value)
{
	prefs.unit_system = value;
	if (prefs.unit_system == METRIC) {
		// make sure all types are updated when changing
		set_volume(units::VOLUME::LITER);
		set_weight(units::WEIGHT::KG);
		set_length(units::LENGTH::METERS);
		set_pressure(units::PRESSURE::BAR);
		set_temperature(units::TEMPERATURE::CELSIUS);

		// this statement need to be AFTER the setters are called
		// because it sets all of prefs.units without calling the
		// setters
		prefs.units = SI_units;
	} else if (prefs.unit_system == IMPERIAL) {
		// make sure all types are updated when changing
		set_volume(units::VOLUME::CUFT);
		set_weight(units::WEIGHT::LBS);
		set_length(units::LENGTH::FEET);
		set_pressure(units::PRESSURE::PSI);
		set_temperature(units::TEMPERATURE::FAHRENHEIT);

		// this statement need to be AFTER the setters are called
		// because it sets all of prefs.units without calling the
		// setters
		prefs.units = IMPERIAL_units;
	}
	disk_unit_system(true);
	emit instance()->unit_systemChanged(value);
}
DISK_LOADSYNC_ENUM(Units, "unit_system", unit_system_values, unit_system);

SET_PREFERENCE_ENUM_EXT(Units, units::TIME, vertical_speed_time, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "vertical_speed_time", units::TIME, vertical_speed_time, units.);

SET_PREFERENCE_ENUM_EXT(Units, units::VOLUME, volume, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "volume", units::VOLUME, volume, units.);

SET_PREFERENCE_ENUM_EXT(Units, units::WEIGHT, weight, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "weight", units::WEIGHT, weight, units.);
