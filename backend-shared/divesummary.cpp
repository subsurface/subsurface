// SPDX-License-Identifier: GPL-2.0
#include "divesummary.h"
#include "core/qthelper.h"
#include "core/dive.h"
#include "core/settings/qPrefUnit.h"

#include <QDateTime>
#include <QTextStream>

QStringList diveSummary::resultCalculation;

timestamp_t diveSummary::firstDive, diveSummary::lastDive;
int diveSummary::dives[2], diveSummary::divesEAN[2], diveSummary::divesDeep[2], diveSummary::diveplans[2];
long diveSummary::divetime[2], diveSummary::depth[2], diveSummary::sac[2];
long diveSummary::divetimeMax[2], diveSummary::depthMax[2], diveSummary::sacMin[2];

void diveSummary::summaryCalculation(int primaryPeriod, int secondaryPeriod)
{
	QDateTime localTime;

	// Calculate Start of the 2 periods.
	timestamp_t now, primaryStart, secondaryStart;
	now = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset();
	primaryStart = (primaryPeriod == 0) ? 0 : now - primaryPeriod * 30 * 24 * 60 * 60;
	secondaryStart = (secondaryPeriod == 0) ? 0 : now - secondaryPeriod * 30 * 24 * 60 * 60;

	// Loop over all dives and sum up data
	loopDives(primaryStart, secondaryStart);

	// prepare stringlist
	resultCalculation.clear();
	resultCalculation << "??" << "??" << "??" << "??" << "??" <<
						 "??" << "??" << "??" <<
						 "?:??" << "?:??" << "?:??" <<
						 "?:??" << "?:??" << "?:??" <<
						 "??" << "??" << "??" << "??" << "??" <<
						 "??" << "??" << "??" << "??" << "??";

	// set oldest/newest date
	if (firstDive) {
		localTime = QDateTime::fromMSecsSinceEpoch(1000 * firstDive, Qt::UTC);
		localTime.setTimeSpec(Qt::UTC);
		resultCalculation[0] = QStringLiteral("%1").arg(localTime.date().toString(prefs.date_format_short));
	}
	if (lastDive) {
		localTime = QDateTime::fromMSecsSinceEpoch(1000 * lastDive, Qt::UTC);
		localTime.setTimeSpec(Qt::UTC);
		resultCalculation[1] = QStringLiteral("%1").arg(localTime.date().toString(prefs.date_format_short));
	}

	// and add data
	buildStringList(0);
	buildStringList(1);
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

void diveSummary::buildStringList(int inx)
{
	int temp1, temp2;
	QString tempStr;

	if (!dives[inx])
		return;

	// dives
	resultCalculation[2+inx] = QStringLiteral("%1").arg(dives[inx]);
	resultCalculation[4+inx] = QStringLiteral("%1").arg(divesEAN[inx]);
	resultCalculation[6+inx] = QStringLiteral("%1").arg(divesDeep[inx]);

	// time
	temp1 = divetime[inx] / 60;
	temp2 = divetime[inx] - temp1 * 60;
	if (temp1 >= 100)
		resultCalculation[8+inx] = QStringLiteral("%1h").arg(temp1);
	else
		resultCalculation[8+inx] = QStringLiteral("%1:%2").arg(temp1).arg(temp2);
	temp1 = divetimeMax[inx] / 60;
	temp2 = divetimeMax[inx] - temp1 * 60;
	resultCalculation[10+inx] = QStringLiteral("%1:%2").arg(temp1).arg(temp2);
	temp2 = divetime[inx] / dives[inx];
	temp1 = temp2 / 60;
	temp2 = temp2 - temp1 * 60;
	resultCalculation[12+inx] = QStringLiteral("%1:%2").arg(temp1).arg(temp2);

	// depth
	tempStr = (qPrefUnits::length() == units::METERS) ? "m" : "ft";
	resultCalculation[14+inx] = QStringLiteral("%1%2").arg(depthMax[inx]).arg(tempStr);
	temp1 = depth[inx] / dives[inx];
	resultCalculation[16+inx] = QStringLiteral("%1%2").arg(temp1).arg(tempStr);

	// SAC
	tempStr = (qPrefUnits::volume() == units::LITER) ? "l/min" : "cuft/min";
	resultCalculation[18+inx] = QStringLiteral("%1 %2").arg(sacMin[inx]).arg(tempStr);
	temp1 = depth[inx] / dives[inx];
	resultCalculation[20+inx] = QStringLiteral("%1%2").arg(temp1).arg(tempStr);

	// Diveplan(s)
	resultCalculation[22+inx] = QStringLiteral("%1").arg(diveplans[inx]);
}
