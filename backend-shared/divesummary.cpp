// SPDX-License-Identifier: GPL-2.0
#include "divesummary.h"
#include "core/dive.h"
#include "core/qthelper.h"
#include "core/settings/qPrefUnit.h"

#include <QDateTime>
#include <array>

struct Stats {
	Stats();
	int dives, divesEAN, divesDeep, diveplans;
	long divetime, depth;
	long divetimeMax, depthMax, sacMin, sacMax;
	long divetimeAvg, depthAvg, sacAvg;
	long totalSACTime, totalSacVolume;
};

Stats::Stats() :
	dives(0), divesEAN(0), divesDeep(0), diveplans(0),
	divetime(0), depth(0), divetimeMax(0), depthMax(0),
	sacMin(99999), sacMax(0), totalSACTime(0), totalSacVolume(0)
{
}

static void calculateDive(struct dive *dive, Stats &stats)
{
	if (is_dc_planner(&dive->dc)) {
		stats.diveplans++;
		return;
	}

	// one more real dive
	stats.dives++;

	// sum dive in minutes and check for new max.
	stats.divetime += dive->duration.seconds;
	if (dive->duration.seconds > stats.divetimeMax)
		stats.divetimeMax = dive->duration.seconds;

	// sum depth in meters, check for new max. and if dive is a deep dive
	stats.depth += dive->maxdepth.mm;
	if (dive->maxdepth.mm > stats.depthMax)
		stats.depthMax = dive->maxdepth.mm;
	if (dive->maxdepth.mm > 39000)
		stats.divesDeep++;

	// sum SAC, check for new min/max.
	if (dive->sac) {
		stats.totalSACTime += dive->duration.seconds;
		stats.totalSacVolume += dive->sac * dive->duration.seconds;
		if (dive->sac < stats.sacMin)
			stats.sacMin = dive->sac;
		if (dive->sac > stats.sacMax)
			stats.sacMax = dive->sac;
	}

	// EAN dive ?
	for (int j = 0; j < dive->cylinders.nr; ++j) {
		if (dive->cylinders.cylinders[j].gasmix.o2.permille > 210) {
			stats.divesEAN++;
			break;
		}
	}
}

// Returns a (first_dive, last_dive) pair
static std::array<timestamp_t, 2> loopDives(timestamp_t primaryStart, timestamp_t secondaryStart, Stats &stats0, Stats &stats1)
{
	struct dive *dive;
	int i;
	timestamp_t firstDive = 0, lastDive = 0;

	for_each_dive (i, dive) {
		// remember time of oldest and newest dive
		if (i == 0)
			firstDive = dive->when;
		if (dive->when > lastDive)
			lastDive = dive->when;

		// check if dive is newer than primaryStart (add to first column)
		if (dive->when > primaryStart)
			calculateDive(dive, stats0);

		// check if dive is newer than secondaryStart (add to second column)
		if (dive->when > secondaryStart)
			calculateDive(dive, stats1);
	}
	return { firstDive, lastDive };
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

static void buildStringList(int inx, const Stats &stats, QStringList &diveSummaryText)
{
	if (!stats.dives) {
		diveSummaryText[2+inx] = QObject::tr("no dives in period");
		return;
	}

	// dives
	diveSummaryText[2+inx] = QStringLiteral("%1").arg(stats.dives);
	diveSummaryText[4+inx] = QStringLiteral("%1").arg(stats.divesEAN);
	diveSummaryText[6+inx] = QStringLiteral("%1").arg(stats.divesDeep);

	// time
	diveSummaryText[8+inx] = timeString(stats.divetime);
	diveSummaryText[10+inx] = timeString(stats.divetimeMax);
	diveSummaryText[12+inx] = timeString(stats.divetime / stats.dives);

	// depth
	QString unitText = (qPrefUnits::length() == units::METERS) ? " m" : " ft";
	diveSummaryText[14+inx] = depthString(stats.depthMax) + unitText;
	diveSummaryText[16+inx] = depthString(stats.depth / stats.dives) + unitText;

	// SAC
	unitText = (qPrefUnits::volume() == units::LITER) ? " l/min" : " cuft/min";
	diveSummaryText[18+inx] = volumeString(stats.sacMin) + unitText;
	diveSummaryText[20+inx] = volumeString(stats.sacMax) + unitText;

	// finally the weighted average
	if (stats.totalSACTime) {
		long avgSac = stats.totalSacVolume / stats.totalSACTime;
		diveSummaryText[22+inx] = volumeString(avgSac) + unitText;
	} else {
		diveSummaryText[22+inx] = QObject::tr("no dives");
	}

	// Diveplan(s)
	diveSummaryText[24+inx] = QStringLiteral("%1").arg(stats.diveplans);
}

QStringList summaryCalculation(int primaryPeriod, int secondaryPeriod)
{
	QDateTime localTime;

	// Calculate Start of the 2 periods.
	timestamp_t now, primaryStart, secondaryStart;
	now = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset();
	primaryStart = (primaryPeriod == 0) ? 0 : now - primaryPeriod * 30 * 24 * 60 * 60;
	secondaryStart = (secondaryPeriod == 0) ? 0 : now - secondaryPeriod * 30 * 24 * 60 * 60;

	// Loop over all dives and sum up data
	Stats stats0, stats1;
	std::array<timestamp_t, 2> firstLast = loopDives(primaryStart, secondaryStart, stats0, stats1);
	timestamp_t firstDive = firstLast[0]; // One day we will be able to use C++17 structured bindings
	timestamp_t lastDive = firstLast[1];

	// prepare stringlist
	QStringList res = {
		"??", "??", QObject::tr("no dives in period"), QObject::tr("no dives in period"), "??",
		"??", "??", "??",
		"?:??", "?:??", "?:??",
		"?:??", "?:??", "?:??",
		"??", "??", "??", "??", "??",
		"??", "??", "??", "??", "??", "??", "??"
	};

	// set oldest/newest date
	if (firstDive) {
		localTime = QDateTime::fromMSecsSinceEpoch(1000 * firstDive, Qt::UTC);
		localTime.setTimeSpec(Qt::UTC);
		res[0] = QStringLiteral("%1").arg(localTime.date().toString(prefs.date_format_short));
	}
	if (lastDive) {
		localTime = QDateTime::fromMSecsSinceEpoch(1000 * lastDive, Qt::UTC);
		localTime.setTimeSpec(Qt::UTC);
		res[1] = QStringLiteral("%1").arg(localTime.date().toString(prefs.date_format_short));
	}

	// and add data
	buildStringList(0, stats0, res);
	buildStringList(1, stats1, res);
	return res;
}
