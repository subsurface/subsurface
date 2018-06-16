// SPDX-License-Identifier: GPL-2.0
#include "qPrefUnits.h"
#include "qPref_private.h"



qPrefUnits *qPrefUnits::m_instance = NULL;
qPrefUnits *qPrefUnits::instance()
{
	if (!m_instance)
 		m_instance = new qPrefUnits;
	return m_instance;
}


void qPrefUnits::loadSync(bool doSync)
{
	diskCoordinatesTraditional(doSync);
	diskDurationUnits(doSync);
	diskLength(doSync);
	diskPressure(doSync);
	diskShowUnitsTable(doSync);
	diskTemperature(doSync);
	diskUnitSystem(doSync);
	diskVerticalSpeedTime(doSync);
	diskVolume(doSync);
	diskWeight(doSync);
}


bool qPrefUnits::coordinatesTraditional() const
{
	return prefs.coordinates_traditional;
}
void qPrefUnits::setCoordinatesTraditional(bool value)
{
	if (value != prefs.coordinates_traditional) {
		prefs.coordinates_traditional = value;
		diskCoordinatesTraditional(true);
		emit coordinatesTraditionalChanged(value);
	}
}
void qPrefUnits::diskCoordinatesTraditional(bool doSync)
{
	LOADSYNC_BOOL("/coordinates", coordinates_traditional);
}


units::DURATION qPrefUnits::durationUnits() const
{
	return prefs.units.duration_units;
}
void qPrefUnits::setDurationUnits(units::DURATION value)
{
	if (value != prefs.units.duration_units) {
		prefs.units.duration_units = value;
		diskDurationUnits(true);
		emit durationUnitsChanged(value);
	}
}
void qPrefUnits::diskDurationUnits(bool doSync)
{
	LOADSYNC_ENUM("/duration_units", units::DURATION, units.duration_units);
}


units::LENGTH qPrefUnits::length() const
{
	return prefs.units.length;
}
void qPrefUnits::setLength(units::LENGTH value)
{
	if (value != prefs.units.length) {
		prefs.units.length = value;
		diskLength(true);
		emit lengthChanged(value);
	}
}
void qPrefUnits::diskLength(bool doSync)
{
	LOADSYNC_ENUM("/length", units::LENGTH, units.length);
}


units::PRESSURE qPrefUnits::pressure() const
{
	return prefs.units.pressure;
}
void qPrefUnits::setPressure(units::PRESSURE value)
{
	if (value == prefs.units.pressure) {
		prefs.units.pressure = (units::PRESSURE) value;
		diskPressure(true);
		emit pressureChanged(value);
	}
}
void qPrefUnits::diskPressure(bool doSync)
{
	LOADSYNC_ENUM("/pressure", units::PRESSURE, units.pressure);
}


bool qPrefUnits::showUnitsTable() const
{
	return prefs.units.show_units_table;
}
void qPrefUnits::setShowUnitsTable(bool value)
{
	if (value != prefs.units.show_units_table) {
		prefs.units.show_units_table = value;
		diskShowUnitsTable(true);
		emit showUnitsTableChanged(value);
	}
}
void qPrefUnits::diskShowUnitsTable(bool doSync)
{
	LOADSYNC_BOOL("/show_units_table", units.show_units_table);
}


units::TEMPERATURE qPrefUnits::temperature() const
{
	return prefs.units.temperature;
}
void qPrefUnits::setTemperature(units::TEMPERATURE value)
{
	if (value != prefs.units.temperature) {
		prefs.units.temperature = value;
		diskTemperature(true);
	QSettings s;
	s.setValue(group + "/temperature", value);
	emit temperatureChanged(value);
	}
}
void qPrefUnits::diskTemperature(bool doSync)
{
	LOADSYNC_ENUM("/temperature", units::TEMPERATURE, units.temperature);
}


const QString qPrefUnits::unitSystem() const
{
	return 	prefs.unit_system == METRIC ? QStringLiteral("metric") :
			prefs.unit_system == IMPERIAL ? QStringLiteral("imperial") :
											QStringLiteral("personalized");
}
void qPrefUnits::setUnitSystem(const QString& value)
{
	short int v = value == QStringLiteral("metric") ? METRIC :
					value == QStringLiteral("imperial")? IMPERIAL :
							PERSONALIZE;
	if (v != prefs.unit_system) {
		if (v == METRIC) {
			prefs.unit_system = METRIC;
			prefs.units = SI_units;
		} else if (v == IMPERIAL) {
			prefs.unit_system = IMPERIAL;
			prefs.units = IMPERIAL_units;
		} else {
		prefs.unit_system = PERSONALIZE;
		}
		emit unitSystemChanged(value);
	}
}
void qPrefUnits::diskUnitSystem(bool doSync)
{
	LOADSYNC_INT("/unit_system", unit_system);
}


units::TIME qPrefUnits::verticalSpeedTime() const
{
	return prefs.units.vertical_speed_time;
}
void qPrefUnits::setVerticalSpeedTime(units::TIME value)
{
	if (value != prefs.units.vertical_speed_time) {
		prefs.units.vertical_speed_time = value;
		diskVerticalSpeedTime(true);
		emit verticalSpeedTimeChanged(value);
	}
}
void qPrefUnits::diskVerticalSpeedTime(bool doSync)
{
	LOADSYNC_ENUM("/vertical_speed_time", units::TIME, units.vertical_speed_time);
}


units::VOLUME qPrefUnits::volume() const
{
	return prefs.units.volume;
}
void qPrefUnits::setVolume(units::VOLUME value)
{
	if (value != prefs.units.volume) {
		prefs.units.volume = (units::VOLUME) value;
		diskVolume(true);
		emit volumeChanged(value);
	}
}
void qPrefUnits::diskVolume(bool doSync)
{
	LOADSYNC_ENUM("/volume", units::VOLUME, units.volume);
}


units::WEIGHT qPrefUnits::weight() const
{
	return prefs.units.weight;
}
void qPrefUnits::setWeight(units::WEIGHT value)
{
	if (value != prefs.units.weight) {
		prefs.units.weight = (units::WEIGHT) value;
		diskWeight(true);
		emit weightChanged(value);
	}
}
void qPrefUnits::diskWeight(bool doSync)
{
	LOADSYNC_ENUM("/weight", units::WEIGHT, units.weight);
}
