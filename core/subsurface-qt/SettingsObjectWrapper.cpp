// SPDX-License-Identifier: GPL-2.0
#include "SettingsObjectWrapper.h"
#include <QSettings>
#include <QApplication>
#include <QFont>
#include <QDate>
#include <QNetworkProxy>

#include "core/qthelper.h"



SettingsObjectWrapper::SettingsObjectWrapper(QObject* parent):
QObject(parent),
	techDetails(new qPrefTechnicalDetails(this)),
	pp_gas(new qPrefPartialPressureGas(this)),
	facebook(new qPrefFacebook(this)),
	geocoding(new qPrefGeocoding(this)),
	cloud_storage(new qPrefCloudStorage(this)),
	proxy(new qPrefProxy(this)),
	planner_settings(new qPrefDivePlanner(this)),
	general_settings(new qPrefGeneral(this)),
	unit_settings(new qPrefUnits(this)),
	display_settings(new qPrefDisplay(this)),
	language_settings(new qPrefLanguage(this)),
	animation_settings(new qPrefAnimations(this)),
	location_settings(new qPrefLocationService(this)),
	update_manager_settings(new qPrefUpdateManager(this)),
	dive_computer_settings(new qPrefDiveComputer(this))
{
}

void SettingsObjectWrapper::load()
{
	uiLanguage(NULL);
	qPref::instance()->load();
}

void SettingsObjectWrapper::sync()
{
	qPref::instance()->sync();
}

SettingsObjectWrapper* SettingsObjectWrapper::instance()
{
	static SettingsObjectWrapper settings;
	return &settings;
}
