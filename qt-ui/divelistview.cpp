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

void DiveListView::keyPressEvent(QKeyEvent* event)
{
	if(event->modifiers())
		mouseClickSelection = true;
	QTreeView::keyPressEvent(event);
}

void DiveListView::keyReleaseEvent(QKeyEvent* event)
{
	mouseClickSelection = false;
	QWidget::keyReleaseEvent(event);
}

void DiveListView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
	if (!current.isValid())
		return;
	const QAbstractItemModel *model = current.model();
	int selectedDive = 0;
	struct dive *dive = (struct dive*) model->data(current, TreeItemDT::DIVE_ROLE).value<void*>();
	if (!dive) { // it's a trip! select first child.
		dive = (struct dive*) model->data(current.child(0,0), TreeItemDT::DIVE_ROLE).value<void*>();
		selectedDive = get_divenr(dive);
	}else{
		selectedDive = get_divenr(dive);
	}
	if (selectedDive == selected_dive)
		return;
	Q_EMIT currentDiveChanged(selectedDive);
}

void DiveListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QList<QModelIndex> parents;
	Q_FOREACH(const QModelIndex& index, deselected.indexes()) {
		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, TreeItemDT::DIVE_ROLE).value<void*>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				expand(index);	// leave this - even if it looks like it shouldn't be here. looks like I've found a Qt bug.
						// the subselection is removed, but the painting is not. this cleans the area.
			}
		} else if (!parents.contains(index.parent())) {
			parents.push_back(index.parent());
		}
	}

	Q_FOREACH(const QModelIndex& index, selected.indexes()) {
		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, TreeItemDT::DIVE_ROLE).value<void*>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				QItemSelection selection;
				selection.select(index.child(0,0), index.child(model->rowCount(index) -1 , 0));
				selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::NoUpdate);
				if (!isExpanded(index)) {
					expand(index);
				}
			}
		} else if (!parents.contains(index.parent())) {
			parents.push_back(index.parent());
		}
	}

	Q_FOREACH(const QModelIndex& index, parents)
		expand(index);
}
