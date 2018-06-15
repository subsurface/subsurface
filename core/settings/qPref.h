// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H

#include <QObject>
#include <QSettings>
#include "core/pref.h"

#include "qPrefDisplay.h"

class qPref : public QObject {
	Q_OBJECT
	Q_ENUMS(cloud_status);

public:
	qPref(QObject *parent = NULL);
	static qPref *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:

private:
};

#endif
