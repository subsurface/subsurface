// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H
#include "core/pref.h"
#include "ssrf-version.h"

#include <QObject>
#include <QQmlEngine>

#include "qPrefCloudStorage.h"
#include "qPrefDisplay.h"
#include "qPrefDiveComputer.h"
#include "qPrefDivePlanner.h"
#include "qPrefFacebook.h"
#include "qPrefGeneral.h"
#include "qPrefGeocoding.h"
#include "qPrefLanguage.h"
#include "qPrefLocationService.h"
#include "qPrefPartialPressureGas.h"
#include "qPrefProxy.h"
#include "qPrefTechnicalDetails.h"
#include "qPrefUnit.h"
#include "qPrefUpdateManager.h"

class qPref : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString canonical_version READ canonical_version);
	Q_PROPERTY(QString mobile_version READ mobile_version);

public:
	qPref(QObject *parent = NULL);
	static qPref *instance();

	// Load/Sync local settings (disk) and struct preference
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

	// Register QML
	void registerQML(QQmlEngine *engine);

public:
	enum cloud_status {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};
	Q_ENUM(cloud_status);

	static const QString canonical_version() { return QString(CANONICAL_VERSION_STRING); }
	static const QString mobile_version() { return QString(MOBILE_VERSION_STRING); }

private:
	static void loadSync(bool doSync);
};
#endif
