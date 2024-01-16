// SPDX-License-Identifier: GPL-2.0
/*
 * divelistview.cpp
 *
 * class for the divelist of Subsurface
 *
 */
#include "qt-models/filtermodels.h"
#include "desktop-widgets/modeldelegates.h"
#include "desktop-widgets/mainwindow.h"
#include "core/selection.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include <unistd.h>
#include <QSettings>
#include <QKeyEvent>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QNetworkReply>
#include <QHeaderView>
#include "commands/command.h"
#include "commands/command_base.h"
#include "core/errorhelper.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "desktop-widgets/divelistview.h"
#include "core/metrics.h"
#include "desktop-widgets/simplewidgets.h"
#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/tripselectiondialog.h"

DiveListView::DiveListView(QWidget *parent) : QTreeView(parent),
	currentLayout(DiveTripModelBase::TREE),
	initialColumnWidths(DiveTripModelBase::COLUMNS, 50),	// Set up with default length 50
	programmaticalSelectionChange(false)
{
	setItemDelegate(new DiveListDelegate(this));
	setUniformRowHeights(true);
	setItemDelegateForColumn(DiveTripModelBase::RATING, new StarWidgetsDelegate(this));
	MultiFilterSortModel *m = MultiFilterSortModel::instance();
	setModel(m);
	connect(m, &MultiFilterSortModel::divesSelected, this, &DiveListView::divesSelectedSlot);
	connect(m, &MultiFilterSortModel::tripSelected, this, &DiveListView::tripSelected);
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &DiveListView::settingsChanged);

	setSortingEnabled(true);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	setSelectionMode(ExtendedSelection);
	header()->setContextMenuPolicy(Qt::ActionsContextMenu);

	resetModel();

	header()->setStretchLastSection(true);
	header()->setSortIndicatorShown(true);
	header()->setSectionsClickable(true);
	connect(header(), &QHeaderView::sortIndicatorChanged, this, &DiveListView::sortIndicatorChanged);

	installEventFilter(this);

	for (int i = DiveTripModelBase::NR; i < DiveTripModelBase::COLUMNS; i++)
		calculateInitialColumnWidth(i);
	setColumnWidths();

	QSettings s;
	s.beginGroup("DiveListColumnState");
	for (int i = 0; i < model()->columnCount(); i++) {
		QString title = QString("%1").arg(model()->headerData(i, Qt::Horizontal).toString());
		QString settingName = QString("showColumn%1").arg(i);
		QAction *a = new QAction(title, header());
		bool showHeaderFirstRun = i == DiveTripModelBase::NR ||
					  i == DiveTripModelBase::DATE ||
					  i == DiveTripModelBase::RATING ||
					  i == DiveTripModelBase::DEPTH ||
					  i == DiveTripModelBase::DURATION ||
					  i == DiveTripModelBase::PHOTOS ||
					  i == DiveTripModelBase::BUDDIES ||
					  i == DiveTripModelBase::DIVEGUIDE ||
					  i == DiveTripModelBase::COUNTRY ||
					  i == DiveTripModelBase::LOCATION;
		bool shown = s.value(settingName, showHeaderFirstRun).toBool();
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

DiveListView::~DiveListView()
{
	QSettings settings;
	settings.beginGroup("ListWidget");
	// don't set a width for the last column - location is supposed to be "the rest"
	for (int i = DiveTripModelBase::NR; i < DiveTripModelBase::COLUMNS - 1; i++) {
		if (isColumnHidden(i))
			continue;
		// we used to hardcode them all to 100 - so that might still be in the settings
		if (columnWidth(i) == 100 || columnWidth(i) == initialColumnWidths[i])
			settings.remove(QString("colwidth%1").arg(i));
		else
			settings.setValue(QString("colwidth%1").arg(i), columnWidth(i));
	}
	settings.remove(QString("colwidth%1").arg(DiveTripModelBase::COLUMNS - 1));
	settings.endGroup();
}

void DiveListView::resetModel()
{
	MultiFilterSortModel *m = MultiFilterSortModel::instance();
	m->resetModel(currentLayout);
}

void DiveListView::calculateInitialColumnWidth(int col)
{
	const QFontMetrics metrics(defaultModelFont());
	QString header_txt = MultiFilterSortModel::instance()->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
	int em = metrics.width('m');
	int zw = metrics.width('0');
	int width = metrics.width(header_txt);
#else // QT 5.11 or newer
	int em = metrics.horizontalAdvance('m');
	int zw = metrics.horizontalAdvance('0');
	int width = metrics.horizontalAdvance(header_txt);
#endif
	int sw = 0;
	switch (col) {
	case DiveTripModelBase::NR:
	case DiveTripModelBase::DURATION:
		sw = 8*zw;
		break;
	case DiveTripModelBase::DATE:
		sw = 14*em;
		break;
	case DiveTripModelBase::RATING:
		sw = static_cast<StarWidgetsDelegate*>(itemDelegateForColumn(col))->starSize().width();
		break;
	case DiveTripModelBase::SUIT:
	case DiveTripModelBase::SAC:
		sw = 7*em;
		break;
	case DiveTripModelBase::PHOTOS:
		sw = 5*em;
		break;
	case DiveTripModelBase::BUDDIES:
		sw = 50*em;
		break;
	case DiveTripModelBase::LOCATION:
		sw = 50*em;
		break;
	default:
		sw = 5*em;
	}
	if (sw > width)
		width = sw;
	width += zw; // small padding
	initialColumnWidths[col] = std::max(initialColumnWidths[col], width);
}

void DiveListView::setColumnWidths()
{
	QSettings settings;
	std::vector<int> expandedRows = backupExpandedRows();
	settings.beginGroup("ListWidget");
	/* if no width are set, use the calculated width for each column;
	 * for that to work we need to temporarily expand all rows */
	expandAll();
	for (int i = DiveTripModelBase::NR; i < DiveTripModelBase::COLUMNS; i++) {
		if (isColumnHidden(i))
			continue;
		QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			setColumnWidth(i, width.toInt());
		else
			setColumnWidth(i, initialColumnWidths[i]);
	}
	settings.endGroup();
	restoreExpandedRows(expandedRows);
	setColumnWidth(lastVisibleColumn(), 10);
}

int DiveListView::lastVisibleColumn()
{
	int lastColumn = -1;
	for (int i = DiveTripModelBase::NR; i < DiveTripModelBase::COLUMNS; i++) {
		if (isColumnHidden(i))
			continue;
		lastColumn = i;
	}
	return lastColumn;
}

std::vector<int> DiveListView::backupExpandedRows()
{
	std::vector<int> expandedRows;
	for (int i = 0; i < model()->rowCount(); i++)
		if (isExpanded(model()->index(i, 0)))
			expandedRows.push_back(i);
	return expandedRows;
}

void DiveListView::restoreExpandedRows(const std::vector<int> &expandedRows)
{
	setAnimated(false);
	for (int i: expandedRows)
		setExpanded(model()->index(i, 0), true);
	setAnimated(true);
}

// If the model is reset, check which items are trip-items and expand the first column
void DiveListView::reset()
{
	// First, let the QTreeView do its thing.
	QTreeView::reset();

	QAbstractItemModel *m = model();
	for (int i = 0; i < m->rowCount(); ++i) {
		if (m->rowCount(m->index(i, 0)) != 0)
			setFirstColumnSpanned(i, QModelIndex(), true);
	}
}

// If items were selected, inform the selection model
void DiveListView::divesSelectedSlot(const QVector<QModelIndex> &indices, QModelIndex currentDive, int currentDC)
{
	// This is the entry point for programmatical selection changes.
	// Set a flag so that selection changes are not further processed,
	// since the core structures were already set.
	programmaticalSelectionChange = true;

	clearSelection();
	QItemSelection selection;
	for (const QModelIndex &index: indices)
		selection.select(index, index); // Is there a faster way to do this?
	selectionModel()->select(selection, QItemSelectionModel::Rows | QItemSelectionModel::Select);

	// Expand all unexpanded trips
	std::vector<int> affectedTrips;
	for (const QModelIndex &index: indices) {
		if (!index.parent().isValid())
			continue;
		int row = index.parent().row();
		if (std::find(affectedTrips.begin(), affectedTrips.end(), row) == affectedTrips.end())
			affectedTrips.push_back(row);
	}
	// Disable animations when expanding trips. With animations, selection of
	// a large number of dives becomes unbearably slow.
	bool oldAnimated = isAnimated();
	setAnimated(false);
	MultiFilterSortModel *m = MultiFilterSortModel::instance();
	for (int row: affectedTrips) {
		QModelIndex idx = m->index(row, 0);
		expand(idx);
	}
	setAnimated(oldAnimated);

	// This is truly sad, but taking the list of selected indices and turning them
	// into dives turned out to be unreasonably slow. Therefore, let's access
	// the core list directly.
	std::vector<dive *> selected = getDiveSelection();
	selectDiveSitesOnMap(selected);
	struct dive *current = currentDive.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	emit divesSelected(selected, current, currentDC);

	programmaticalSelectionChange = false;

	// Set the currently activated row.
	// Note, we have to use the QItemSelectionModel::Current mode to avoid
	// changing our selection (in contrast to Qt's documentation, which
	// instructs to use QItemSelectionModel::NoUpdate, which results in
	// funny side-effects).
	selectionModel()->setCurrentIndex(currentDive, QItemSelectionModel::Current);
}

// If rows are added, check which of these rows is a trip and expand the first column
void DiveListView::rowsInserted(const QModelIndex &parent, int start, int end)
{
	// First, let the QTreeView do its thing.
	QTreeView::rowsInserted(parent, start, end);

	// Check for each inserted row whether this is a trip and expand the first column
	QAbstractItemModel *m = model();
	if (parent.isValid()) // Trips don't have a parent
		return;
	for (int i = start; i <= end; ++i) {
		if (m->rowCount(m->index(i, 0)) != 0)
			setFirstColumnSpanned(i, QModelIndex(), true);
	}
}

void DiveListView::tripSelected(QModelIndex trip, QModelIndex currentDive)
{
	programmaticalSelectionChange = true;

	// clear the Qt selection
	selectionModel()->clearSelection();

	if (trip.isValid()) {
		QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::Select | QItemSelectionModel::Rows;
		selectionModel()->select(trip, flags);
		selectTripItems(trip);
	}

	// Set the currently activated row.
	// Note, we have to use the QItemSelectionModel::Current mode to avoid
	// changing our selection (in contrast to Qt's documentation, which
	// instructs to use QItemSelectionModel::NoUpdate, which results in
	// funny side-effects).
	selectionModel()->setCurrentIndex(currentDive, QItemSelectionModel::Current);

	// This is truly sad, but taking the list of selected indices and turning them
	// into dives turned out to be unreasonably slow. Therefore, let's access
	// the core list directly.
	std::vector<dive *> selection = getDiveSelection();
	selectDiveSitesOnMap(selection);
	struct dive *current = currentDive.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	emit divesSelected(selection, current, -1);

	programmaticalSelectionChange = false;
}

bool DiveListView::eventFilter(QObject *, QEvent *event)
{
	if (event->type() != QEvent::KeyPress)
		return false;
	QKeyEvent *keyEv = static_cast<QKeyEvent *>(event);
	if (keyEv->key() == Qt::Key_Delete) {
		contextMenuIndex = currentIndex();
		deleteDive();
	}
	if (keyEv->key() != Qt::Key_Escape)
		return false;
	return true;
}

void DiveListView::sortIndicatorChanged(int i, Qt::SortOrder order)
{
	DiveTripModelBase::Layout newLayout = i == (int)DiveTripModelBase::NR ? DiveTripModelBase::TREE : DiveTripModelBase::LIST;
	/* No layout change? Just re-sort, and scroll to first selection, making sure all selections are expanded */
	if (currentLayout == newLayout) {
		sortByColumn(i, order);
	} else {
		// clear the model, repopulate with new indices.
		std::vector<int> expandedRows;
		if(currentLayout == DiveTripModelBase::TREE)
			expandedRows = backupExpandedRows();
		currentLayout = newLayout;
		resetModel();
		sortByColumn(i, order);
		if (newLayout == DiveTripModelBase::TREE)
			restoreExpandedRows(expandedRows);
	}
}

void DiveListView::setSortOrder(int i, Qt::SortOrder order)
{
	// The QHeaderView will call our signal if the sort order changed
	header()->setSortIndicator(i, order);
}

void DiveListView::reload()
{
	resetModel();

	if (selectedIndexes().count()) {
		QModelIndex curr = selectedIndexes().first();
		curr = curr.parent().isValid() ? curr.parent() : curr;
		if (!isExpanded(curr)) {
			setAnimated(false);
			expand(curr);
			scrollTo(curr);
			setAnimated(true);
		}
	}
}

void DiveListView::settingsChanged()
{
	update();

	for (int i = 0; i < model()->columnCount(); i++) {
		QString title = model()->headerData(i, Qt::Horizontal).toString();
		header()->actions()[i]->setText(title);
	}
}

void DiveListView::toggleColumnVisibilityByIndex()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;
	const int idx = action->property("index").toInt();

	// Count the number of visible columns.
	int totalVisible = 0, lastVisibleIdx = -1;
	for (int i = 0; i < model()->columnCount(); i++) {
		if (isColumnHidden(i))
			continue;
		totalVisible++;
		lastVisibleIdx = i;
	}
	// If there is only one visible column and we are performing an action on it,
	// don't hide the column and revert the action back to checked == true.
	// This keeps one column visible at all times.
	if (totalVisible == 1 && idx == lastVisibleIdx) {
		action->setChecked(true);
		return;
	}

	QSettings s;
	s.beginGroup("DiveListColumnState");
	s.setValue(action->property("settingName").toString(), action->isChecked());
	s.endGroup();
	s.sync();
	setColumnHidden(idx, !action->isChecked());
	setColumnWidth(lastVisibleColumn(), 10);
}

void DiveListView::currentChanged(const QModelIndex &current, const QModelIndex&)
{
	if (!isVisible())
		return;
	if (!current.isValid())
		return;
	scrollTo(current);
}

void DiveListView::selectDiveSitesOnMap(const std::vector<dive *> &dives)
{
#ifdef MAP_SUPPORT
	// When receiving the divesSelected signal the main window will
	// instruct the map to update the flags. Thus, make sure that
	// the selected maps are registered correctly.
	// But don't do this if we are in divesite mode, because then
	// the dive-site selection is controlled by the filter not
	// by the selected dives.
	if (!DiveFilter::instance()->diveSiteMode()) {
		QVector<dive_site *> selectedSites;
		selectedSites.reserve(dives.size());
		for (dive *d: dives) {
			if (!d->hidden_by_filter && d->dive_site && !selectedSites.contains(d->dive_site))
				selectedSites.push_back(d->dive_site);
		}
		MapWidget::instance()->setSelected(selectedSites);
	}
#endif
}

void DiveListView::selectTripItems(QModelIndex index)
{
	const QAbstractItemModel *model = index.model();
	if (model->rowCount(index)) {
		QItemSelection selection;
		selection.select(model->index(0, 0, index), model->index(model->rowCount(index) - 1, 0, index));
		selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
		if (!isExpanded(index))
			expand(index);
	}
}

void DiveListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	if (programmaticalSelectionChange) {
		// This is a programmatical change of the selection.
		// Call the QTreeView base function to reflect the selection in the display,
		// but don't process it any further.
		QTreeView::selectionChanged(selected, deselected);
		return;
	}

	// Avoid recursion: if later we decide to select all dives of a trip,
	// this function is called again. Avoid this by setting the
	// programmaticalSelectionChange flag. Note that the recursion never
	// was a problem, but it just feels correct to avoid this recursion.
	programmaticalSelectionChange = true;

	// This is a manual selection change. This means that the core does not yet know
	// of the new selection. Update the core-structures accordingly and select/deselect
	// all dives of a trip if a trip is selected/deselected.
	const QItemSelection &newDeselected = deselected;

	QItemSelection newSelected = selected.size() ? selected : selectionModel()->selection();

	std::vector<dive *> addToSelection, removeFromSelection;
	Q_FOREACH (const QModelIndex &index, newDeselected.indexes()) {
		if (index.column() != 0)
			continue;
		const QAbstractItemModel *model = index.model();
		struct dive *dive = model->data(index, DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
		if (dive) {
			removeFromSelection.push_back(dive);
		} else if (dive_trip *trip = model->data(index, DiveTripModelBase::TRIP_ROLE).value<dive_trip *>()) {
			deselect_trip(trip);
			for (int i = 0; i < trip->dives.nr; ++i)
				removeFromSelection.push_back(trip->dives.dives[i]);
		}
	}
	Q_FOREACH (const QModelIndex &index, newSelected.indexes()) {
		if (index.column() != 0)
			continue;

		const QAbstractItemModel *model = index.model();
		struct dive *dive = model->data(index, DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
		if (dive) {
			addToSelection.push_back(dive);
		} else if (dive_trip *trip = model->data(index, DiveTripModelBase::TRIP_ROLE).value<dive_trip *>()) {
			select_trip(trip);
			for (int i = 0; i < trip->dives.nr; ++i)
				addToSelection.push_back(trip->dives.dives[i]);
			selectTripItems(index);
		}
	}

	// Funny: It can happen that dives were added to the add and remove lists.
	// For example, when switching from a trip to a single dive in the trip.
	// To avoid ill-define situations, clean that up.
	for (dive *d: addToSelection) {
		auto it = std::find(removeFromSelection.begin(), removeFromSelection.end(), d);
		if (it != removeFromSelection.end())
			removeFromSelection.erase(it);
	}

	std::vector<dive *> selection = getDiveSelection();
	updateSelection(selection, addToSelection, removeFromSelection);
	dive *newCurrent = selection.empty() ? nullptr : selection.front();
	setSelectionCore(selection, newCurrent);

	// Display the new, processed, selection
	QTreeView::selectionChanged(selectionModel()->selection(), newDeselected);

	selectDiveSitesOnMap(selection);
	emit divesSelected(selection, newCurrent, -1);

	programmaticalSelectionChange = false;
}

enum asked_user {NOTYET, MERGE, DONTMERGE};

static bool can_merge(const struct dive *a, const struct dive *b, enum asked_user *have_asked)
{
	if (!a || !b)
		return false;
	if (a->when > b->when)
		return false;
	/* Don't merge dives if there's more than half an hour between them */
	if (dive_endtime(a) + 30 * 60 < b->when) {
		if (*have_asked == NOTYET) {
			if (QMessageBox::warning(MainWindow::instance(),
						 MainWindow::tr("Warning"),
						 MainWindow::tr("Trying to merge dives with %1min interval in between").arg(
							 (b->when - dive_endtime(a)) / 60),
					     QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
				*have_asked = DONTMERGE;
				return false;
			} else {
				*have_asked = MERGE;
				return true;
			}
		} else {
			return *have_asked == MERGE ? true : false;
		}
	}
	return true;
}

void DiveListView::mergeDives()
{
	enum asked_user have_asked = NOTYET;

	// Collect a vector of batches of dives to merge (i.e. a vector of vector of dives)
	QVector<QVector<dive *>> merge_batches;
	QVector<dive *> current_batch;
	for (dive *d: getDiveSelection()) {
		if (current_batch.empty()) {
			current_batch.append(d);
		} else if (can_merge(current_batch.back(), d, &have_asked)) {
			current_batch.append(d);
		} else {
			if (current_batch.count() > 1)
				merge_batches.append(current_batch);
			current_batch.clear();
		}
	}
	if (current_batch.count() > 1)
		merge_batches.append(current_batch);

	for (const QVector<dive *> &batch: merge_batches)
		Command::mergeDives(batch);
}

void DiveListView::splitDives()
{
	for (struct dive *d: getDiveSelection())
		Command::splitDives(d, duration_t{-1});
}

void DiveListView::addDivesToTrip()
{
	TripSelectionDialog dialog(MainWindow::instance());
	dive_trip *t = dialog.getTrip();
	std::vector<dive *> dives = getDiveSelection();
	if (!t || dives.empty())
		return;
	Command::addDivesToTrip(stdToQt<dive *>(dives), t);
}

void DiveListView::renumberDives()
{
	RenumberDialog dialog(true, MainWindow::instance());
	dialog.exec();
}

void DiveListView::merge_trip(const QModelIndex &a, int offset)
{
	int i = a.row() + offset;
	QModelIndex b = a.sibling(i, 0);

	dive_trip_t *trip_a = a.data(DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();
	dive_trip_t *trip_b = b.data(DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();
	if (trip_a == trip_b || !trip_a || !trip_b)
		return;
	Command::mergeTrips(trip_a, trip_b);
}

void DiveListView::mergeTripAbove()
{
	merge_trip(contextMenuIndex, -1);
}

void DiveListView::mergeTripBelow()
{
	merge_trip(contextMenuIndex, +1);
}

void DiveListView::removeFromTrip()
{
	Command::removeDivesFromTrip(stdToQt(getDiveSelection()));
}

void DiveListView::newTripAbove()
{
	struct dive *d = contextMenuIndex.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	if (!d) // shouldn't happen as we only are setting up this action if this is a dive
		return;
	Command::createTrip(stdToQt(getDiveSelection()));
}

void DiveListView::addToTripBelow()
{
	addToTrip(1);
}

void DiveListView::addToTripAbove()
{
	addToTrip(-1);
}

void DiveListView::addToTrip(int delta)
{
	// d points to the row that has (mouse-)pointer focus, and there are nr rows selected
	struct dive *d = contextMenuIndex.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	int nr = selectionModel()->selectedRows().count();
	QModelIndex t;
	dive_trip_t *trip = NULL;

	// now look for the trip to add to, for this, loop over the selected dives and
	// check if its sibling is a trip.
	for (int i = 1; i <= nr; i++) {
		t = contextMenuIndex.sibling(contextMenuIndex.row() + (delta > 0 ? i: i * -1), 0);
		trip = t.data(DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();
		if (trip)
			break;
	}

	if (!trip || !d)
		// no dive, no trip? get me out of here
		return;
	std::vector<dive *> dives = getDiveSelection();
	Command::addDivesToTrip(stdToQt<dive *>(dives), trip);
}

void DiveListView::markDiveInvalid()
{
	Command::editInvalid(true, false);
}

void DiveListView::markDiveValid()
{
	Command::editInvalid(false, false);
}

void DiveListView::deleteDive()
{
	struct dive *d = contextMenuIndex.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	if (!d)
		return;
	Command::deleteDive(stdToQt<dive *>(getDiveSelection()));
}

void DiveListView::contextMenuEvent(QContextMenuEvent *event)
{
	QAction *collapseAction = NULL;
	// let's remember where we are
	contextMenuIndex = indexAt(event->pos());
	struct dive *d = contextMenuIndex.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	dive_trip_t *trip = contextMenuIndex.data(DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();
	QMenu popup(this);
	if (currentLayout == DiveTripModelBase::TREE) {
		// verify if there is a node that`s not expanded.
		bool needs_expand = false;
		bool needs_collapse = false;
		uint expanded_nodes = 0;
		for(int i = 0, end = model()->rowCount(); i < end; i++) {
			QModelIndex idx = model()->index(i, 0);
			if (idx.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>())
				continue;

			if (!isExpanded(idx)) {
				needs_expand = true;
			} else {
				needs_collapse = true;
				expanded_nodes ++;
			}
		}
		if (needs_expand)
			popup.addAction(tr("Expand all"), this, &QTreeView::expandAll);
		if (needs_collapse)
			popup.addAction(tr("Collapse all"), this, &QTreeView::collapseAll);

		// verify if there`s a need for collapse others
		if (expanded_nodes > 1 && (trip || (d && d->divetrip)))
			collapseAction = popup.addAction(tr("Collapse others"), this, &QTreeView::collapseAll);


		if (d) {
			if (d->divetrip) {
				popup.addAction(tr("Remove dive(s) from trip","",amount_selected), this, &DiveListView::removeFromTrip);
			}
			popup.addAction(tr("Create new trip above"), this, &DiveListView::newTripAbove);
			if (!d->divetrip) {
				struct dive *top = d;
				struct dive *bottom = d;
				Qt::SortOrder currentOrder = header()->sortIndicatorOrder();
				if (d->selected) {
					if (currentOrder == Qt::AscendingOrder) {
						top = first_selected_dive();
						bottom = last_selected_dive();
					} else {
						top = last_selected_dive();
						bottom = first_selected_dive();
					}
				}
				if (is_trip_before_after(top, (currentOrder == Qt::AscendingOrder)))
					popup.addAction(tr("Add dive(s) to trip immediately above","",amount_selected), this, &DiveListView::addToTripAbove);
				if (is_trip_before_after(bottom, (currentOrder == Qt::DescendingOrder)))
					popup.addAction(tr("Add dive(s) to trip immediately below","",amount_selected), this, &DiveListView::addToTripBelow);
			}
		}
		if (trip) {
			popup.addAction(tr("Merge trip with trip above"), this, &DiveListView::mergeTripAbove);
			popup.addAction(tr("Merge trip with trip below"), this, &DiveListView::mergeTripBelow);
		}
	}
	if (d) {
		popup.addAction(tr("Delete dive(s)","",amount_selected), this, &DiveListView::deleteDive);
		if (d->invalid)
			popup.addAction(tr("Mark dive(s) valid","",amount_selected), this, &DiveListView::markDiveValid);
		else
			popup.addAction(tr("Mark dive(s) invalid","",amount_selected), this, &DiveListView::markDiveInvalid);
	}
	if (amount_selected > 1 && consecutive_selected())
		popup.addAction(tr("Merge selected dives"), this, &DiveListView::mergeDives);
	if (amount_selected >= 1) {
		popup.addAction(tr("Add dive(s) to arbitrary trip","",amount_selected), this, &DiveListView::addDivesToTrip);
		popup.addAction(tr("Renumber dive(s)","",amount_selected), this, &DiveListView::renumberDives);
		popup.addAction(tr("Shift dive times"), this, &DiveListView::shiftTimes);
		popup.addAction(tr("Split selected dives"), this, &DiveListView::splitDives);
		popup.addAction(tr("Load media from file(s)"), this, &DiveListView::loadImages);
		popup.addAction(tr("Load media from web"), this, &DiveListView::loadWebImages);
	}

	// "collapse all" really closes all trips,
	// "collapse" keeps the trip with the selected dive open
	QAction *actionTaken = popup.exec(event->globalPos());
	if (actionTaken == collapseAction && collapseAction) {
		this->setAnimated(false);
		scrollTo(selectedIndexes().first());
		this->setAnimated(true);
	}
	event->accept();
}

void DiveListView::shiftTimes()
{
	std::vector<dive *> dives = getDiveSelection();
	if (dives.empty())
		return;
	ShiftTimesDialog dialog(std::move(dives), MainWindow::instance());
	dialog.exec();
}

void DiveListView::loadImages()
{
	QStringList m_filters = mediaExtensionFilters();
	QStringList i_filters = imageExtensionFilters();
	QStringList v_filters = videoExtensionFilters();
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
							      tr("Open media files"),
							      lastUsedImageDir(),
							      QString("%1 (%2);;%3 (%4);;%5 (%6);;%7 (*.*)")
							      .arg(tr("Media files"), m_filters.join(" ")
							      , tr("Image files"), i_filters.join(" ")
							      , tr("Video files"), v_filters.join(" ")
							      , tr("All files")));

	if (fileNames.isEmpty())
		return;
	updateLastUsedImageDir(QFileInfo(fileNames[0]).dir().path());
	matchImagesToDives(fileNames);
}

void DiveListView::matchImagesToDives(const QStringList &fileNames)
{
	ShiftImageTimesDialog shiftDialog(this, fileNames);
	shiftDialog.setOffset(lastImageTimeOffset());
	if (!shiftDialog.exec())
		return;
	updateLastImageTimeOffset(shiftDialog.amount());

	// Create the data structure of pictures to be added: a list of pictures per dive.
	std::vector<Command::PictureListForAddition> pics;
	for (const QString &fileName: fileNames) {
		struct dive *d;
		picture *pic = create_picture(qPrintable(fileName), shiftDialog.amount(), shiftDialog.matchAll(), &d);
		if (!pic)
			continue;
		PictureObj pObj(*pic);
		free(pic);

		auto it = std::find_if(pics.begin(), pics.end(), [d](const Command::PictureListForAddition &l) { return l.d == d; });
		if (it == pics.end())
			pics.push_back(Command::PictureListForAddition { d, { std::move(pObj) } });
		else
			it->pics.push_back(std::move(pObj));
	}

	if (pics.empty())
		return;

	Command::addPictures(pics);
}

void DiveListView::loadWebImages()
{
	URLDialog urlDialog(this);
	if (!urlDialog.exec())
		return;
	loadImagesFromURLs(urlDialog.url());
}

void DiveListView::loadImagesFromURLs(const QString &urls)
{
	QStringList validUrls;
	QStringList urlList = urls.split('\n');
	QStringList::ConstIterator userUrl;
	for (userUrl = urlList.constBegin(); userUrl != urlList.constEnd(); ++userUrl) {
		QUrl url = QUrl::fromUserInput(*userUrl);
		if (url.isValid()) {
			QEventLoop loop;
			QNetworkRequest request(url);
			QNetworkReply *reply = manager.get(request);
			while (reply->isRunning()) {
				loop.processEvents();
				sleep(1);
			}
			QByteArray imageData = reply->readAll();

			QImage image = QImage();
			image.loadFromData(imageData);
			if (image.isNull()) {
				// If this is not an image, maybe it's an html file and Miika can provide some xslr magic to extract images.
				// In this case we would call the function recursively on the list of image source urls;
				report_error(qPrintable(tr("%1 does not appear to be an image").arg(url.toString())));
				return;
			}

			QCryptographicHash hash(QCryptographicHash::Sha1);
			hash.addData(url.toString().toUtf8());
			QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
			QDir dir(path);
			if (!dir.exists())
				dir.mkpath(path);
			QFile imageFile(path.append("/").append(hash.result().toHex()));
			if (imageFile.open(QIODevice::WriteOnly)) {
				QDataStream stream(&imageFile);
				stream.writeRawData(imageData.data(), imageData.length());
				imageFile.waitForBytesWritten(-1);
				imageFile.close();
				learnPictureFilename(url.toString(), imageFile.fileName());
				validUrls << url.toString();
			}
		}
	}
	matchImagesToDives(validUrls);
}

QString DiveListView::lastUsedImageDir()
{
	QSettings settings;
	QString lastImageDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastImageDir"))
		if (QDir(settings.value("LastImageDir").toString()).exists())
			lastImageDir = settings.value("LastImageDir").toString();
	return lastImageDir;
}

void DiveListView::updateLastUsedImageDir(const QString &dir)
{
	QSettings s;
	s.beginGroup("FileDialog");
	s.setValue("LastImageDir", dir);
}

int DiveListView::lastImageTimeOffset()
{
	QSettings settings;
	int offset = 0;

	settings.beginGroup("MainWindow");
	if (settings.contains("LastImageTimeOffset"))
		offset = settings.value("LastImageTimeOffset").toInt();
	return offset;
}

void DiveListView::updateLastImageTimeOffset(timestamp_t offset)
{
	QSettings s;
	s.beginGroup("MainWindow");
	// Can't create a QVariant from int64_t, for whatever reason.
	s.setValue("LastImageTimeOffset", static_cast<long long>(offset));
}

void DiveListView::mouseDoubleClickEvent(QMouseEvent*)
{
	return;
}
