#include "yearlystatisticsmodel.h"
#include "dive.h"
#include "helpers.h"
#include "metrics.h"
#include "statistics.h"

class YearStatisticsItem : public TreeItem {
public:
	enum {
		YEAR,
		DIVES,
		TOTAL_TIME,
		AVERAGE_TIME,
		SHORTEST_TIME,
		LONGEST_TIME,
		AVG_DEPTH,
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
	YearStatisticsItem(stats_t interval);

private:
	stats_t stats_interval;
};

YearStatisticsItem::YearStatisticsItem(stats_t interval) : stats_interval(interval)
{
}

QVariant YearStatisticsItem::data(int column, int role) const
{
	double value;
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
		ret = get_time_string(stats_interval.total_time.seconds, 0);
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
		if (stats_interval.combined_temp && stats_interval.combined_count) {
			ret = QString::number(stats_interval.combined_temp / stats_interval.combined_count, 'f', 1);
		}
		break;
	case MIN_TEMP:
		value = get_temp_units(stats_interval.min_temp, NULL);
		if (value > -100.0)
			ret = QString::number(value, 'f', 1);
		break;
	case MAX_TEMP:
		value = get_temp_units(stats_interval.max_temp, NULL);
		if (value > -100.0)
			ret = QString::number(value, 'f', 1);
		break;
	}
	return ret;
}

YearlyStatisticsModel::YearlyStatisticsModel(QObject *parent)
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

	for (i = 0; stats_yearly != NULL && stats_yearly[i].period; ++i) {
		YearStatisticsItem *item = new YearStatisticsItem(stats_yearly[i]);
		combined_months = 0;
		for (j = 0; combined_months < stats_yearly[i].selection_size; ++j) {
			combined_months += stats_monthly[month].selection_size;
			YearStatisticsItem *iChild = new YearStatisticsItem(stats_monthly[month]);
			item->children.append(iChild);
			iChild->parent = item;
			month++;
		}
		rootItem->children.append(item);
		item->parent = rootItem;
	}


	if (stats_by_trip != NULL && stats_by_trip[0].is_trip == true) {
		YearStatisticsItem *item = new YearStatisticsItem(stats_by_trip[0]);
		for (i = 1; stats_by_trip != NULL && stats_by_trip[i].is_trip; ++i) {
			YearStatisticsItem *iChild = new YearStatisticsItem(stats_by_trip[i]);
			item->children.append(iChild);
			iChild->parent = item;
		}
		rootItem->children.append(item);
		item->parent = rootItem;
	}

	/* Show the statistic sorted by dive type */
	if (stats_by_type != NULL && stats_by_type[0].selection_size) {
		YearStatisticsItem *item = new YearStatisticsItem(stats_by_type[0]);
		for (i = 1; i <= sizeof(dive_comp_type) + 1; ++i) {
			if (stats_by_type[i].selection_size == 0)
				continue;
			YearStatisticsItem *iChild = new YearStatisticsItem(stats_by_type[i]);
			item->children.append(iChild);
			iChild->parent = item;
		}
		rootItem->children.append(item);
		item->parent = rootItem;
	}
}
