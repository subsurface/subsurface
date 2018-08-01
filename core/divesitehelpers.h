// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITEHELPERS_H
#define DIVESITEHELPERS_H

#include "units.h"
#include <QThread>

class ReverseGeoLookupThread : public QThread {
Q_OBJECT
public:
	static ReverseGeoLookupThread *instance();
	void lookup(struct dive_site *ds);
	void run();

private:
	ReverseGeoLookupThread(QObject *parent = 0);
};

#endif // DIVESITEHELPERS_H
