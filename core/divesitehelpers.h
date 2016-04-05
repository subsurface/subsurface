#ifndef DIVESITEHELPERS_H
#define DIVESITEHELPERS_H

#include "units.h"
#include <QThread>

class ReverseGeoLookupThread : public QThread {
Q_OBJECT
public:
	static ReverseGeoLookupThread *instance();
	void lookup(struct dive_site *ds);
	void run() Q_DECL_OVERRIDE;

private:
	ReverseGeoLookupThread(QObject *parent = 0);
};

#endif // DIVESITEHELPERS_H
