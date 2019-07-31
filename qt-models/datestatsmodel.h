#ifndef DATESTATSMODEL_H
#define DATESTATSMODEL_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QHash>
#include <QtCore/QRect>

class DateStatsTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum {
		JAN,
		FEB,
		MAR,
		APR,
		MAY,
		JUN,
		JUL,
		AUG,
		SEP,
		OCT,
		NOV,
		DEC,
		TOTAL,
		YEAR
	};
	explicit DateStatsTableModel(QObject *parent = 0);

	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	int axisMax(Qt::Orientation orientation) const;

private:
	QList<QVector<int>> m_data;
};

#endif // DATESTATSMODEL_H
