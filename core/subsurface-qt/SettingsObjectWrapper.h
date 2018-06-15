// SPDX-License-Identifier: GPL-2.0
#ifndef SETTINGSOBJECTWRAPPER_H
#define SETTINGSOBJECTWRAPPER_H

#include <QObject>
#include <QDate>

#include "../pref.h"
#include "../prefs-macros.h"
#include "../settings/qPref.h"

/* Wrapper class for the Settings. This will allow
 * seamlessy integration of the settings with the QML
 * and QWidget frontends. This class will be huge, since
 * I need tons of properties, one for each option. */

class UpdateManagerSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool dont_check_for_updates READ dontCheckForUpdates WRITE setDontCheckForUpdates NOTIFY dontCheckForUpdatesChanged)
	Q_PROPERTY(QString last_version_used READ lastVersionUsed WRITE setLastVersionUsed NOTIFY lastVersionUsedChanged)
	Q_PROPERTY(QDate next_check READ nextCheck WRITE nextCheckChanged)
public:
	UpdateManagerSettings(QObject *parent);
	bool dontCheckForUpdates() const;
	bool dontCheckExists() const;
	QString lastVersionUsed() const;
	QDate nextCheck() const;

public slots:
	void setDontCheckForUpdates(bool value);
	void setLastVersionUsed(const QString& value);
	void setNextCheck(const QDate& date);

signals:
	void dontCheckForUpdatesChanged(bool value);
	void lastVersionUsedChanged(const QString& value);
	void nextCheckChanged(const QDate& date);
private:
	const QString group = QStringLiteral("UpdateManager");
};

/* Control the state of the Partial Pressure Graphs preferences */
class PartialPressureGasSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool   show_po2          READ showPo2         WRITE setShowPo2         NOTIFY showPo2Changed)
	Q_PROPERTY(bool   show_pn2          READ showPn2         WRITE setShowPn2         NOTIFY showPn2Changed)
	Q_PROPERTY(bool   show_phe	    READ showPhe         WRITE setShowPhe         NOTIFY showPheChanged)
	Q_PROPERTY(double po2_threshold_min READ po2ThresholdMin WRITE setPo2ThresholdMin NOTIFY po2ThresholdMinChanged)
	Q_PROPERTY(double po2_threshold_max READ po2ThresholdMax WRITE setPo2ThresholdMax NOTIFY po2ThresholdMaxChanged)
	Q_PROPERTY(double pn2_threshold     READ pn2Threshold    WRITE setPn2Threshold    NOTIFY pn2ThresholdChanged)
	Q_PROPERTY(double phe_threshold     READ pheThreshold    WRITE setPheThreshold    NOTIFY pheThresholdChanged)

public:
	PartialPressureGasSettings(QObject *parent);
	bool showPo2() const;
	bool showPn2() const;
	bool showPhe() const;
	double po2ThresholdMin() const;
	double po2ThresholdMax() const;
	double pn2Threshold() const;
	double pheThreshold() const;

public slots:
	void setShowPo2(bool value);
	void setShowPn2(bool value);
	void setShowPhe(bool value);
	void setPo2ThresholdMin(double value);
	void setPo2ThresholdMax(double value);
	void setPn2Threshold(double value);
	void setPheThreshold(double value);

signals:
	void showPo2Changed(bool value);
	void showPn2Changed(bool value);
	void showPheChanged(bool value);
	void po2ThresholdMaxChanged(double value);
	void po2ThresholdMinChanged(double value);
	void pn2ThresholdChanged(double value);
	void pheThresholdChanged(double value);

private:
	const QString group = QStringLiteral("TecDetails");
};



class UnitsSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(int length           READ length                 WRITE setLength                 NOTIFY lengthChanged)
	Q_PROPERTY(int pressure       READ pressure               WRITE setPressure               NOTIFY pressureChanged)
	Q_PROPERTY(int volume           READ volume                 WRITE setVolume                 NOTIFY volumeChanged)
	Q_PROPERTY(int temperature READ temperature            WRITE setTemperature            NOTIFY temperatureChanged)
	Q_PROPERTY(int weight           READ weight                 WRITE setWeight                 NOTIFY weightChanged)
	Q_PROPERTY(QString unit_system            READ unitSystem             WRITE setUnitSystem             NOTIFY unitSystemChanged)
	Q_PROPERTY(bool coordinates_traditional   READ coordinatesTraditional WRITE setCoordinatesTraditional NOTIFY coordinatesTraditionalChanged)
	Q_PROPERTY(int vertical_speed_time READ verticalSpeedTime    WRITE setVerticalSpeedTime    NOTIFY verticalSpeedTimeChanged)
	Q_PROPERTY(int duration_units          READ durationUnits        WRITE setDurationUnits         NOTIFY durationUnitChanged)
	Q_PROPERTY(bool show_units_table          READ showUnitsTable        WRITE setShowUnitsTable         NOTIFY showUnitsTableChanged)

public:
	UnitsSettings(QObject *parent = 0);
	int length() const;
	int pressure() const;
	int volume() const;
	int temperature() const;
	int weight() const;
	int verticalSpeedTime() const;
	int durationUnits() const;
	bool showUnitsTable() const;
	QString unitSystem() const;
	bool coordinatesTraditional() const;

public slots:
	void setLength(int value);
	void setPressure(int value);
	void setVolume(int value);
	void setTemperature(int value);
	void setWeight(int value);
	void setVerticalSpeedTime(int value);
	void setDurationUnits(int value);
	void setShowUnitsTable(bool value);
	void setUnitSystem(const QString& value);
	void setCoordinatesTraditional(bool value);

signals:
	void lengthChanged(int value);
	void pressureChanged(int value);
	void volumeChanged(int value);
	void temperatureChanged(int value);
	void weightChanged(int value);
	void verticalSpeedTimeChanged(int value);
	void unitSystemChanged(const QString& value);
	void coordinatesTraditionalChanged(bool value);
	void durationUnitChanged(int value);
	void showUnitsTableChanged(bool value);
private:
	const QString group = QStringLiteral("Units");
};


class SettingsObjectWrapper : public QObject {
	Q_OBJECT

	Q_PROPERTY(qPrefTechnicalDetails*   techical_details MEMBER techDetails CONSTANT)
	Q_PROPERTY(PartialPressureGasSettings* pp_gas           MEMBER pp_gas CONSTANT)
	Q_PROPERTY(qPrefFacebook*           facebook         MEMBER facebook CONSTANT)
	Q_PROPERTY(qPrefGeocoding*       geocoding        MEMBER geocoding CONSTANT)
	Q_PROPERTY(qPrefCloudStorage*          cloud_storage    MEMBER cloud_storage CONSTANT)
	Q_PROPERTY(qPrefProxy*              proxy            MEMBER proxy CONSTANT)
	Q_PROPERTY(qPrefDivePlanner*        planner          MEMBER planner_settings CONSTANT)
	Q_PROPERTY(UnitsSettings*              units            MEMBER unit_settings CONSTANT)
	Q_PROPERTY(qPrefDisplay*         display   MEMBER display_settings CONSTANT)
	Q_PROPERTY(qPrefGeneral*         general   MEMBER general_settings CONSTANT)
	Q_PROPERTY(qPrefLanguage*        language  MEMBER language_settings CONSTANT)
	Q_PROPERTY(qPrefAnimations*      animation MEMBER animation_settings CONSTANT)
	Q_PROPERTY(qPrefLocationService* Location  MEMBER location_settings CONSTANT)

	Q_PROPERTY(UpdateManagerSettings* update MEMBER update_manager_settings CONSTANT)
	Q_PROPERTY(qPrefDiveComputer* dive_computer MEMBER dive_computer_settings CONSTANT)
public:
	static SettingsObjectWrapper *instance();

	qPrefTechnicalDetails *techDetails;
	PartialPressureGasSettings *pp_gas;
	qPrefFacebook *facebook;
	qPrefGeocoding *geocoding;
	qPrefCloudStorage *cloud_storage;
	qPrefProxy *proxy;
	qPrefDivePlanner *planner_settings;
	UnitsSettings *unit_settings;
	qPrefGeneral *general_settings;
	qPrefDisplay *display_settings;
	qPrefLanguage *language_settings;
	qPrefAnimations *animation_settings;
	qPrefLocationService *location_settings;
	UpdateManagerSettings *update_manager_settings;
	qPrefDiveComputer *dive_computer_settings;

	void sync();
	void load();
private:
	SettingsObjectWrapper(QObject *parent = NULL);
};

#endif
