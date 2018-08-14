// SPDX-License-Identifier: GPL-2.0
#ifndef SETTINGSOBJECTWRAPPER_H
#define SETTINGSOBJECTWRAPPER_H

#include <QObject>
#include <QDate>

#include "core/pref.h"
#include "core/settings/qPref.h"

/* Wrapper class for the Settings. This will allow
 * seamlessy integration of the settings with the QML
 * and QWidget frontends. This class will be huge, since
 * I need tons of properties, one for each option. */

class SettingsObjectWrapper : public QObject {
	Q_OBJECT

	Q_PROPERTY(qPrefTechnicalDetails*   techical_details MEMBER techDetails CONSTANT)
	Q_PROPERTY(qPrefPartialPressureGas* pp_gas           MEMBER pp_gas CONSTANT)
	Q_PROPERTY(qPrefFacebook*           facebook         MEMBER facebook CONSTANT)
	Q_PROPERTY(qPrefGeocoding*       geocoding        MEMBER geocoding CONSTANT)
	Q_PROPERTY(qPrefProxy*              proxy            MEMBER proxy CONSTANT)
	Q_PROPERTY(qPrefCloudStorage*       cloud_storage    MEMBER cloud_storage CONSTANT)
	Q_PROPERTY(qPrefDivePlanner*        planner          MEMBER planner_settings CONSTANT)
	Q_PROPERTY(qPrefUnits*              units            MEMBER unit_settings CONSTANT)
	Q_PROPERTY(qPrefGeneral*         general   MEMBER general_settings CONSTANT)
	Q_PROPERTY(qPrefDisplay*         display   MEMBER display_settings CONSTANT)
	Q_PROPERTY(qPrefLanguage*        language  MEMBER language_settings CONSTANT)
	Q_PROPERTY(qPrefAnimations*      animation MEMBER animation_settings CONSTANT)
	Q_PROPERTY(qPrefLocationService* Location  MEMBER location_settings CONSTANT)

	Q_PROPERTY(qPrefUpdateManager* update MEMBER update_manager_settings CONSTANT)
	Q_PROPERTY(qPrefDiveComputer* dive_computer MEMBER dive_computer_settings CONSTANT)
public:
	static SettingsObjectWrapper *instance();

	qPrefTechnicalDetails *techDetails;
	qPrefPartialPressureGas *pp_gas;
	qPrefFacebook *facebook;
	qPrefGeocoding *geocoding;
	qPrefProxy *proxy;
	qPrefCloudStorage *cloud_storage;
	qPrefDivePlanner *planner_settings;
	qPrefUnits *unit_settings;
	qPrefGeneral *general_settings;
	qPrefDisplay *display_settings;
	qPrefLanguage *language_settings;
	qPrefAnimations *animation_settings;
	qPrefLocationService *location_settings;
	qPrefUpdateManager *update_manager_settings;
	qPrefDiveComputer *dive_computer_settings;

private:
	SettingsObjectWrapper(QObject *parent = NULL);
};

#endif
