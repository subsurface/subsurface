#ifndef TAGSTATSMODEL_H
#define TAGSTATSMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QHash>
#include <QtCore/QRect>

class TagStatsListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit TagStatsListModel(QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	int axisMax() const;

private:
	struct TagCounts {
		bool operator<(const TagCounts &other) const {
    			return other.count < count; // sort by number of dives
		}
		QString tag;
		int count;
	};
	QList<TagCounts> m_data;

	int findIndex(QString tag) const;
};

#endif // TAGSTATSMODEL_H
