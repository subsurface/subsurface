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
#include "qPrefProxy.h"
#include "qPrefTechnicalDetails.h"


#define TechnicalDetailsSettings qPrefTechnicalDetails

class qPref : public QObject {
	Q_OBJECT
	Q_ENUMS(cloud_status);

public:
	qPref(QObject *parent = NULL) : QObject(parent) {};
	~qPref() {};
	static qPref *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

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

};

#endif
