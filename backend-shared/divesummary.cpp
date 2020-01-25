// SPDX-License-Identifier: GPL-2.0
#include "divesummary.h"
#include "core/qthelper.h"
#include "core/dive.h"

#include <QDateTime>


QStringList diveSummary::resultCalculation;

timestamp_t diveSummary::firstDive, diveSummary::lastDive;
int diveSummary::dives[2], diveSummary::divesEAN[2], diveSummary::divesDeep[2], diveSummary::diveplans[2];
long diveSummary::divetime[2], diveSummary::depth[2], diveSummary::sac[2];
long diveSummary::divetimeMax[2], diveSummary::depthMax[2], diveSummary::sacMin[2];

void diveSummary::summaryCalculation(int primaryPeriod, int secondaryPeriod)
{
	// Calculate Start of the 2 periods.
	timestamp_t now, primaryStart, secondaryStart;
	now = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset();
	primaryStart = (primaryPeriod == 0) ? 0 : now - primaryPeriod * 30 * 24 * 60 * 60;
	secondaryStart = (secondaryPeriod == 0) ? 0 : now - secondaryPeriod * 30 * 24 * 60 * 60;

	resultCalculation.clear();

	// Loop over all dives and sum up data
	loopDives(primaryStart, secondaryStart);

	// change data into strings to be used in the UI
	resultCalculation << "13/12/2001" << "10/1/2020" <<
						"120" << "8" <<
					   "12"  << "1" <<
					   "50"  << "10" <<
					   "200h" << "10h" <<
					   "2:31" << "0:51" <<
					   "1:15" << "0:41" <<
					   "62m" << "31m" <<
					   "23m" << "29m" <<
					   "11 l/min" << "12 l/min" <<
					   "12 l/min" << "16 l/min" <<
					   "4" << "1";
}

void diveSummary::loopDives(timestamp_t primaryStart, timestamp_t secondaryStart)
{
	struct dive *dive;
	int i;

	// Clear summary data
	firstDive = lastDive = 0;
	dives[0] = dives[1] = divesEAN[0] = divesEAN[1] = 0;
	divesDeep[0] = divesDeep[1] = diveplans[0] = diveplans[1] = 0;
	divetime[0] = divetime[1] = depth[0] = depth[1] = 0;
	sac[0] = sac[1] = 0;
	divetimeMax[0] = divetimeMax[1] = depthMax[0] = depthMax[1] = 0;
	sacMin[0] = sacMin[1] = 99999;

	for_each_dive (i, dive) {
		// remember time of oldest and newest dive
		if (i == 0)
			firstDive = dive->when;
		if (dive->when > lastDive)
			lastDive = dive->when;

		// check if dive is newer than primaryStart (add to first column)
		if (dive->when > primaryStart) {
			if (is_dc_planner(&dive->dc))
				diveplans[0]++;
			else
				calculateDive(0, dive);
		}

		// check if dive is newer than secondaryStart (add to second column)
		if (dive->when > secondaryStart) {
			if (is_dc_planner(&dive->dc))
				diveplans[1]++;
			else
				calculateDive(1, dive);
		}
	}
}

void diveSummary::calculateDive(int inx, struct dive *dive)
{
	long temp;

	// one more real dive
	dives[inx]++;

	// sum dive in minutes and check for new max.
	temp = dive->duration.seconds / 60;
	divetime[inx] += temp;
	if (temp > divetimeMax[inx])
		divetimeMax[inx] = temp;

	// sum depth in meters, check for new max. and if dive is a deep dive
	temp = dive->maxdepth.mm / 1000;
	depth[inx] += temp;
	if (temp > depthMax[inx])
		depthMax[inx] = temp;
	if (temp > 39)
		divesDeep[inx]++;

	// sum SAC in liters, check for new max.
	temp = dive->sac / 1000;
	sac[inx] += temp;
	if (temp < sacMin[inx])
		sacMin[inx] = temp;

	// EAN dive ?
	for (int j = 0; j < dive->cylinders.nr; ++j) {
		if (dive->cylinders.cylinders[j].gasmix.o2.permille > 210)
			divesEAN[inx]++;
	}
}

