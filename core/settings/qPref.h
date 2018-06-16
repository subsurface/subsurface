// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H


#include <QObject>
#include "core/pref.h"



class qPref : public QObject {
	Q_OBJECT

public:
	qPref(QObject *parent = NULL) : QObject(parent) {};
	~qPref() {};
	static qPref *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:


private:
	static qPref *m_instance;

};

#endif
