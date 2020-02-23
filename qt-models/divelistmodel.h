// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELISTMODEL_H
#define DIVELISTMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>

#include "core/divefilter.h"
#include "core/subsurface-qt/diveobjecthelper.h"

class CollapsedDiveListSortModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	static CollapsedDiveListSortModel *instance();
	void setSourceModel(QAbstractItemModel *sourceModel);
	Q_INVOKABLE QString tripTitle(const QString &trip);
	Q_INVOKABLE QString tripShortDate(const QString &trip);
	Q_INVOKABLE void setActiveTrip(const QString &trip);
	Q_INVOKABLE QString activeTrip() const;
	// super subtle optimization alert - in order to reduce the number of model accesses from QML,
	// the two states where we show the dive in question have odd numbers
	enum CollapsedState {
		DontShow = 0,
		ShowDive = 1,
		ShowTrip = 2,
		ShowDiveAndTrip = 3
	};
	void updateFilterState();
	void updateSelectionState();

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
	CollapsedDiveListSortModel();
	bool isExpanded(struct dive_trip *dt) const;
	QString m_activeTrip;
};

class DiveListSortModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	static DiveListSortModel *instance();
	void setSourceModel(QAbstractItemModel *sourceModel);
	Q_INVOKABLE void reload();
	QString filterString;
	void updateFilterState();
	Q_PROPERTY(int shown READ shown NOTIFY shownChanged);
	int shown();
public slots:
	int getIdxForId(int id);
	void setFilter(QString f, FilterData::Mode mode);
	void resetFilter();
signals:
	void shownChanged();
protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
private:
	DiveListSortModel();
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
		CollapsedRole,
		SelectedRole,
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
