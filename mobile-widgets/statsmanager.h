// SPDX-License-Identifier: GPL-2.0
#ifndef STATSMANAGER_H
#define STATSMANAGER_H

#include "stats/statsview.h"
#include "stats/statsstate.h"

class StatsManager : public QObject {
	Q_OBJECT
public:
	StatsManager();
	~StatsManager();
	Q_INVOKABLE void init(StatsView *v);
	Q_INVOKABLE void doit();
private:
	StatsView *view;
	StatsState state;
};

#endif
