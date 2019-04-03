// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPrefPrivate.h"
#include "qPrefCloudStorage.h"
#include "qPrefDisplay.h"
#include "qPrefDiveComputer.h"
#include "qPrefDivePlanner.h"
#include "qPrefGeneral.h"
#include "qPrefGeocoding.h"
#include "qPrefLanguage.h"
#include "qPrefLocationService.h"
#include "qPrefPartialPressureGas.h"
#include "qPrefProxy.h"
#include "qPrefTechnicalDetails.h"
#include "qPrefUnit.h"
#include "qPrefUpdateManager.h"

#include <QtQml>
#include <QQmlContext>

void qPref::loadSync(bool doSync)
{
	if (!doSync)
		uiLanguage(NULL);

	// the following calls, ensures qPref* is instanciated, registred and
	// that properties are loaded
	qPrefCloudStorage::loadSync(doSync);
	qPrefDisplay::loadSync(doSync);
	qPrefDiveComputer::loadSync(doSync);
	qPrefDivePlanner::loadSync(doSync);
	qPrefGeneral::loadSync(doSync);
	qPrefGeocoding::loadSync(doSync);
	qPrefLanguage::loadSync(doSync);
	qPrefLocationService::loadSync(doSync);
	qPrefPartialPressureGas::loadSync(doSync);
	qPrefProxy::loadSync(doSync);
	qPrefTechnicalDetails::loadSync(doSync);
	qPrefUnits::loadSync(doSync);
	qPrefUpdateManager::loadSync(doSync);
}

Q_DECLARE_METATYPE(deco_mode);
Q_DECLARE_METATYPE(def_file_behavior);
Q_DECLARE_METATYPE(taxonomy_category);
void qPref::registerQML(QQmlEngine *engine)
{
	if (engine) {
		QQmlContext *ct = engine->rootContext();

		ct->setContextProperty("PrefCloudStorage", qPrefCloudStorage::instance());
		ct->setContextProperty("PrefDisplay", qPrefDisplay::instance());
		ct->setContextProperty("PrefDiveComputer", qPrefDiveComputer::instance());
		ct->setContextProperty("PrefDivePlanner", qPrefDivePlanner::instance());
		ct->setContextProperty("PrefGeneral", qPrefGeneral::instance());
		ct->setContextProperty("PrefGeocoding", qPrefGeocoding::instance());
		ct->setContextProperty("PrefLanguage", qPrefLanguage::instance());
		ct->setContextProperty("PrefLocationService", qPrefLocationService::instance());
		ct->setContextProperty("PrefPartialPressureGas", qPrefPartialPressureGas::instance());
		ct->setContextProperty("PrefProxy", qPrefProxy::instance());
		ct->setContextProperty("PrefTechnicalDetails", qPrefTechnicalDetails::instance());
		ct->setContextProperty("PrefUnits", qPrefUnits::instance());
		ct->setContextProperty("PrefUpdateManager", qPrefUpdateManager::instance());
	}

	// Register special types
	qmlRegisterUncreatableType<qPrefCloudStorage>("org.subsurfacedivelog.mobile",1,0,"CloudStatus","Enum is not a type");
	qRegisterMetaType<deco_mode>();
	qRegisterMetaType<def_file_behavior>();
	qRegisterMetaType<taxonomy_category>();
}
