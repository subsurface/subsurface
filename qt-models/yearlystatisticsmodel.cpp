// SPDX-License-Identifier: GPL-2.0
#include "qt-models/yearlystatisticsmodel.h"
#include "core/qthelper.h"
#include "core/metrics.h"
#include "core/statistics.h"

class YearStatisticsItem : public TreeItem {
	Q_DECLARE_TR_FUNCTIONS(YearStatisticsItem)
public:
	enum {
		YEAR,
		DIVES,
		TOTAL_TIME,
		AVERAGE_TIME,
		SHORTEST_TIME,
		LONGEST_TIME,
		AVG_DEPTH,
		AVG_MAX_DEPTH,
		MIN_DEPTH,
		MAX_DEPTH,
		AVG_SAC,
		MIN_SAC,
		MAX_SAC,
		AVG_TEMP,
		MIN_TEMP,
		MAX_TEMP,
		COLUMNS
	};

	QVariant data(int column, int role) const;
	YearStatisticsItem(const stats_t &interval);

private:
	stats_t stats_interval;
};

YearStatisticsItem::YearStatisticsItem(const stats_t &interval) : stats_interval(interval)
{
}

QVariant YearStatisticsItem::data(int column, int role) const
{
	QVariant ret;

	if (role == Qt::FontRole) {
		QFont font = defaultModelFont();
		font.setBold(stats_interval.is_year);
		return font;
	} else if (role != Qt::DisplayRole) {
		return ret;
	}
	switch (column) {
	case YEAR:
		if (stats_interval.is_trip) {
			ret = stats_interval.location;
		} else {
			ret = stats_interval.period;
		}
		break;
	case DIVES:
		ret = stats_interval.selection_size;
		break;
	case TOTAL_TIME:
		ret = get_dive_duration_string(stats_interval.total_time.seconds, tr("h"), tr("min"), tr("sec"), " ");
		break;
	case AVERAGE_TIME:
		ret = get_minutes(stats_interval.total_time.seconds / stats_interval.selection_size);
		break;
	case SHORTEST_TIME:
		ret = get_minutes(stats_interval.shortest_time.seconds);
		break;
	case LONGEST_TIME:
		ret = get_minutes(stats_interval.longest_time.seconds);
		break;
	case AVG_DEPTH:
		ret = get_depth_string(stats_interval.avg_depth);
		break;
	case AVG_MAX_DEPTH:
		if (stats_interval.selection_size)
			ret = get_depth_string(stats_interval.combined_max_depth.mm / stats_interval.selection_size);
		break;
	case MIN_DEPTH:
		ret = get_depth_string(stats_interval.min_depth);
		break;
	case MAX_DEPTH:
		ret = get_depth_string(stats_interval.max_depth);
		break;
	case AVG_SAC:
		ret = get_volume_string(stats_interval.avg_sac);
		break;
	case MIN_SAC:
		ret = get_volume_string(stats_interval.min_sac);
		break;
	case MAX_SAC:
		ret = get_volume_string(stats_interval.max_sac);
		break;
	case AVG_TEMP:
		if (stats_interval.combined_temp.mkelvin && stats_interval.combined_count) {
			temperature_t avg_temp;
			avg_temp.mkelvin = stats_interval.combined_temp.mkelvin / stats_interval.combined_count;
			ret = get_temperature_string(avg_temp);
		}
		break;
	case MIN_TEMP:
		if (stats_interval.min_temp.mkelvin)
			ret = get_temperature_string(stats_interval.min_temp);
		break;
	case MAX_TEMP:
		if (stats_interval.max_temp.mkelvin)
			ret = get_temperature_string(stats_interval.max_temp);
		break;
	}
	return ret;
}

YearlyStatisticsModel::YearlyStatisticsModel(QObject *parent) : TreeModel(parent)
{
	columns = COLUMNS;
	update_yearly_stats();
}

QVariant YearlyStatisticsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant val;
	if (role == Qt::FontRole)
		val = defaultModelFont();

	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
		case YEAR:
			val = tr("Year \n > Month / Trip");
			break;
		case DIVES:
			val = tr("#");
			break;
		case TOTAL_TIME:
			val = tr("Duration \n Total");
			break;
		case AVERAGE_TIME:
			val = tr("\nAverage");
			break;
		case SHORTEST_TIME:
			val = tr("\nShortest");
			break;
		case LONGEST_TIME:
			val = tr("\nLongest");
			break;
		case AVG_DEPTH:
			val = QString(tr("Depth (%1)\n Average")).arg(get_depth_unit());
			break;
		case AVG_MAX_DEPTH:
			val = tr("\nAverage maximum");
			break;
		case MIN_DEPTH:
			val = tr("\nMinimum");
			break;
		case MAX_DEPTH:
			val = tr("\nMaximum");
			break;
		case AVG_SAC:
			val = QString(tr("SAC (%1)\n Average")).arg(get_volume_unit());
			break;
		case MIN_SAC:
			val = tr("\nMinimum");
			break;
		case MAX_SAC:
			val = tr("\nMaximum");
			break;
		case AVG_TEMP:
			val = QString(tr("Temp. (%1)\n Average").arg(get_temp_unit()));
			break;
		case MIN_TEMP:
			val = tr("\nMinimum");
			break;
		case MAX_TEMP:
			val = tr("\nMaximum");
			break;
		}
	}
	return val;
}

void YearlyStatisticsModel::update_yearly_stats()
{
	int i, month = 0;
	unsigned int j, combined_months;
	stats_summary_auto_free stats;
	QString label;
	temperature_t t_range_min,t_range_max;
	calculate_stats_summary(&stats, false);

	for (i = 0; stats.stats_yearly != NULL && stats.stats_yearly[i].period; ++i) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_yearly[i]);
		combined_months = 0;
		for (j = 0; combined_months < stats.stats_yearly[i].selection_size; ++j) {
			combined_months += stats.stats_monthly[month].selection_size;
			YearStatisticsItem *iChild = new YearStatisticsItem(stats.stats_monthly[month]);
			item->children.append(iChild);
			iChild->parent = item;
			month++;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	if (stats.stats_by_trip != NULL && stats.stats_by_trip[0].is_trip == true) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_trip[0]);
		for (i = 1; stats.stats_by_trip != NULL && stats.stats_by_trip[i].is_trip; ++i) {
			YearStatisticsItem *iChild = new YearStatisticsItem(stats.stats_by_trip[i]);
			item->children.append(iChild);
			iChild->parent = item;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	/* Show the statistic sorted by dive type */
	if (stats.stats_by_type != NULL && stats.stats_by_type[0].selection_size) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_type[0]);
		for (i = 1; i <= NUM_DIVEMODE; ++i) {
			if (stats.stats_by_type[i].selection_size == 0)
				continue;
			YearStatisticsItem *iChild = new YearStatisticsItem(stats.stats_by_type[i]);
			item->children.append(iChild);
			iChild->parent = item;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	/* Show the statistic sorted by dive depth */
	if (stats.stats_by_depth != NULL && stats.stats_by_depth[0].selection_size) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_depth[0]);
		for (i = 1; stats.stats_by_depth[i].is_trip; ++i)
			if (stats.stats_by_depth[i].selection_size) {
				label = QString(tr("%1 - %2")).arg(get_depth_string((i - 1) * (STATS_DEPTH_BUCKET * 1000), true, false),
					get_depth_string(i * (STATS_DEPTH_BUCKET * 1000), true, false));
				stats.stats_by_depth[i].location = strdup(label.toUtf8().data());
				YearStatisticsItem *iChild = new YearStatisticsItem(stats.stats_by_depth[i]);
				item->children.append(iChild);
				iChild->parent = item;
			}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	/* Show the statistic sorted by dive temperature */
	if (stats.stats_by_temp != NULL && stats.stats_by_temp[0].selection_size) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_temp[0]);
		for (i = 1; stats.stats_by_temp[i].is_trip; ++i)
			if (stats.stats_by_temp[i].selection_size) {
				t_range_min.mkelvin = C_to_mkelvin((i - 1) * STATS_TEMP_BUCKET);
				t_range_max.mkelvin = C_to_mkelvin(i * STATS_TEMP_BUCKET);
				label = QString(tr("%1 - %2")).arg(get_temperature_string(t_range_min, true),
					get_temperature_string(t_range_max, true));
				stats.stats_by_temp[i].location = strdup(label.toUtf8().data());
				YearStatisticsItem *iChild = new YearStatisticsItem(stats.stats_by_temp[i]);
				item->children.append(iChild);
				iChild->parent = item;
			}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}
}
