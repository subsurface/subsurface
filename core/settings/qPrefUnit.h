// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFUNIT_H
#define QPREFUNIT_H
#include "core/pref.h"

#include <QObject>


class qPrefUnits : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool coordinates_traditional READ coordinates_traditional WRITE set_coordinates_traditional NOTIFY coordinates_traditionalChanged);
	Q_PROPERTY(units::DURATION duration_units READ duration_units WRITE set_duration_units NOTIFY duration_unitsChanged);
	Q_PROPERTY(units::LENGTH length READ length WRITE set_length NOTIFY lengthChanged);
	Q_PROPERTY(units::PRESSURE pressure READ pressure WRITE set_pressure NOTIFY pressureChanged);
	Q_PROPERTY(bool show_units_table READ show_units_table WRITE set_show_units_table NOTIFY show_units_tableChanged);
	Q_PROPERTY(units::TEMPERATURE temperature READ temperature WRITE set_temperature NOTIFY temperatureChanged);
	Q_PROPERTY(QString unit_system READ unit_system WRITE set_unit_system NOTIFY unit_systemChanged);
	Q_PROPERTY(units::TIME vertical_speed_time READ vertical_speed_time WRITE set_vertical_speed_time NOTIFY vertical_speed_timeChanged);
	Q_PROPERTY(units::VOLUME volume READ volume WRITE set_volume NOTIFY volumeChanged);
	Q_PROPERTY(units::WEIGHT weight READ weight WRITE set_weight NOTIFY weightChanged);

public:
	qPrefUnits(QObject *parent = NULL);
	static qPrefUnits *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static bool coordinates_traditional() { return prefs.coordinates_traditional; }
	static units::DURATION duration_units() { return prefs.units.duration_units; }
	static units::LENGTH length() { return prefs.units.length; }
	static units::PRESSURE pressure() { return prefs.units.pressure; }
	static bool show_units_table() { return prefs.units.show_units_table; }
	static units::TEMPERATURE temperature() { return prefs.units.temperature; }
	static QString unit_system();
	static units::TIME vertical_speed_time() { return prefs.units.vertical_speed_time; }
	static units::VOLUME volume() { return prefs.units.volume; }
	static units::WEIGHT weight() { return prefs.units.weight; }

public slots:
	static void set_coordinates_traditional(bool value);
	static void set_duration_units(units::DURATION value);
	static void set_length(units::LENGTH value);
	static void set_pressure(units::PRESSURE value);
	static void set_show_units_table(bool value);
	static void set_temperature(units::TEMPERATURE value);
	static void set_unit_system(const QString& value);
	static void set_vertical_speed_time(units::TIME value);
	static void set_volume(units::VOLUME value);
	static void set_weight(units::WEIGHT value);

signals:
	void coordinates_traditionalChanged(bool value);
	void duration_unitsChanged(int value);
	void lengthChanged(int value);
	void pressureChanged(int value);
	void show_units_tableChanged(bool value);
	void temperatureChanged(int value);
	void unit_systemChanged(const QString& value);
	void vertical_speed_timeChanged(int value);
	void volumeChanged(int value);
	void weightChanged(int value);

private:
	static void disk_coordinates_traditional(bool doSync);
	static void disk_duration_units(bool doSync);
	static void disk_length(bool doSync);
	static void disk_pressure(bool doSync);
	static void disk_show_units_table(bool doSync);
	static void disk_temperature(bool doSync);
	static void disk_unit_system(bool doSync);
	static void disk_vertical_speed_time(bool doSync);
	static void disk_volume(bool doSync);
	static void disk_weight(bool doSync);
};

#endif
