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

QString qPrefUnits::length()
{
	return 	prefs.units.length == units::LENGTH::METERS ? QStringLiteral("meters") : QStringLiteral("feet");
}
void qPrefUnits::set_length(const QString& value)
{
	set_length(value == QStringLiteral("meters") ? units::LENGTH::METERS : units::LENGTH::FEET);
	emit instance()->lengthStringChanged(value);
}
SET_PREFERENCE_ENUM_EXT(Units, units::LENGTH, length, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "length", units::LENGTH, length, units.);

QString qPrefUnits::pressure()
{
	return 	prefs.units.pressure == units::PRESSURE::BAR ? QStringLiteral("bar") : QStringLiteral("psi");
}
void qPrefUnits::set_pressure(const QString& value)
{
	set_pressure(value == QStringLiteral("bar") ? units::PRESSURE::BAR : units::PRESSURE::PSI);
	emit instance()->pressureStringChanged(value);
}
SET_PREFERENCE_ENUM_EXT(Units, units::PRESSURE, pressure, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "pressure", units::PRESSURE, pressure, units.);

HANDLE_PREFERENCE_BOOL_EXT(Units, "show_units_table", show_units_table, units.);

QString qPrefUnits::temperature()
{
	return 	prefs.units.temperature == units::TEMPERATURE::CELSIUS ? QStringLiteral("celcius") : QStringLiteral("fahrenheit");
}
void qPrefUnits::set_temperature(const QString& value)
{
	set_temperature(value == QStringLiteral("celcius") ? units::TEMPERATURE::CELSIUS : units::TEMPERATURE::FAHRENHEIT);
	emit instance()->temperatureStringChanged(value);
}
SET_PREFERENCE_ENUM_EXT(Units, units::TEMPERATURE, temperature, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "temperature", units::TEMPERATURE, temperature, units.);

void qPrefUnits::set_unit_system(unit_system_values value)
{
	prefs.unit_system = value;
	if (prefs.unit_system == METRIC) {
		prefs.units = SI_units;

		// make sure all types are updated when changing
		set_volume(units::VOLUME::LITER);
		set_weight(units::WEIGHT::KG);
		set_length(units::LENGTH::METERS);
		set_pressure(units::PRESSURE::BAR);
		set_temperature(units::TEMPERATURE::CELSIUS);
	} else if (prefs.unit_system == IMPERIAL) {
		prefs.units = IMPERIAL_units;

		// make sure all types are updated when changing
		set_volume(units::VOLUME::CUFT);
		set_weight(units::WEIGHT::LBS);
		set_length(units::LENGTH::FEET);
		set_pressure(units::PRESSURE::PSI);
		set_temperature(units::TEMPERATURE::FAHRENHEIT);
	} else {
		prefs.unit_system = PERSONALIZE;
	}
	disk_unit_system(true);
	emit instance()->unit_systemChanged(value);
	emit instance()->volumeChanged(prefs.units.volume);
	emit instance()->volumeStringChanged(volume());
	emit instance()->weightChanged(prefs.units.weight);
	emit instance()->weightStringChanged(weight());
	emit instance()->lengthChanged(prefs.units.length);
	emit instance()->lengthStringChanged(length());
	emit instance()->temperatureChanged(prefs.units.temperature);
	emit instance()->temperatureStringChanged(temperature());
}
DISK_LOADSYNC_ENUM(Units, "unit_system", unit_system_values, unit_system);

QString qPrefUnits::vertical_speed_time()
{
	return 	prefs.units.vertical_speed_time == units::TIME::MINUTES ? QStringLiteral("minutes") : QStringLiteral("seconds");
}
void qPrefUnits::set_vertical_speed_time(const QString& value)
{
	set_vertical_speed_time(value == QStringLiteral("minutes") ? units::TIME::MINUTES : units::TIME::SECONDS);
	emit instance()->vertical_speed_timeStringChanged(value);
}
SET_PREFERENCE_ENUM_EXT(Units, units::TIME, vertical_speed_time, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "vertical_speed_time", units::TIME, vertical_speed_time, units.);

QString qPrefUnits::volume()
{
	return 	prefs.units.volume == units::VOLUME::LITER ? QStringLiteral("liter") : QStringLiteral("cuft");
}
void qPrefUnits::set_volume(const QString& value)
{
	set_volume(value == QStringLiteral("liter") ? units::VOLUME::LITER : units::VOLUME::CUFT);
	emit instance()->volumeStringChanged(value);
}
SET_PREFERENCE_ENUM_EXT(Units, units::VOLUME, volume, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "volume", units::VOLUME, volume, units.);

QString qPrefUnits::weight()
{
	return 	prefs.units.weight == units::WEIGHT::KG ? QStringLiteral("kg") : QStringLiteral("lbs");
}
void qPrefUnits::set_weight(const QString& value)
{
	set_weight(value == QStringLiteral("kg") ? units::WEIGHT::KG : units::WEIGHT::LBS);
	emit instance()->weightStringChanged(value);
}
SET_PREFERENCE_ENUM_EXT(Units, units::WEIGHT, weight, units.);
DISK_LOADSYNC_ENUM_EXT(Units, "weight", units::WEIGHT, weight, units.);
