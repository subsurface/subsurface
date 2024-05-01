// SPDX-License-Identifier: GPL-2.0
#include "qt-models/yearlystatisticsmodel.h"
#include "core/qthelper.h"
#include "core/metrics.h"
#include "core/statistics.h"
#include "core/string-format.h"
#include "core/dive.h" // For NUM_DIVEMODE

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
	if (role == Qt::FontRole) {
		QFont font = defaultModelFont();
		font.setBold(stats_interval.is_year);
		return font;
	} else if (role != Qt::DisplayRole) {
		return QVariant();
	}
	switch (column) {
	case YEAR:
		if (stats_interval.is_trip) {
			return QString::fromStdString(stats_interval.location);
		} else {
			return stats_interval.period;
		}
	case DIVES:
		return stats_interval.selection_size;
	case TOTAL_TIME:
		return get_dive_duration_string(stats_interval.total_time.seconds, tr("h"), tr("min"), tr("sec"), " ");
	case AVERAGE_TIME:
		return formatMinutes(stats_interval.total_time.seconds / stats_interval.selection_size);
	case SHORTEST_TIME:
		return formatMinutes(stats_interval.shortest_time.seconds);
	case LONGEST_TIME:
		return formatMinutes(stats_interval.longest_time.seconds);
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
	}
	return QVariant();
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
	QString label;
	temperature_t t_range_min,t_range_max;
	stats_summary stats = calculate_stats_summary(false);

	int month = 0;
	for (const auto &s: stats.stats_yearly) {
		YearStatisticsItem *item = new YearStatisticsItem(s);
		size_t combined_months = 0;
		while (combined_months < s.selection_size) {
			combined_months += stats.stats_monthly[month].selection_size;
			YearStatisticsItem *iChild = new YearStatisticsItem(stats.stats_monthly[month]);
			item->children.append(iChild);
			iChild->parent = item;
			month++;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	if (stats.stats_by_trip[0].is_trip == true) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_trip[0]);
		for (auto it = std::next(stats.stats_by_trip.begin()); it != stats.stats_by_trip.end(); ++it) {
			YearStatisticsItem *iChild = new YearStatisticsItem(*it);
			item->children.append(iChild);
			iChild->parent = item;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	/* Show the statistic sorted by dive type */
	if (stats.stats_by_type[0].selection_size) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_type[0]);
		for (auto it = std::next(stats.stats_by_type.begin()); it != stats.stats_by_type.end(); ++it) {
			if (it->selection_size == 0)
				continue;
			YearStatisticsItem *iChild = new YearStatisticsItem(*it);
			item->children.append(iChild);
			iChild->parent = item;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	/* Show the statistic sorted by dive depth */
	if (stats.stats_by_depth[0].selection_size) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_depth[0]);
		int i = 0;
		for (auto it = std::next(stats.stats_by_depth.begin()); it != stats.stats_by_depth.end(); ++it) {
			if (it->selection_size) {
				QString label = QString(tr("%1 - %2")).arg(get_depth_string(i * (STATS_DEPTH_BUCKET * 1000), true, false),
						get_depth_string((i + 1) * (STATS_DEPTH_BUCKET * 1000), true, false));
				it->location = label.toStdString();
				YearStatisticsItem *iChild = new YearStatisticsItem(*it);
				item->children.append(iChild);
				iChild->parent = item;
			}
			i++;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}

	/* Show the statistic sorted by dive temperature */
	if (stats.stats_by_temp[0].selection_size) {
		YearStatisticsItem *item = new YearStatisticsItem(stats.stats_by_temp[0]);
		int i = 0;
		for (auto it = std::next(stats.stats_by_temp.begin()); it != stats.stats_by_temp.end(); ++it) {
			if (it->selection_size) {
				t_range_min.mkelvin = C_to_mkelvin(i * STATS_TEMP_BUCKET);
				t_range_max.mkelvin = C_to_mkelvin((i + 1) * STATS_TEMP_BUCKET);
				label = QString(tr("%1 - %2")).arg(get_temperature_string(t_range_min, true),
					get_temperature_string(t_range_max, true));
				it->location = label.toStdString();
				YearStatisticsItem *iChild = new YearStatisticsItem(*it);
				item->children.append(iChild);
				iChild->parent = item;
			}
			i++;
		}
		rootItem->children.append(item);
		item->parent = rootItem.get();
	}
}
