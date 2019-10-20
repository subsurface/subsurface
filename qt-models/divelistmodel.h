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
	static DiveListSortModel *instance();
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
	DiveListSortModel();
	QString filterString;
	void updateFilterState();
};

QString formatSac(const dive *d);
QString formatNotes(const dive *d);
QString format_gps_decimal(const dive *d);
QStringList formatGetCylinder(const dive *d);
QStringList getStartPressure(const dive *d);
QStringList getEndPressure(const dive *d);
QStringList getFirstGas(const dive *d);
QStringList getFullCylinderList();

class DiveListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	enum DiveListRoles {
		DiveDateRole = Qt::UserRole + 1,
		TripIdRole,
		TripNrDivesRole,
		DateTimeRole,
		IdRole,
		NumberRole,
		LocationRole,
		DepthRole,
		DurationRole,
		DepthDurationRole,
		RatingRole,
		VizRole,
		SuitRole,
		AirTempRole,
		WaterTempRole,
		SacRole,
		SumWeightRole,
		DiveMasterRole,
		BuddyRole,
		NotesRole,
		GpsDecimalRole,
		GpsRole,
		NoDiveRole,
		DiveSiteRole,
		CylinderRole,
		GetCylinderRole,
		CylinderListRole,
		SingleWeightRole,
		StartPressureRole,
		EndPressureRole,
		FirstGasRole,
	};

	static DiveListModel *instance();
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
private:
	DiveListModel();
};

#endif // DIVELISTMODEL_H
