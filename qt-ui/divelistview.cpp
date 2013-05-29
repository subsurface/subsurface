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
#include <QSettings>
#include <QKeyEvent>
#include <QSortFilterProxyModel>
#include <QAction>


DiveListView::DiveListView(QWidget *parent) : QTreeView(parent), mouseClickSelection(false), currentHeaderClicked(-1)
{
	setUniformRowHeights(true);
	setItemDelegateForColumn(TreeItemDT::RATING, new StarWidgetsDelegate());
	QSortFilterProxyModel *model = new QSortFilterProxyModel(this);
	model->setSortRole(TreeItemDT::SORT_ROLE);
	setModel(model);
	setSortingEnabled(false);
	header()->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void DiveListView::headerClicked(int i)
{
	if (currentHeaderClicked == i) {
		sortByColumn(i);
		return;
	}

	if (currentLayout == (i == (int) TreeItemDT::NR ? DiveTripModel::TREE : DiveTripModel::LIST)) {
		sortByColumn(i);
		return;
	}

	QItemSelection oldSelection = selectionModel()->selection();
	QList<struct dive*> currentSelectedDives;
	Q_FOREACH(const QModelIndex& index , oldSelection.indexes()) {
		if (index.column() != 0) // We only care about the dives, so, let's stick to rows and discard columns.
			continue;

		struct dive *d = (struct dive *) index.data(TreeItemDT::DIVE_ROLE).value<void*>();
		if (d)
			currentSelectedDives.push_back(d);
	}

	// clear the model, repopulate with new indexes.
	reload( i == (int) TreeItemDT::NR ? DiveTripModel::TREE : DiveTripModel::LIST, false);
	selectionModel()->clearSelection();
	sortByColumn(i, Qt::DescendingOrder);

	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());

	// repopulat the selections.
	Q_FOREACH(struct dive *d, currentSelectedDives) {
		QModelIndexList match = m->match(m->index(0,0), TreeItemDT::NR, d->number, 1, Qt::MatchRecursive);
		QModelIndex idx = match.first();
		if (i == (int) TreeItemDT::NR && idx.parent().isValid()) { // Tree Mode Activated.
			QModelIndex parent = idx.parent();
			expand(parent);
		}
		selectionModel()->select( idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
	}
}

void DiveListView::reload(DiveTripModel::Layout layout, bool forceSort)
{
	currentLayout = layout;

	header()->setClickable(true);
	connect(header(), SIGNAL(sectionPressed(int)), this, SLOT(headerClicked(int)), Qt::UniqueConnection);

	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	QAbstractItemModel *oldModel = m->sourceModel();
	if (oldModel)
		oldModel->deleteLater();

	DiveTripModel *tripModel = new DiveTripModel(this);
	tripModel->setLayout(layout);

	m->setSourceModel(tripModel);

	if(!forceSort)
		return;

	sortByColumn(0, Qt::DescendingOrder);
	QModelIndex firstDiveOrTrip = m->index(0,0);
	if (firstDiveOrTrip.isValid()) {
		if (m->index(0,0, firstDiveOrTrip).isValid())
			setCurrentIndex(m->index(0,0, firstDiveOrTrip));
		else
			setCurrentIndex(firstDiveOrTrip);
	}
}

void DiveListView::reloadHeaderActions()
{
	// Populate the context menu of the headers that will show
	// the menu to show / hide columns.
	if (!header()->actions().size()) {
		QAction *visibleAction = new QAction("Visible:", header());
		header()->addAction(visibleAction);
		QSettings s;
		s.beginGroup("DiveListColumnState");
		for(int i = 0; i < model()->columnCount(); i++) {
			QString title = QString("%1").arg(model()->headerData(i, Qt::Horizontal).toString());
			QString settingName = QString("showColumn%1").arg(i);
			QAction *a = new QAction(title, header());
			bool shown = s.value(settingName, true).toBool();
			a->setCheckable(true);
			a->setChecked(shown);
			a->setProperty("index", i);
			a->setProperty("settingName", settingName);
			connect(a, SIGNAL(triggered(bool)), this, SLOT(toggleColumnVisibilityByIndex()));
			header()->addAction(a);
			setColumnHidden(i, !shown);
		}
		s.endGroup();
	}
}

void DiveListView::toggleColumnVisibilityByIndex()
{
	QAction *action = qobject_cast<QAction*>(sender());
	if (!action)
		return;

	QSettings s;
	s.beginGroup("DiveListColumnState");
	s.setValue(action->property("settingName").toString(), action->isChecked());
	s.endGroup();
	s.sync();
	setColumnHidden(action->property("index").toInt(), !action->isChecked());
}

void DiveListView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
	if (!current.isValid())
		return;
	const QAbstractItemModel *model = current.model();
	int selectedDive = 0;
	struct dive *dive = (struct dive*) model->data(current, TreeItemDT::DIVE_ROLE).value<void*>();
	if (!dive)  // it's a trip! select first child.
		dive = (struct dive*) model->data(current.child(0,0), TreeItemDT::DIVE_ROLE).value<void*>();
	selectedDive = get_divenr(dive);
	if (selectedDive == selected_dive)
		return;
	Q_EMIT currentDiveChanged(selectedDive);
}

void DiveListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QItemSelection newSelected = selected.size() ? selected : selectionModel()->selection();
	QItemSelection newDeselected = deselected;

	disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),         this, SLOT(currentChanged(QModelIndex,QModelIndex)));

	Q_FOREACH(const QModelIndex& index, newSelected.indexes()) {
		if(index.column() != 0)
			continue;

		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, TreeItemDT::DIVE_ROLE).value<void*>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				QItemSelection selection;
				selection.select(index.child(0,0), index.child(model->rowCount(index) -1 , 0));
				selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::NoUpdate);
				if (!isExpanded(index))
					expand(index);
			}
		}
	}

	QTreeView::selectionChanged(selectionModel()->selection(), newDeselected);
	connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),         this, SLOT(currentChanged(QModelIndex,QModelIndex)));
}
