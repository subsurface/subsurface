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
#include "models.h"

class DiveListView : public QTreeView
{
	Q_OBJECT
public:
	DiveListView(QWidget *parent = 0);
	~DiveListView();
	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
	void reload(DiveTripModel::Layout layout, bool forceSort = true);
	bool eventFilter(QObject* , QEvent* );
	void unselectDives();
	void selectDive(int dive_table_idx, bool scrollto = false, bool toggle = false);
	void rememberSelection();
	void restoreSelection();
	void contextMenuEvent(QContextMenuEvent *event);
	QSet<dive_trip_t *> selectedTrips;

public slots:
	void toggleColumnVisibilityByIndex();
	void reloadHeaderActions();
	void headerClicked(int);
	void showSearchEdit();
	void removeFromTrip();
	void deleteDive();
	void testSlot();
	void fixMessyQtModelBehaviour();
	void mergeTripAbove();
	void mergeTripBelow();
	void newTripAbove();
	void addToTripAbove();
	void mergeDives();
	void saveSelectedDivesAs();
	void exportSelectedDivesAsUDDF();

signals:
	void currentDiveChanged(int divenr);

private:
	bool mouseClickSelection;
	QList<int> expandedRows;
	QList<int> selectedDives;
	int sortColumn;
	Qt::SortOrder currentOrder;
	DiveTripModel::Layout currentLayout;
	QLineEdit *searchBox;
	QModelIndex contextMenuIndex;
	void merge_trip(const QModelIndex &a, const int offset);
	void setupUi();
	void backupExpandedRows();
	void restoreExpandedRows();
	int lastVisibleColumn();
};

#endif // DIVELISTVIEW_H
