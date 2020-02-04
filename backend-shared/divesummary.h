// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESUMMARY_H
#define DIVESUMMARY_H
#include <QStringList>
#include <array>
#include "core/dive.h"

class diveSummary {

public:
	static void summaryCalculation(int primaryPeriod, int secondaryPeriod);

	static QStringList diveSummaryText;

private:
	diveSummary() {}

	struct Stats {
		Stats();
		int dives, divesEAN, divesDeep, diveplans;
		long divetime, depth;
		long divetimeMax, depthMax, sacMin, sacMax;
		long divetimeAvg, depthAvg, sacAvg;
		long totalSACTime, totalSacVolume;
	};

	// Returns a (first_dive, last_dive) pair
	static std::array<timestamp_t, 2> loopDives(timestamp_t primaryStart, timestamp_t secondaryStart, Stats &stats0, Stats &stats1);
	static void calculateDive(struct dive *dive, Stats &stats);
	static void buildStringList(int inx, const Stats &stats);
};
#endif // DIVESUMMARY_H
