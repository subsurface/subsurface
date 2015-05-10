#ifndef DIVESITEHELPERS_H
#define DIVESITEHELPERS_H

#include "units.h"
#include <QThread>

class ReverseGeoLoockupThread : public QThread {
Q_OBJECT
public:
	static ReverseGeoLoockupThread *instance();
	void run() Q_DECL_OVERRIDE;

private:
	ReverseGeoLoockupThread(QObject *parent = 0);
};

#endif // DIVESITEHELPERS_H
