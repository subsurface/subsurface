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
#include "divetripmodel.h"

class DiveListView : public QTreeView {
	Q_OBJECT
public:
	DiveListView(QWidget *parent = 0);
	~DiveListView();
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void currentChanged(const QModelIndex &current, const QModelIndex &previous);
	void reload(DiveTripModel::Layout layout, bool forceSort = true);
	bool eventFilter(QObject *, QEvent *);
	void unselectDives();
	void clearTripSelection();
	void selectDive(int dive_table_idx, bool scrollto = false, bool toggle = false);
	void selectDives(const QList<int> &newDiveSelection);
	void rememberSelection();
	void restoreSelection();
	void contextMenuEvent(QContextMenuEvent *event);
	QList<dive_trip_t *> selectedTrips();
public
slots:
	void toggleColumnVisibilityByIndex();
	void reloadHeaderActions();
	void headerClicked(int);
	void removeFromTrip();
	void deleteDive();
	void markDiveInvalid();
	void testSlot();
	void fixMessyQtModelBehaviour();
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
	static QString lastUsedImageDir();

signals:
	void currentDiveChanged(int divenr);

private:
	bool mouseClickSelection;
	QList<int> expandedRows;
	int sortColumn;
	Qt::SortOrder currentOrder;
	DiveTripModel::Layout currentLayout;
	QModelIndex contextMenuIndex;
	bool dontEmitDiveChangedSignal;
	bool selectionSaved;

	/* if dive_trip_t is null, there's no problem. */
	QMultiHash<dive_trip_t *, int> selectedDives;
	void merge_trip(const QModelIndex &a, const int offset);
	void setupUi();
	void backupExpandedRows();
	void restoreExpandedRows();
	int lastVisibleColumn();
	void selectTrip(dive_trip_t *trip);
	void updateLastUsedImageDir(const QString &s);
	void updateLastImageTimeOffset(int offset);
	int lastImageTimeOffset();
	void addToTrip(int delta);
	void matchImagesToDives(QStringList fileNames);
	void loadImageFromURL(QUrl url);
	QNetworkAccessManager manager;
};

#endif // DIVELISTVIEW_H
