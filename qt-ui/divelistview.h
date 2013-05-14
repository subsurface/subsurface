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

class DiveListView : public QTreeView
{
	Q_OBJECT
public:
	DiveListView(QWidget *parent = 0);
	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
	void setModel(QAbstractItemModel* model);
	void mousePressEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent*);
	void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command);
Q_SIGNALS:
	void currentDiveChanged(int divenr);
private:
	bool mouseClickSelection;
};

#endif // DIVELISTVIEW_H
