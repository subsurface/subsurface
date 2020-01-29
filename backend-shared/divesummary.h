// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESUMMARY_H
#define DIVESUMMARY_H
#include <QStringList>
#include "core/dive.h"


class diveSummary {

public:
	static void summaryCalculation(int primaryPeriod, int secondaryPeriod);

	static QStringList diveSummaryText;

private:
	diveSummary() {}

	static void loopDives(timestamp_t primaryStart, timestamp_t secondaryStart);
	static void calculateDive(int inx, struct dive *dive);
	static void buildStringList(int inx);

	static timestamp_t firstDive, lastDive;
	static int dives[2], divesEAN[2], divesDeep[2], diveplans[2];
	static long divetime[2], depth[2];
	static long divetimeMax[2], depthMax[2], sacMin[2], sacMax[2];
	static long divetimeAvg[2], depthAvg[2], sacAvg[2];
	static long totalSACTime[2], totalSacVolume[2];
};
#endif // DIVESUMMARY_H
