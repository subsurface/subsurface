// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divesummarymodel.h"
#include "core/dive.h"
#include "core/qthelper.h"

#include <QLocale>
#include <QDateTime>

int DiveSummaryModel::rowCount(const QModelIndex &) const
{
	return NUM_ROW;
}

int DiveSummaryModel::columnCount(const QModelIndex &) const
{
	return (int)results.size();
}

QHash<int, QByteArray> DiveSummaryModel::roleNames() const
{
	return { { HEADER_ROLE, "header" },
		 { COLUMN0_ROLE, "col0"  },
		 { COLUMN1_ROLE, "col1"  },
		 { SECTION_ROLE, "section" } };
}

QVariant DiveSummaryModel::dataDisplay(int row, int col) const
{
	if (col >= (int)results.size())
		return QVariant();
	const Result &res = results[col];

	switch (row) {
	case DIVES: return res.dives;
	case DIVES_EAN: return res.divesEAN;
	case DIVES_DEEP: return res.divesDeep;
	case PLANS: return res.plans;
	case TIME: return res.time;
	case TIME_MAX: return res.time_max;
	case TIME_AVG: return res.time_avg;
	case DEPTH_MAX: return res.depth_max;
	case DEPTH_AVG: return res.depth_avg;
	case SAC_MIN: return res.sac_min;
	case SAC_MAX: return res.sac_max;
	case SAC_AVG: return res.sac_avg;
	}

	return QVariant();
}

QVariant DiveSummaryModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole) // The "normal" case
		return dataDisplay(index.row(), index.column());

	// The QML case
	int row = index.row();
	switch (role) {
	case HEADER_ROLE:
		return headerData(row, Qt::Vertical, Qt::DisplayRole);
	case COLUMN0_ROLE:
		return dataDisplay(row, 0);
	case COLUMN1_ROLE:
		return dataDisplay(row, 1);
	case SECTION_ROLE:
		switch (row) {
		case DIVES ... PLANS: return tr("Number of dives");
		case TIME ... TIME_AVG: return tr("Time");
		case DEPTH_MAX ... DEPTH_AVG: return tr("Depth");
		case SAC_MIN ... SAC_AVG: return tr("SAC");
		default: return QVariant();
		}
	}

	// The unsupported case
	return QVariant();
}

QVariant DiveSummaryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Vertical || role != Qt::DisplayRole)
		return QVariant();

	switch (section) {
	case DIVES: return tr("Total");
	case DIVES_EAN: return tr("EAN dives");
	case DIVES_DEEP: return tr("Deep dives (> 39 m)");
	case PLANS: return tr("Dive plan(s)");
	case TIME: return tr("Total time");
	case TIME_MAX: return tr("Max Time");
	case TIME_AVG: return tr("Avg time");
	case DEPTH_MAX: return tr("Max depth");
	case DEPTH_AVG: return tr("Avg max depth");
	case SAC_MIN: return tr("Min SAC");
	case SAC_MAX: return tr("Max SAC");
	case SAC_AVG: return tr("Avg SAC");
	}
	return QVariant();
}

struct Stats {
	Stats();
	int dives, divesEAN, divesDeep, diveplans;
	int64_t divetime, depth;
	int64_t divetimeMax, depthMax, sacMin, sacMax;
	int64_t divetimeAvg, depthAvg, sacAvg;
	int64_t totalSACTime, totalSacVolume;
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
		if (get_cylinder(dive, j)->gasmix.o2.permille > 210) {
			stats.divesEAN++;
			break;
		}
	}
}

// Returns a (first_dive, last_dive) pair
static Stats loopDives(timestamp_t start)
{
	Stats stats;
	struct dive *dive;
	int i;

	for_each_dive (i, dive) {
		// check if dive is newer than primaryStart (add to first column)
		if (dive->when > start)
			calculateDive(dive, stats);
	}
	return stats;
}

static QString timeString(int64_t duration)
{
	int64_t hours = duration / 3600;
	int64_t minutes = (duration - hours * 3600) / 60;
	if (hours >= 100)
		return QStringLiteral("%1 h").arg(hours);
	else
		return QStringLiteral("%1:%2").arg(hours).arg(minutes, 2, 10, QChar('0'));
}

static QString depthString(int64_t depth)
{
	return QStringLiteral("%L1").arg(prefs.units.length == units::METERS ? depth / 1000 : lrint(mm_to_feet(depth)));
}

static QString volumeString(int64_t volume)
{
	return QStringLiteral("%L1").arg(prefs.units.volume == units::LITER ? volume / 1000 : round(100.0 * ml_to_cuft(volume)) / 100.0);
}

static DiveSummaryModel::Result formatResults(const Stats &stats)
{
	DiveSummaryModel::Result res;
	if (!stats.dives) {
		res.dives = QObject::tr("no dives in period");
		res.divesEAN = res.divesDeep = res.plans = QStringLiteral("0");
		res.time = res.time_max = res.time_avg = QStringLiteral("0:00");
		res.depth_max = res.depth_avg = QStringLiteral("-");
		res.sac_min = res.sac_max = res.sac_avg = QStringLiteral("-");
		return res;
	}

	// dives
	QLocale loc;
	res.dives = loc.toString(stats.dives);
	res.divesEAN = loc.toString(stats.divesEAN);
	res.divesDeep = loc.toString(stats.divesDeep);

	// time
	res.time = timeString(stats.divetime);
	res.time_max = timeString(stats.divetimeMax);
	res.time_avg = timeString(stats.divetime / stats.dives);

	// depth
	QString unitText = (prefs.units.length == units::METERS) ? " m" : " ft";
	res.depth_max = depthString(stats.depthMax) + unitText;
	res.depth_avg = depthString(stats.depth / stats.dives) + unitText;

	// SAC
	if (stats.totalSACTime) {
		unitText = (prefs.units.volume == units::LITER) ? " l/min" : " cuft/min";
		int64_t avgSac = stats.totalSacVolume / stats.totalSACTime;
		res.sac_avg = volumeString(avgSac) + unitText;
		res.sac_min = volumeString(stats.sacMin) + unitText;
		res.sac_max = volumeString(stats.sacMax) + unitText;
	} else {
		res.sac_avg = QStringLiteral("-");
		res.sac_min = QStringLiteral("-");
		res.sac_max = QStringLiteral("-");
	}

	// Diveplan(s)
	res.plans = loc.toString(stats.diveplans);

	return res;
}

void DiveSummaryModel::calc(int column, int period)
{
	if (column >= (int)results.size())
		return;

	QDateTime currentTime = QDateTime::currentDateTime();
	QDateTime startTime = currentTime;

	// Calculate Start of the periods.
	switch (period) {
	case 0: // having startTime == currentTime is used as special case below
		break;
	case 1: startTime = currentTime.addMonths(-1);
		break;
	case 2: startTime = currentTime.addMonths(-3);
		break;
	case 3: startTime = currentTime.addMonths(-6);
		break;
	case 4: startTime = currentTime.addYears(-1);
		break;
	default: qWarning("DiveSummaryModel::calc called with invalid period");
	}
	timestamp_t start;
	if (startTime == currentTime)
		start = 0;
	else
		start = dateTimeToTimestamp(startTime) + gettimezoneoffset();

	// Loop over all dives and sum up data
	Stats stats = loopDives(start);
	results[column] = formatResults(stats);

	// For QML always reload column 0, because that works via roles not columns
	if (column != 0)
		emit dataChanged(index(0, 0), index(NUM_ROW - 1, 0));
	emit dataChanged(index(0, column), index(NUM_ROW - 1, column));
}

void DiveSummaryModel::setNumData(int num)
{
	beginResetModel();
	results.resize(num);
	endResetModel();
}
