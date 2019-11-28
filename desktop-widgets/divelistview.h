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
#include <QLineEdit>
#include <QNetworkAccessManager>
#include "qt-models/divetripmodel.h"
#include "core/subsurface-qt/DiveListNotifier.h"

class DiveListView : public QTreeView {
	Q_OBJECT
public:
	DiveListView(QWidget *parent = 0);
	~DiveListView();
	void mouseDoubleClickEvent(QMouseEvent * event);
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void currentChanged(const QModelIndex &current, const QModelIndex &previous);
	void setSortOrder(int i, Qt::SortOrder order); // Call to set sort order
	void reload(); // Call to reload model data
	bool eventFilter(QObject *, QEvent *);
	void unselectDives();
	void clearTripSelection();
	void selectDive(QModelIndex index, bool scrollto = false);
	void selectDive(int dive_table_idx, bool scrollto = false);
	void selectDives(const QList<int> &newDiveSelection);
	void contextMenuEvent(QContextMenuEvent *event);
	QList<dive_trip *> selectedTrips();
	static QString lastUsedImageDir();
	static void updateLastUsedImageDir(const QString &s);
signals:
	void divesSelected();
public
slots:
	void toggleColumnVisibilityByIndex();
	void reloadHeaderActions();
	void sortIndicatorChanged(int index, Qt::SortOrder order);
	void removeFromTrip();
	void deleteDive();
	void markDiveInvalid();
	void rowsInserted(const QModelIndex &parent, int start, int end) override;
	void reset() override;
	void mergeTripAbove();
	void mergeTripBelow();
	void newTripAbove();
	void addToTripAbove();
	void addToTripBelow();
	void mergeDives();
	void splitDives();
	void renumberDives();
	void shiftTimes();
	void loadImages();
	void loadWebImages();
	void diveSelectionChanged(const QVector<QModelIndex> &indexes);
	void currentDiveChanged(QModelIndex index);
	void filterFinished();
	void tripChanged(dive_trip *trip, TripField);
private:
	void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags) override;
	void selectAll() override;
	void selectionChangeDone();
	DiveTripModelBase::Layout currentLayout;
	QModelIndex contextMenuIndex;
	bool dontEmitDiveChangedSignal;
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
	void updateLastImageTimeOffset(int offset);
	int lastImageTimeOffset();
	void addToTrip(int delta);
	void matchImagesToDives(QStringList fileNames);
	void loadImageFromURL(QUrl url);
	QNetworkAccessManager manager;
};

#endif // DIVELISTVIEW_H
