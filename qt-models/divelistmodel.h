// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELISTMODEL_H
#define DIVELISTMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "core/subsurface-qt/DiveObjectHelper.h"

class DiveListSortModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	DiveListSortModel(QObject *parent = 0);
	void setSourceModel(QAbstractItemModel *sourceModel);
	Q_INVOKABLE void reload();
	Q_INVOKABLE QString tripTitle(const QString &trip);
	Q_INVOKABLE QString tripShortDate(const QString &trip);
public slots:
	int getIdxForId(int id);
	void setFilter(QString f);
	void resetFilter();
	int shown();
protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
private:
	QString filterString;
	void updateFilterState();
};

class DiveListModel : public QAbstractListModel
{
	Q_OBJECT
public:

	enum DiveListRoles {
		DiveRole = Qt::UserRole + 1,
		DiveDateRole,
		TripIdRole,
		TripNrDivesRole,
		DateTimeRole,
		IdRole,
		NumberRole,
		LocationRole,
		DepthDurationRole,
	};

	static DiveListModel *instance();
	DiveListModel();
	void addDive(const QList<dive *> &listOfDives);
	void addAllDives();
	void insertDive(int i);
	void removeDive(int i);
	void removeDiveById(int id);
	void updateDive(int i, dive *d);
	void reload(); // Only call after clearing the model!
	struct dive *getDive(int i);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int getDiveIdx(int id) const;
	QModelIndex getDiveQIdx(int id);
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QHash<int, QByteArray> roleNames() const;
	QString startAddDive();
	void resetInternalData();
	void clear(); // Clear all dives in core
	Q_INVOKABLE DiveObjectHelper at(int i);
};

#endif // DIVELISTMODEL_H
