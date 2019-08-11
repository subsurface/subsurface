#include "tagstatsmodel.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/tag.h"
#include <QtCore/QVector>

TagStatsListModel::TagStatsListModel(QObject *parent) :
	QAbstractListModel(parent)
{
	int idx, tagIdx;
	struct dive *dp;
	struct tag_entry *tl;

	// Iterate through dives and count for each tag
	for_each_dive(idx, dp) {
		if (!dp->hidden_by_filter) {
			tl = dp->tag_list;
			while(tl) {
				if ((tagIdx = findIndex(QString("%1").arg(tl->tag->name))) == -1) {
					beginInsertRows(QModelIndex(), rowCount(), rowCount());
					m_data.append({QString("%1").arg(tl->tag->name), 1});
					endInsertRows();
				} else {
					m_data[tagIdx].count++;
				}
				tl = tl->next;
			}
		}
	}
	qSort(m_data);
}

int TagStatsListModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_data.count();
}

int TagStatsListModel::axisMax() const
{
	int idx, max_dives = 0;

	// Find max number of year dives and round up to next multiple of 10
	for (idx = 0; idx < rowCount(); idx++)
		if (m_data.at(idx).count > max_dives)
			max_dives = m_data.at(idx).count;
	return (((max_dives / 10) + 1) * 10);
}

int TagStatsListModel::findIndex(QString tag) const
{
	int idx;

	for (idx = 0; idx < rowCount(); idx++)
		if (m_data.at(idx).tag == tag)
			return idx;
	return (-1);
}

QVariant TagStatsListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	return m_data.at(section).tag;
}

QVariant TagStatsListModel::data(const QModelIndex &index, int role) const
{
	return m_data.at(index.row()).count;
}
