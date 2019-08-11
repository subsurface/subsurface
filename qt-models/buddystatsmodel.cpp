#include "buddystatsmodel.h"
#include "core/dive.h"
#include "core/divelist.h"
#include <QtCore/QVector>

BuddyStatsListModel::BuddyStatsListModel(QObject *parent) :
	QAbstractListModel(parent)
{
	int idx, buddyIdx;
	struct dive *dp;

	// Iterate through dives and count for each buddy
	for_each_dive(idx, dp) {
		if (!dp->hidden_by_filter) {
			QStringList dive_people = QString(dp->buddy).split(",", QString::SkipEmptyParts, Qt::CaseInsensitive);

			for ( auto& p : dive_people  )
			{
				p = p.trimmed();
				if ((buddyIdx = findIndex(p)) == -1) {
					beginInsertRows(QModelIndex(), rowCount(), rowCount());
					m_data.append({p, 1});
					endInsertRows();
				} else {
					m_data[buddyIdx].count++;
				}
			}
		}
	}
	qSort(m_data);
}

int BuddyStatsListModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_data.count();
}

int BuddyStatsListModel::axisMax() const
{
	int idx, max_dives = 0;

	// Find max number of year dives and round up to next multiple of 10
	for (idx = 0; idx < rowCount(); idx++)
		if (m_data.at(idx).count > max_dives)
			max_dives = m_data.at(idx).count;
	return (((max_dives / 10) + 1) * 10);
}

int BuddyStatsListModel::findIndex(QString buddy) const
{
	int idx;

	for (idx = 0; idx < rowCount(); idx++)
		if (m_data.at(idx).buddy == buddy)
			return idx;
	return (-1);
}

QVariant BuddyStatsListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	return m_data.at(section).buddy;
}

QVariant BuddyStatsListModel::data(const QModelIndex &index, int role) const
{
	return m_data.at(index.row()).count;
}
