// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H


#include <QObject>
#include "core/pref.h"
#include "qPrefAnimations.h"
#include "qPrefCloudStorage.h"
#include "qPrefDiveComputer.h"
#include "qPrefDivePlanner.h"
#include "qPrefDisplay.h"
#include "qPrefFacebook.h"
#include "qPrefGeneral.h"
#include "qPrefGeocoding.h"
#include "qPrefLanguage.h"
#include "qPrefLocationService.h"
#include "qPrefPartialPressureGas.h"
#include "qPrefProxy.h"
#include "qPrefTechnicalDetails.h"
#include "qPrefUnits.h"
#include "qPrefUpdateManager.h"



class qPref : public QObject {
	Q_OBJECT
	Q_ENUMS(cloud_status);

public:
	qPref(QObject *parent = NULL);
	~qPref() {};
	static qPref *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

	// convinience functions
	inline void load() {loadSync(false); };
	inline void sync() {loadSync(true); };

public:
	enum cloud_status {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};


private:
	static qPref *m_instance;

	qPrefAnimations *animations;
	qPrefCloudStorage *cloudStorage;
	qPrefDisplay *display;
	qPrefDiveComputer *diveComputer;
	qPrefDivePlanner *divePlanner;
	qPrefFacebook *facebook;
	qPrefGeneral *general;
	qPrefGeocoding *geocoding;
	qPrefLanguage *language;
	qPrefLocationService *locationService;
	qPrefPartialPressureGas *partialPressureGas;
	qPrefProxy *proxy;
	qPrefTechnicalDetails *technicalDetails;
	qPrefUnits *units;
	qPrefUpdateManager *updateManager;
};

#endif
