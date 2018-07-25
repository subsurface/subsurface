// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H

#include "core/pref.h"
#include <QObject>

#include "qPrefAnimations.h"
#include "qPrefCloudStorage.h"
#include "qPrefDisplay.h"
#include "qPrefDiveComputer.h"

class qPref : public QObject {
	Q_OBJECT
	Q_ENUMS(cloud_status);
	Q_PROPERTY(QString canonical_version READ canonical_version);
	Q_PROPERTY(QString mobile_version READ mobile_version);

public:
	qPref(QObject *parent = NULL);
	static qPref *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void inline load() { loadSync(false); }
	void inline sync() { loadSync(true); }

public:
	enum cloud_status {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};

	const QString canonical_version() const;
	const QString mobile_version() const;
};

#endif
