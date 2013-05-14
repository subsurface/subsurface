/*
 * divelistview.cpp
 *
 * classes for the divelist of Subsurface
 *
 */
#include "divelistview.h"
#include "models.h"
#include "modeldelegates.h"
#include <QApplication>
#include <QHeaderView>
#include <QDebug>
#include <QKeyEvent>


DiveListView::DiveListView(QWidget *parent) : QTreeView(parent), mouseClickSelection(false)
{
	setUniformRowHeights(true);
	setItemDelegateForColumn(TreeItemDT::RATING, new StarWidgetsDelegate());

}

void DiveListView::setModel(QAbstractItemModel* model)
{
	QTreeView::setModel(model);
}

void DiveListView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command)
{
	if (mouseClickSelection)
		QTreeView::setSelection(rect, command);
}

void DiveListView::mousePressEvent(QMouseEvent* event)
{
	mouseClickSelection = true;
	QTreeView::mousePressEvent(event);
}

void DiveListView::mouseReleaseEvent(QMouseEvent* event)
{
	mouseClickSelection = false;
	QTreeView::mouseReleaseEvent(event);
}

void DiveListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	Q_FOREACH(const QModelIndex& index, deselected.indexes()) {
		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, TreeItemDT::DIVE_ROLE).value<void*>();
		if (!dive) { // is's a trip!
			if (model->rowCount(index)) {
				expand(index);	// leave this - even if it looks like it shouldn't be here. looks like I'v found a Qt bug.
						// the subselection is removed, but the painting is not. this cleans the area.
			}
		}
	}

	QList<QModelIndex> parents;
	Q_FOREACH(const QModelIndex& index, selected.indexes()) {
		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, TreeItemDT::DIVE_ROLE).value<void*>();
		if (!dive) { // is's a trip!
			if (model->rowCount(index)) {
				QItemSelection selection;
				selection.select(index.child(0,0), index.child(model->rowCount(index) -1 , 0));
				selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::NoUpdate);
				if (!isExpanded(index)){
					expand(index);
				}
			}
		}
		else if (!parents.contains(index.parent())){
			parents.push_back(index.parent());
		}
	}

	Q_FOREACH(const QModelIndex& index, parents){
		expand(index);
	}
}
