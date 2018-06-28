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
	Q_PROPERTY(qPrefAnimations* animation MEMBER m_qPrefAnimations CONSTANT);
	Q_PROPERTY(qPrefCloudStorage* cloud_storage MEMBER m_qPrefCloudStorage CONSTANT);
	Q_PROPERTY(qPrefDisplay* display MEMBER m_qPrefDisplay CONSTANT);
	Q_PROPERTY(qPrefDiveComputer* diveComputer MEMBER m_qPrefDiveComputer CONSTANT);
	Q_PROPERTY(qPrefDivePlanner* planner MEMBER m_qPrefDivePlanner CONSTANT);
	Q_PROPERTY(qPrefFacebook* facebook MEMBER m_qPrefFacebook CONSTANT);
	Q_PROPERTY(qPrefGeocoding* geocoding MEMBER m_qPrefGeocoding CONSTANT);
	Q_PROPERTY(qPrefGeneral* general MEMBER m_qPrefGeneral CONSTANT);
	Q_PROPERTY(qPrefLanguage* language MEMBER m_qPrefLanguage CONSTANT);
	Q_PROPERTY(qPrefLocationService* Location MEMBER m_qPrefLocationService CONSTANT);
	Q_PROPERTY(qPrefPartialPressureGas* pp_gas MEMBER m_qPrefPartialPressureGas CONSTANT);
	Q_PROPERTY(qPrefProxy* proxy MEMBER m_qPrefProxy CONSTANT);
	Q_PROPERTY(qPrefTechnicalDetails* techical_details MEMBER m_qPrefTechnicalDetails CONSTANT);
	Q_PROPERTY(qPrefUnits* units MEMBER m_qPrefUnits CONSTANT);
	Q_PROPERTY(qPrefUpdateManager* updateManager MEMBER m_qPrefUpdateManager CONSTANT);

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

	qPrefAnimations *m_qPrefAnimations;
	qPrefCloudStorage *m_qPrefCloudStorage;
	qPrefDisplay *m_qPrefDisplay;
	qPrefDiveComputer *m_qPrefDiveComputer;
	qPrefDivePlanner *m_qPrefDivePlanner;
	qPrefFacebook *m_qPrefFacebook;
	qPrefGeneral *m_qPrefGeneral;
	qPrefGeocoding *m_qPrefGeocoding;
	qPrefLanguage *m_qPrefLanguage;
	qPrefLocationService *m_qPrefLocationService;
	qPrefPartialPressureGas *m_qPrefPartialPressureGas;
	qPrefProxy *m_qPrefProxy;
	qPrefTechnicalDetails *m_qPrefTechnicalDetails;
	qPrefUnits *m_qPrefUnits;
	qPrefUpdateManager *m_qPrefUpdateManager;
};

#endif
