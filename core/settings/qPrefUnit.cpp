// SPDX-License-Identifier: GPL-2.0
#include "qPrefUnit.h"
#include "qPrefPrivate.h"


static const QString group = QStringLiteral("Units");

qPrefUnits::qPrefUnits(QObject *parent) : QObject(parent)
{
}

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

HANDLE_PREFERENCE_ENUM_EXT(Units, units::DURATION, "duration_units", duration_units, units.);

HANDLE_PREFERENCE_ENUM_EXT(Units, units::LENGTH, "length", length, units.);

HANDLE_PREFERENCE_ENUM_EXT(Units, units::PRESSURE, "pressure", pressure, units.);

HANDLE_PREFERENCE_BOOL_EXT(Units, "show_units_table", show_units_table, units.);

HANDLE_PREFERENCE_ENUM_EXT(Units, units::TEMPERATURE, "temperature", temperature, units.);

QString qPrefUnits::unit_system()
{
	return 	prefs.unit_system == METRIC ? QStringLiteral("metric") :
			prefs.unit_system == IMPERIAL ? QStringLiteral("imperial") :
											QStringLiteral("personalized");
}
void qPrefUnits::set_unit_system(const QString& value)
{
	short int v = value == QStringLiteral("metric") ? METRIC :
					value == QStringLiteral("imperial")? IMPERIAL :
							PERSONALIZE;
	if (v == METRIC) {
		prefs.unit_system = METRIC;
		prefs.units = SI_units;
	} else if (v == IMPERIAL) {
		prefs.unit_system = IMPERIAL;
		prefs.units = IMPERIAL_units;
	} else {
		prefs.unit_system = PERSONALIZE;
	}
	disk_unit_system(true);
	emit instance()->unit_systemChanged(value);
}
DISK_LOADSYNC_ENUM(Units, "unit_system", unit_system_values, unit_system);

HANDLE_PREFERENCE_ENUM_EXT(Units, units::TIME, "vertical_speed_time", vertical_speed_time, units.);

HANDLE_PREFERENCE_ENUM_EXT(Units, units::VOLUME, "volume", volume, units.);

HANDLE_PREFERENCE_ENUM_EXT(Units, units::WEIGHT, "weight", weight, units.);
