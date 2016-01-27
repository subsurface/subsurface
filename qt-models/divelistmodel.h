#ifndef DIVELISTMODEL_H
#define DIVELISTMODEL_H

#include <QAbstractListModel>

#include "dive.h"
#include "helpers.h"
#include "subsurface-qt/DiveObjectHelper.h"

class DiveListModel : public QAbstractListModel
{
	Q_OBJECT
public:

	enum DiveListRoles {
		DiveRole = Qt::UserRole + 1,
		DiveDateRole
	};

	static DiveListModel *instance();
	DiveListModel(QObject *parent = 0);
	void addDive(dive *d);
	void insertDive(int i, DiveObjectHelper *newDive);
	void removeDive(int i);
	void updateDive(dive *d);
	void clear();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QHash<int, QByteArray> roleNames() const;
	QString startAddDive();
	Q_INVOKABLE DiveObjectHelper* at(int i);
private:
	QList<DiveObjectHelper*> m_dives;
	static DiveListModel *m_instance;
};

#endif // DIVELISTMODEL_H
