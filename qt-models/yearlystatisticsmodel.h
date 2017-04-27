// SPDX-License-Identifier: GPL-2.0
#ifndef YEARLYSTATISTICSMODEL_H
#define YEARLYSTATISTICSMODEL_H

#include "treemodel.h"

class YearlyStatisticsModel : public TreeModel {
	Q_OBJECT
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

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	YearlyStatisticsModel(QObject *parent = 0);
	void update_yearly_stats();
};

#endif
