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
	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
	void reload(DiveTripModel::Layout layout, bool forceSort = true);
	bool eventFilter(QObject* , QEvent* );
	void unselectDives();
	void selectDive(struct dive *, bool scrollto = false, bool toggle = false);
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

Q_SIGNALS:
	void currentDiveChanged(int divenr);

private:
	bool mouseClickSelection;
	int currentHeaderClicked;
	DiveTripModel::Layout currentLayout;
	QLineEdit *searchBox;
	QModelIndex contextMenuIndex;
};

#endif // DIVELISTVIEW_H
