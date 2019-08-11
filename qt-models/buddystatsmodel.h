#ifndef BUDDYSTATSMODEL_H
#define BUDDYSTATSMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QHash>
#include <QtCore/QRect>

class BuddyStatsListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit BuddyStatsListModel(QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	int axisMax() const;

private:
	struct BuddyCounts {
		bool operator<(const BuddyCounts &other) const {
    			return other.count < count; // sort by number of dives
		}
		QString buddy;
		int count;
	};
	QList<BuddyCounts> m_data;

	int findIndex(QString buddy) const;
};

#endif // BUDDYSTATSMODEL_H
