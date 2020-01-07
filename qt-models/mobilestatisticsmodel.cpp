// SPDX-License-Identifier: GPL-2.0
#include "qt-models/mobilestatisticsmodel.h"
#include "core/qthelper.h"

#include <QDebug>
QHash<int, QByteArray> MobileStatisticsModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[YEAR] = "year";
	roles[DIVES] = "dives";
	roles[TOTAL_TIME] = "totalTime";
	roles[AVERAGE_TIME] = "averageTime";
	roles[SHORTEST_TIME] = "shortestTime";
	roles[LONGEST_TIME] = "longestTime";
	roles[AVG_DEPTH] = "averageDepth";
	roles[AVG_MAX_DEPTH] = "avgMaxDepth";
	roles[MIN_DEPTH] = "minDepth";
	roles[MAX_DEPTH] = "maxDepth";
	roles[AVG_SAC] = "averageSac";
	roles[MIN_SAC] = "minSac";
	roles[MAX_SAC] = "maxSac";
	roles[AVG_TEMP] = "averageTemp";
	roles[MIN_TEMP] = "minTemp";
	roles[MAX_TEMP] = "macTemp";
	return roles;
}

int MobileStatisticsModel::rowCount(const QModelIndex &parent) const
{
	return numRows;
}

QVariant MobileStatisticsModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	if (row < 0 || row >= numRows)
		return QVariant();

	stats_t stats_interval = stats.stats_yearly[numRows - row -1];
	switch(role) {
	case YEAR:
		return stats_interval.period;
	case DIVES:
		return stats_interval.selection_size;
	case TOTAL_TIME:
		return get_dive_duration_string(stats_interval.total_time.seconds, tr("h"), tr("min"), tr("sec"), " ");
	case AVERAGE_TIME:
		if (stats_interval.selection_size)
			return get_minutes(stats_interval.total_time.seconds / stats_interval.selection_size);
		break;
	case SHORTEST_TIME:
		return get_minutes(stats_interval.shortest_time.seconds);
	case LONGEST_TIME:
		return get_minutes(stats_interval.longest_time.seconds);
	case AVG_DEPTH:
		return get_depth_string(stats_interval.avg_depth);
	case AVG_MAX_DEPTH:
		if (stats_interval.selection_size)
			return get_depth_string(stats_interval.combined_max_depth.mm / stats_interval.selection_size);
		break;
	case MIN_DEPTH:
		return get_depth_string(stats_interval.min_depth);
	case MAX_DEPTH:
		return get_depth_string(stats_interval.max_depth);
	case AVG_SAC:
		return get_volume_string(stats_interval.avg_sac);
	case MIN_SAC:
		return get_volume_string(stats_interval.min_sac);
	case MAX_SAC:
		return get_volume_string(stats_interval.max_sac);
	case AVG_TEMP:
		if (stats_interval.combined_temp.mkelvin && stats_interval.combined_count) {
			temperature_t avg_temp;
			avg_temp.mkelvin = stats_interval.combined_temp.mkelvin / stats_interval.combined_count;
			return get_temperature_string(avg_temp);
		}
		break;
	case MIN_TEMP:
		if (stats_interval.min_temp.mkelvin)
			return get_temperature_string(stats_interval.min_temp);
		break;
	case MAX_TEMP:
		if (stats_interval.max_temp.mkelvin)
			return get_temperature_string(stats_interval.max_temp);
		break;
	default:
		qDebug() << "missing implementation for data for role" << role;
	}
	return QVariant();
}

MobileStatisticsModel::MobileStatisticsModel(QObject *parent): QAbstractListModel(parent)
{
	updateData();
}

void MobileStatisticsModel::updateData()
{
	// all we want to do is trigger all the views to re-query the data
	beginResetModel();
	calculate_stats_summary(&stats, false);
	for (numRows = 0; stats.stats_yearly != NULL && stats.stats_yearly[numRows].period; ++numRows)
		// do nothing
		;
	qDebug() << "stats model reset, found" << numRows << "years";
	endResetModel();
}
