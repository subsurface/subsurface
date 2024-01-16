// SPDX-License-Identifier: GPL-2.0
/*
 * divelistview.h
 *
 * header file for the dive list of Subsurface
 *
 */
#ifndef DIVELISTVIEW_H
#define DIVELISTVIEW_H

/*! A view subclass for use with dives
  Note: calling this a list view might be misleading?
*/

#include <QTreeView>
#include <QNetworkAccessManager>
#include "qt-models/divetripmodel.h"
#include "core/subsurface-qt/divelistnotifier.h"

class DiveListView : public QTreeView {
	Q_OBJECT
public:
	DiveListView(QWidget *parent = 0);
	~DiveListView();
	void setSortOrder(int i, Qt::SortOrder order); // Call to set sort order
	void reload(); // Call to reload model data
	static QString lastUsedImageDir();
	static void updateLastUsedImageDir(const QString &s);
	void loadImages();
	void loadWebImages();
signals:
	// currentDC = -1: don't change dc number.
	void divesSelected(const std::vector<dive *> &dives, dive *currentDive, int currentDC);
public
slots:
	void settingsChanged();
private
slots:
	void toggleColumnVisibilityByIndex();
	void sortIndicatorChanged(int index, Qt::SortOrder order);
	void removeFromTrip();
	void deleteDive();
	void markDiveInvalid();
	void markDiveValid();
	void mergeTripAbove();
	void mergeTripBelow();
	void newTripAbove();
	void addToTripAbove();
	void addToTripBelow();
	void mergeDives();
	void splitDives();
	void renumberDives();
	void addDivesToTrip();
	void shiftTimes();
	void divesSelectedSlot(const QVector<QModelIndex> &indices, QModelIndex currentDive, int currentDC);
	void tripSelected(QModelIndex trip, QModelIndex currentDive);
private:
	void rowsInserted(const QModelIndex &parent, int start, int end) override;
	void reset() override;
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
	void selectDiveSitesOnMap(const std::vector<dive *> &dives);
	void selectTripItems(QModelIndex index);
	DiveTripModelBase::Layout currentLayout;
	QModelIndex contextMenuIndex;
	// Remember the initial column widths, to avoid writing unchanged widths to the settings
	QVector<int> initialColumnWidths;

	void resetModel();	// Call after model changed
	void merge_trip(const QModelIndex &a, const int offset);
	void setColumnWidths();
	void calculateInitialColumnWidth(int col);
	std::vector<int> backupExpandedRows();
	void restoreExpandedRows(const std::vector<int> &);
	int lastVisibleColumn();
	void selectTrip(dive_trip *trip);
	void updateLastImageTimeOffset(timestamp_t offset);
	int lastImageTimeOffset();
	void addToTrip(int delta);
	void matchImagesToDives(const QStringList &fileNames);
	void loadImagesFromURLs(const QString &urls);
	bool eventFilter(QObject *, QEvent *) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
	QNetworkAccessManager manager;
	bool programmaticalSelectionChange;
};

#endif // DIVELISTVIEW_H
