// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELISTMODEL_H
#define DIVELISTMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "core/dive.h"
#include "core/helpers.h"
#include "core/subsurface-qt/DiveObjectHelper.h"

class DiveListSortModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	DiveListSortModel(QObject *parent = 0);
	Q_INVOKABLE void addAllDives();
	Q_INVOKABLE void clear();
public slots:
	int getDiveId(int idx);
	int getIdxForId(int id);
};

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
	void addDive(QList<dive *> listOfDives);
	void addAllDives();
	void insertDive(int i, DiveObjectHelper *newDive);
	void removeDive(int i);
	void removeDiveById(int id);
	void updateDive(int i, dive *d);
	void clear();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int getDiveId(int idx) const;
	int getDiveIdx(int id) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QHash<int, QByteArray> roleNames() const;
	QString startAddDive();
	Q_INVOKABLE DiveObjectHelper* at(int i);
private:
	QList<DiveObjectHelper*> m_dives;
	static DiveListModel *m_instance;
};

#endif // DIVELISTMODEL_H
