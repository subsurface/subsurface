// SPDX-License-Identifier: GPL-2.0
#include "SettingsObjectWrapper.h"
#include <QSettings>
#include <QApplication>
#include <QFont>
#include <QDate>

#include "core/qthelper.h"
#include "core/prefs-macros.h"


SettingsObjectWrapper::SettingsObjectWrapper(QObject* parent):
QObject(parent),
	techDetails(new qPrefTechnicalDetails(this)),
	pp_gas(new qPrefPartialPressureGas(this)),
	facebook(new qPrefFacebook(this)),
	geocoding(new qPrefGeocoding(this)),
	proxy(new qPrefProxy(this)),
	cloud_storage(new qPrefCloudStorage(this)),
	planner_settings(new qPrefDivePlanner(this)),
	unit_settings(new qPrefUnits(this)),
	general_settings(new qPrefGeneral(this)),
	display_settings(new qPrefDisplay(this)),
	language_settings(new qPrefLanguage(this)),
	animation_settings(new qPrefAnimations(this)),
	location_settings(new qPrefLocationService(this)),
	update_manager_settings(new qPrefUpdateManager(this)),
	dive_computer_settings(new qPrefDiveComputer(this))
{
}

SettingsObjectWrapper* SettingsObjectWrapper::instance()
{
	static SettingsObjectWrapper settings;
	return &settings;
}
