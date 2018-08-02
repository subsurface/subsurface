// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFUNIT_H
#define QPREFUNIT_H
#include "core/pref.h"

#include <QObject>


class qPrefUnits : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool coordinates_traditional READ coordinates_traditional WRITE set_coordinates_traditional NOTIFY coordinates_traditional_changed);
	Q_PROPERTY(units::DURATION duration_units READ duration_units WRITE set_duration_units NOTIFY duration_units_changed);
	Q_PROPERTY(units::LENGTH length READ length WRITE set_length NOTIFY length_changed);
	Q_PROPERTY(units::PRESSURE pressure READ pressure WRITE set_pressure NOTIFY pressure_changed);
	Q_PROPERTY(bool show_units_table READ show_units_table WRITE set_show_units_table NOTIFY show_units_table_changed);
	Q_PROPERTY(units::TEMPERATURE temperature READ temperature WRITE set_temperature NOTIFY temperature_changed);
	Q_PROPERTY(QString unit_system READ unit_system WRITE set_unit_system NOTIFY unit_system_changed);
	Q_PROPERTY(units::TIME vertical_speed_time READ vertical_speed_time WRITE set_vertical_speed_time NOTIFY vertical_speed_time_changed);
	Q_PROPERTY(units::VOLUME volume READ volume WRITE set_volume NOTIFY volume_changed);
	Q_PROPERTY(units::WEIGHT weight READ weight WRITE set_weight NOTIFY weight_changed);

public:
	qPrefUnits(QObject *parent = NULL);
	static qPrefUnits *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

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
	void set_coordinates_traditional(bool value);
	void set_duration_units(units::DURATION value);
	void set_length(units::LENGTH value);
	void set_pressure(units::PRESSURE value);
	void set_show_units_table(bool value);
	void set_temperature(units::TEMPERATURE value);
	void set_unit_system(const QString& value);
	void set_vertical_speed_time(units::TIME value);
	void set_volume(units::VOLUME value);
	void set_weight(units::WEIGHT value);

signals:
	void coordinates_traditional_changed(bool value);
	void duration_units_changed(int value);
	void length_changed(int value);
	void pressure_changed(int value);
	void show_units_table_changed(bool value);
	void temperature_changed(int value);
	void unit_system_changed(const QString& value);
	void vertical_speed_time_changed(int value);
	void volume_changed(int value);
	void weight_changed(int value);

private:
	void disk_coordinates_traditional(bool doSync);
	void disk_duration_units(bool doSync);
	void disk_length(bool doSync);
	void disk_pressure(bool doSync);
	void disk_show_units_table(bool doSync);
	void disk_temperature(bool doSync);
	void disk_unit_system(bool doSync);
	void disk_vertical_speed_time(bool doSync);
	void disk_volume(bool doSync);
	void disk_weight(bool doSync);
};

#endif
