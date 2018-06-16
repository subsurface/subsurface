// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFUNITS_H
#define QPREFUNITS_H

#include <QObject>
#include "../units.h"


class qPrefUnits : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool		coordinates_traditional
				READ	coordinatesTraditional
				WRITE	setCoordinatesTraditional
				NOTIFY  coordinatesTraditionalChanged);
	Q_PROPERTY(units::DURATION  duration_units
						READ	durationUnits
						WRITE	setDurationUnits
						NOTIFY  durationUnitsChanged);
	Q_PROPERTY(units::LENGTH   length
						READ   length
						WRITE  setLength
				NOTIFY  lengthChanged);
	Q_PROPERTY(units::PRESSURE  pressure
						READ	pressure
						WRITE	setPressure
				NOTIFY  pressureChanged);
	Q_PROPERTY(bool		show_units_table
				READ	showUnitsTable
				WRITE	setShowUnitsTable
				NOTIFY  showUnitsTableChanged);
	Q_PROPERTY(units::TEMPERATURE  temperature
							READ   temperature
							WRITE  setTemperature
							NOTIFY temperatureChanged);
	Q_PROPERTY(QString	unit_system
				READ	unitSystem
				WRITE	setUnitSystem
				NOTIFY  unitSystemChanged);
	Q_PROPERTY(units::TIME  vertical_speed_time
					READ	verticalSpeedTime
					WRITE	setVerticalSpeedTime
					NOTIFY  verticalSpeedTimeChanged);
	Q_PROPERTY(units::VOLUME  volume
					READ      volume
					WRITE     setVolume
					NOTIFY    volumeChanged);
	Q_PROPERTY(units::WEIGHT  weight
					READ      weight
					WRITE     setWeight
					NOTIFY    weightChanged);

public:
	qPrefUnits(QObject *parent = NULL) : QObject(parent) {};
	~qPrefUnits() {};
	static qPrefUnits *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	bool coordinatesTraditional() const;
	units::DURATION durationUnits() const;
	units::LENGTH length() const;
	units::PRESSURE pressure() const;
	bool showUnitsTable() const;
	units::TEMPERATURE temperature() const;
	const QString unitSystem() const;
	units::TIME verticalSpeedTime() const;
	units::VOLUME volume() const;
	units::WEIGHT weight() const;

public slots:
	void setCoordinatesTraditional(bool value);
	void setDurationUnits(units::DURATION value);
	void setLength(units::LENGTH value);
	void setPressure(units::PRESSURE value);
	void setShowUnitsTable(bool value);
	void setTemperature(units::TEMPERATURE value);
	void setUnitSystem(const QString& value);
	void setVerticalSpeedTime(units::TIME value);
	void setVolume(units::VOLUME value);
	void setWeight(units::WEIGHT value);

signals:
	void coordinatesTraditionalChanged(bool value);
	void durationUnitsChanged(units::DURATION value);
	void lengthChanged(units::LENGTH value);
	void pressureChanged(units::PRESSURE value);
	void showUnitsTableChanged(bool value);
	void temperatureChanged(units::TEMPERATURE value);
	void unitSystemChanged(const QString& value);
	void verticalSpeedTimeChanged(units::TIME value);
	void volumeChanged(units::VOLUME value);
	void weightChanged(units::WEIGHT value);

private:
	const QString group = QStringLiteral("Units");
	static qPrefUnits *m_instance;

	void diskCoordinatesTraditional(bool doSync);
	void diskDurationUnits(bool doSync);
	void diskLength(bool doSync);
	void diskPressure(bool doSync);
	void diskShowUnitsTable(bool doSync);
	void diskTemperature(bool doSync);
	void diskUnitSystem(bool doSync);
	void diskVerticalSpeedTime(bool doSync);
	void diskVolume(bool doSync);
	void diskWeight(bool doSync);
};

#endif
