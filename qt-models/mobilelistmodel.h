// SPDX-License-Identifier: GPL-2.0
#ifndef MOBILELISTMODEL_H
#define MOBILELISTMODEL_H

#include "divetripmodel.h"

class MobileListModel : public QAbstractItemModel {
	Q_OBJECT
public:
	enum Roles {
		IsTopLevelRole = DiveTripModelBase::LAST_ROLE + 1,
		DiveDateRole,
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
		SelectedRole,
	};
	MobileListModel(DiveTripModelBase *source);
	static MobileListModel *instance();
	void resetModel();
	void expand(int row);
	void unexpand();
	void toggle(int row);
	Q_PROPERTY(int shown READ shown NOTIFY shownChanged);
signals:
	void shownChanged();
private:
	struct IndexRange {
		bool visible;
		int first, last;
	};
	void connectSignals();
	std::vector<IndexRange> rangeStack;
	QModelIndex sourceIndex(int row, int col, int parentRow = -1) const;
	int numSubItems() const;
	int mapRowFromSourceTopLevel(int row) const;
	int mapRowFromSourceTopLevelForInsert(int row) const;
	int mapRowFromSourceTrip(const QModelIndex &parent, int parentRow, int row) const;
	int mapRowFromSource(const QModelIndex &parent, int row) const;
	int invertRow(const QModelIndex &parent, int row) const;
	IndexRange mapRangeFromSource(const QModelIndex &parent, int first, int last) const;
	IndexRange mapRangeFromSourceForInsert(const QModelIndex &parent, int first, int last) const;
	QModelIndex mapFromSource(const QModelIndex &idx) const;
	QModelIndex mapToSource(const QModelIndex &idx) const;
	static void updateRowAfterRemove(const IndexRange &range, int &row);
	static void updateRowAfterMove(const IndexRange &range, const IndexRange &dest, int &row);
	QVariant data(const QModelIndex &index, int role) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	QHash<int, QByteArray> roleNames() const override;
	int shown() const;

	DiveTripModelBase *source;
	int expandedRow;
private slots:
	void prepareRemove(const QModelIndex &parent, int first, int last);
	void doneRemove(const QModelIndex &parent, int first, int last);
	void prepareInsert(const QModelIndex &parent, int first, int last);
	void doneInsert(const QModelIndex &parent, int first, int last);
	void prepareMove(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int destRow);
	void doneMove(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int destRow);
	void changed(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
};


// Helper functions - these are actually defined in DiveObjectHelper.cpp. Why declare them here?
QString formatSac(const dive *d);
QString formatNotes(const dive *d);
QString format_gps_decimal(const dive *d);
QStringList formatGetCylinder(const dive *d);
QStringList getStartPressure(const dive *d);
QStringList getEndPressure(const dive *d);
QStringList getFirstGas(const dive *d);
QStringList getFullCylinderList();

#endif
