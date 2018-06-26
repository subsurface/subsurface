// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H


#include <QObject>
#include "core/pref.h"
#include "qPrefAnimations.h"


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
