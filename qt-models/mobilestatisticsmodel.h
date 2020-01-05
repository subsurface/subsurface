// SPDX-License-Identifier: GPL-2.0
#ifndef MOBILESTATISTICSMODEL_H
#define MOBILESTATISTICSMODEL_H

#include <QAbstractListModel>
#include "core/statistics.h"

class MobileStatisticsModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum MobileStatisticRoles {
		YEAR = Qt::UserRole + 1,
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
		MAX_TEMP
	};
	MobileStatisticsModel(QObject *parent = 0);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
	void updateData(void);
private:
	stats_summary_auto_free stats;
	int numRows;
};

#endif /* MOBILESTATISTICSMODEL_H */
