// SPDX-License-Identifier: GPL-2.0
#include "divesummary.h"
#include "core/qthelper.h"
#include "core/settings/qPrefUnit.h"

#include <QDateTime>

QStringList diveSummary::diveSummaryText;

timestamp_t diveSummary::firstDive, diveSummary::lastDive;
int diveSummary::dives[2], diveSummary::divesEAN[2], diveSummary::divesDeep[2], diveSummary::diveplans[2];
long diveSummary::divetime[2], diveSummary::depth[2];
long diveSummary::divetimeMax[2], diveSummary::depthMax[2], diveSummary::sacMin[2], diveSummary::sacMax[2];
long diveSummary::totalSACTime[2], diveSummary::totalSacVolume[2];
bool diveSummary::diveSummaryLimit;

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
	diveSummaryText.clear();
	diveSummaryText << "??" << "??" << QObject::tr("no dives in period") << QObject::tr("no divies in period")  << "??" <<
						 "??" << "??" << "??" <<
						 "?:??" << "?:??" << "?:??" <<
						 "?:??" << "?:??" << "?:??" <<
						 "??" << "??" << "??" << "??" << "??" <<
						 "??" << "??" << "??" << "??" << "??" << "??" << "??";

	// set oldest/newest date
	if (firstDive) {
		localTime = QDateTime::fromMSecsSinceEpoch(1000 * firstDive, Qt::UTC);
		localTime.setTimeSpec(Qt::UTC);
		diveSummaryText[0] = QStringLiteral("%1").arg(localTime.date().toString(prefs.date_format_short));
	}
	if (lastDive) {
		localTime = QDateTime::fromMSecsSinceEpoch(1000 * lastDive, Qt::UTC);
		localTime.setTimeSpec(Qt::UTC);
		diveSummaryText[1] = QStringLiteral("%1").arg(localTime.date().toString(prefs.date_format_short));
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
	divetimeMax[0] = divetimeMax[1] = depthMax[0] = depthMax[1] = 0;
	sacMin[0] = sacMin[1] = 99999;
	sacMax[0] = sacMax[1] = 0;
	totalSACTime[0] = totalSACTime[1] = 0;
	totalSacVolume[0] = totalSacVolume[1] = 0;

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
	// Check for dive validity
	// Swimmingpool dives (4 meters) are not counted, nor
	// are very short dives (5 minutes)
	if (!diveSummaryLimit && (dive->maxdepth.mm < 4000 ||
		dive->duration.seconds < 5 * 60))
		return;

	// one more real dive
	dives[inx]++;

	// sum dive in minutes and check for new max.
	divetime[inx] += dive->duration.seconds;
	if (dive->duration.seconds > divetimeMax[inx])
		divetimeMax[inx] = dive->duration.seconds;

	// sum depth in meters, check for new max. and if dive is a deep dive
	depth[inx] += dive->maxdepth.mm;
	if (dive->maxdepth.mm > depthMax[inx])
		depthMax[inx] = dive->maxdepth.mm;
	if (dive->maxdepth.mm > 39000)
		divesDeep[inx]++;

	// sum SAC, check for new min/max.
	// Do not include dives with SAC > 40 l/min (probably a free flow regulator)
	// and SAC < 4 l/min (probably a faulty transmittor)
	if (!diveSummaryLimit && dive->sac <= 40000 && dive->sac >= 4000) {
		totalSACTime[inx] += dive->duration.seconds;
		totalSacVolume[inx] += dive->sac * dive->duration.seconds;
		if (dive->sac < sacMin[inx])
			sacMin[inx] = dive->sac;
		if (dive->sac > sacMax[inx])
			sacMax[inx] = dive->sac;
	}

	// EAN dive ?
	for (int j = 0; j < dive->cylinders.nr; ++j) {
		if (dive->cylinders.cylinders[j].gasmix.o2.permille > 210) {
			divesEAN[inx]++;
			break;
		}
	}
}

static QString timeString(long duration)
{
	long hours = duration / 3600;
	long minutes = (duration - hours * 3600) / 60;
	if (hours >= 100)
		return QStringLiteral("%1h").arg(hours);
	else
		return QStringLiteral("%1:%2").arg(hours).arg(minutes, 2, 10, QChar('0'));
}

static QString depthString(long depth)
{
	return QString("%1").arg((qPrefUnits::length() == units::METERS) ? depth / 1000 : lrint(mm_to_feet(depth)));
}

static QString volumeString(long volume)
{
	return QString("%1").arg((qPrefUnits::volume() == units::LITER) ? volume / 1000 : round(100.0 * ml_to_cuft(volume)) / 100.0);
}

void diveSummary::buildStringList(int inx)
{
	if (!dives[inx]) {
		diveSummaryText[2+inx] = QObject::tr("no dives in period");
		return;
	}

	// dives
	diveSummaryText[2+inx] = QStringLiteral("%1").arg(dives[inx]);
	diveSummaryText[4+inx] = QStringLiteral("%1").arg(divesEAN[inx]);
	diveSummaryText[6+inx] = QStringLiteral("%1").arg(divesDeep[inx]);

	// time
	diveSummaryText[8+inx] = timeString(divetime[inx]);
	diveSummaryText[10+inx] = timeString(divetimeMax[inx]);
	diveSummaryText[12+inx] = timeString(divetime[inx] / dives[inx]);

	// depth
	QString unitText = (qPrefUnits::length() == units::METERS) ? " m" : " ft";
	diveSummaryText[14+inx] = depthString(depthMax[inx]) + unitText;
	diveSummaryText[16+inx] = depthString(depth[inx] / dives[inx]) + unitText;

	// SAC
	unitText = (qPrefUnits::volume() == units::LITER) ? " l/min" : " cuft/min";
	diveSummaryText[18+inx] = volumeString(sacMin[inx]) + unitText;
	diveSummaryText[20+inx] = volumeString(sacMax[inx]) + unitText;

	// finally the weighted average
	if (totalSACTime[inx]) {
		long avgSac = totalSacVolume[inx] / totalSACTime[inx];
		diveSummaryText[22+inx] = volumeString(avgSac) + unitText;
	} else {
		diveSummaryText[22+inx] = QObject::tr("no dives");
	}

	// Diveplan(s)
	diveSummaryText[24+inx] = QStringLiteral("%1").arg(diveplans[inx]);
}
