// SPDX-License-Identifier: GPL-2.0
/*
 * divelistview.cpp
 *
 * classes for the divelist of Subsurface
 *
 */
#include "qt-models/filtermodels.h"
#include "desktop-widgets/modeldelegates.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divepicturewidget.h"
#include "core/display.h"
#include <unistd.h>
#include <QSettings>
#include <QKeyEvent>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QMessageBox>
#include <QHeaderView>
#include "commands/command.h"
#include "core/errorhelper.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "desktop-widgets/divelistview.h"
#include "qt-models/divepicturemodel.h"
#include "core/metrics.h"
#include "desktop-widgets/simplewidgets.h"
#include "desktop-widgets/mapwidget.h"

DiveListView::DiveListView(QWidget *parent) : QTreeView(parent), mouseClickSelection(false),
	currentLayout(DiveTripModelBase::TREE), dontEmitDiveChangedSignal(false), selectionSaved(false),
	initialColumnWidths(DiveTripModelBase::COLUMNS, 50)	// Set up with default length 50
{
	setItemDelegate(new DiveListDelegate(this));
	setUniformRowHeights(true);
	setItemDelegateForColumn(DiveTripModelBase::RATING, new StarWidgetsDelegate(this));
	setModel(MultiFilterSortModel::instance());

	setSortingEnabled(true);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	setSelectionMode(ExtendedSelection);
	header()->setContextMenuPolicy(Qt::ActionsContextMenu);

	resetModel();

	// Update selection if all selected dives were hidden by filter
	connect(MultiFilterSortModel::instance(), &MultiFilterSortModel::filterFinished, this, &DiveListView::filterFinished);

	connect(&diveListNotifier, &DiveListNotifier::tripChanged, this, &DiveListView::tripChanged);

	header()->setStretchLastSection(true);
	header()->setSortIndicatorShown(true);
	header()->setSectionsClickable(true);
	connect(header(), &QHeaderView::sortIndicatorChanged, this, &DiveListView::sortIndicatorChanged);

	installEventFilter(this);

	for (int i = DiveTripModelBase::NR; i < DiveTripModelBase::COLUMNS; i++)
		calculateInitialColumnWidth(i);
	setColumnWidths();
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
	MultiFilterSortModel::instance()->resetModel(currentLayout);
	// If the model was reset, we have to reconnect the signals and tell
	// the filter model to update its source model.
	connect(DiveTripModelBase::instance(), &DiveTripModelBase::selectionChanged, this, &DiveListView::diveSelectionChanged);
	connect(DiveTripModelBase::instance(), &DiveTripModelBase::newCurrentDive, this, &DiveListView::currentDiveChanged);
}

void DiveListView::calculateInitialColumnWidth(int col)
{
	const QFontMetrics metrics(defaultModelFont());
	int em = metrics.width('m');
	int zw = metrics.width('0');

	QString header_txt = DiveTripModelBase::instance()->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
	int width = metrics.width(header_txt);
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
	backupExpandedRows();
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
	restoreExpandedRows();
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

void DiveListView::backupExpandedRows()
{
	expandedRows.clear();
	for (int i = 0; i < model()->rowCount(); i++)
		if (isExpanded(model()->index(i, 0)))
			expandedRows.push_back(i);
}

void DiveListView::restoreExpandedRows()
{
	setAnimated(false);
	Q_FOREACH (const int &i, expandedRows)
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
void DiveListView::diveSelectionChanged(const QVector<QModelIndex> &indexes)
{
	// Since dives are selected dive-by-dive, send only a single signal at the
	// end, not one for every dive.
	dontEmitDiveChangedSignal = true;

	clearSelection();
	MultiFilterSortModel *m = MultiFilterSortModel::instance();
	QItemSelectionModel *s = selectionModel();
	for (const QModelIndex &index: indexes) {
		// We have to transform the indices into local indices, since
		// there might be sorting or filtering in effect.
		QModelIndex localIndex = m->mapFromSource(index);

		// It might be possible that the item is not shown (filter is
		// in effect). Then we get an invalid index and should ignore
		// this selection.
		if (!localIndex.isValid())
			continue;

		s->select(localIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);

		// If an item of a not-yet expanded trip is selected, expand the trip.
		if (localIndex.parent().isValid() && !isExpanded(localIndex.parent())) {
			setAnimated(false);
			expand(localIndex.parent());
			setAnimated(true);
		}
	}

	dontEmitDiveChangedSignal = false;
	emit divesSelected();
}

void DiveListView::currentDiveChanged(QModelIndex index)
{
	// Transform the index into a local index, since
	// there might be sorting or filtering in effect.
	MultiFilterSortModel *m = MultiFilterSortModel::instance();
	QModelIndex localIndex = m->mapFromSource(index);

	// Then, set the currently activated row.
	// Note, we have to use the QItemSelectionModel::Current mode to avoid
	// changing our selection (in contrast to Qt's documentation, which
	// instructs to use QItemSelectionModel::NoUpdate, which results in
	// funny side-effects).
	selectionModel()->setCurrentIndex(localIndex, QItemSelectionModel::Current);
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

// this only remembers dives that were selected, not trips
void DiveListView::rememberSelection()
{
	selectedDives.clear();
	QItemSelection selection = selectionModel()->selection();
	Q_FOREACH (const QModelIndex &index, selection.indexes()) {
		if (index.column() != 0) // We only care about the dives, so, let's stick to rows and discard columns.
			continue;
		struct dive *d = index.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
		if (d) {
			selectedDives.insert(d->divetrip, get_divenr(d));
		} else {
			struct dive_trip *t = index.data(DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();
			if (t)
				selectedDives.insert(t, -1);
		}
	}
	selectionSaved = true;
}

void DiveListView::restoreSelection()
{
	if (!selectionSaved)
		return;

	selectionSaved = false;
	QList<int> divesToSelect;
	Q_FOREACH (dive_trip_t *trip, selectedDives.keys()) {
		QList<int> divesOnTrip = getDivesInTrip(trip);
		QList<int> selectedDivesOnTrip = selectedDives.values(trip);

		// Only select trip if all of its dives were selected
		if(selectedDivesOnTrip.contains(-1)) {
			selectTrip(trip);
			selectedDivesOnTrip.removeAll(-1);
		}
		divesToSelect += selectedDivesOnTrip;
	}
	selectDives(divesToSelect);
}

// This is a bit ugly: we hook directly into the tripChanged signal to
// select the trip if it was edited. This feels like a layering violation:
// Shouldn't the core-layer call us?
void DiveListView::tripChanged(dive_trip *trip, TripField)
{
	// First check if the trip is already selected (and only
	// this trip, as only then is it displayed). Is so, then do nothing.
	QList<dive_trip *> selected = selectedTrips();
	if (selected.size() == 1 && selected[0] == trip)
		return;

	dontEmitDiveChangedSignal = true;
	unselectDives();
	dontEmitDiveChangedSignal = false;
	selectTrip(trip);
}

void DiveListView::selectTrip(dive_trip_t *trip)
{
	if (!trip)
		return;

	QAbstractItemModel *m = model();
	QModelIndexList match = m->match(m->index(0, 0), DiveTripModelBase::TRIP_ROLE, QVariant::fromValue(trip), 2, Qt::MatchRecursive);
	QItemSelectionModel::SelectionFlags flags;
	if (!match.count())
		return;
	QModelIndex idx = match.first();
	flags = QItemSelectionModel::Select;
	flags |= QItemSelectionModel::Rows;
	selectionModel()->select(idx, flags);
	expand(idx);
}

// this is an odd one - when filtering the dive list the selection status of the trips
// is kept - but all other selections are lost. That's gets us into rather inconsistent state
// we call this function which clears the selection state of the trips as well, but does so
// without updating our internal "->selected" state. So once we called this function we can
// go back and select those dives that are still visible under the filter and everything
// works as expected
void DiveListView::clearTripSelection()
{
	// This marks the selection change as being internal - ie. we don't process it further.
	// TODO: This should probably be done differently.
	auto marker = diveListNotifier.enterCommand();

	// we want to make sure no trips are selected
	Q_FOREACH (const QModelIndex &index, selectionModel()->selectedRows()) {
		if (!index.data(DiveTripModelBase::IS_TRIP_ROLE).toBool())
			continue;
		selectionModel()->select(index, QItemSelectionModel::Deselect);
	}
}

void DiveListView::unselectDives()
{
	// make sure we don't try to redraw the dives during the selection change
	current_dive = nullptr;
	amount_selected = 0;
	// clear the Qt selection
	selectionModel()->clearSelection();
	// clearSelection should emit selectionChanged() but sometimes that
	// appears not to happen
	// since we are unselecting all dives there is no need to use deselect_dive() - that
	// would only cause pointless churn
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		dive->selected = false;
	}
}

QList<dive_trip_t *> DiveListView::selectedTrips()
{
	QList<dive_trip_t *> ret;
	Q_FOREACH (const QModelIndex &index, selectionModel()->selectedRows()) {
		dive_trip_t *trip = index.data(DiveTripModelBase::TRIP_ROLE).value<dive_trip *>();
		if (!trip)
			continue;
		ret.push_back(trip);
	}
	return ret;
}

void DiveListView::selectDive(QModelIndex idx, bool scrollto, bool toggle)
{
	if (!idx.isValid())
		return;
	QItemSelectionModel::SelectionFlags flags = toggle ? QItemSelectionModel::Toggle : QItemSelectionModel::Select;
	flags |= QItemSelectionModel::Rows;
	selectionModel()->setCurrentIndex(idx, flags);
	if (idx.parent().isValid()) {
		setAnimated(false);
		expand(idx.parent());
		if (scrollto)
			scrollTo(idx.parent());
		setAnimated(true);
	}
	if (scrollto)
		scrollTo(idx, PositionAtCenter);
}

void DiveListView::selectDive(int i, bool scrollto, bool toggle)
{
	if (i == -1)
		return;
	QAbstractItemModel *m = model();
	QModelIndexList match = m->match(m->index(0, 0), DiveTripModelBase::DIVE_IDX, i, 2, Qt::MatchRecursive);
	if (match.isEmpty())
		return;
	QModelIndex idx = match.first();
	selectDive(idx, scrollto, toggle);
}

void DiveListView::selectDives(const QList<int> &newDiveSelection)
{
	int firstInList, newSelection;
	struct dive *d;

	if (!newDiveSelection.count())
		return;

	dontEmitDiveChangedSignal = true;

	// First, clear the old selection
	unselectDives();

	// select the dives, highest index first - this way the oldest of the dives
	// becomes the selected_dive that we scroll to
	QList<int> sortedSelection = newDiveSelection;
	std::sort(sortedSelection.begin(), sortedSelection.end());
	newSelection = firstInList = sortedSelection.first();

	while (!sortedSelection.isEmpty())
		selectDive(sortedSelection.takeLast());

	while (!current_dive) {
		// that can happen if we restored a selection after edit
		// and the only selected dive is no longer visible because of a filter
		newSelection--;
		if (newSelection < 0)
			newSelection = dive_table.nr - 1;
		if (newSelection == firstInList)
			break;
		if ((d = get_dive(newSelection)) != NULL && !d->hidden_by_filter)
			selectDive(newSelection);
	}
	QAbstractItemModel *m = model();
	QModelIndexList idxList = m->match(m->index(0, 0), DiveTripModelBase::DIVE_IDX, get_divenr(current_dive), 2, Qt::MatchRecursive);
	if (!idxList.isEmpty()) {
		QModelIndex idx = idxList.first();
		if (idx.parent().isValid())
			scrollTo(idx.parent());
		scrollTo(idx);
	}

	// update the selected-flag for the dive sites.
	// the actual reloading of the dive sites will be perfomed
	// by the main-window in response to the divesSelected signal
	// emitted below.
	// But don't do this if we are in divesite mode, because then
	// the dive-site selection is controlled by the filter not
	// by the selected dives.
	if (!MultiFilterSortModel::instance()->diveSiteMode()) {
		QVector<dive_site *> selectedSites;
		for (int idx: newDiveSelection) {
			dive *d = get_dive(idx);
			if (!d)
				continue;
			dive_site *ds = d->dive_site;
			if (ds && !selectedSites.contains(ds))
				selectedSites.append(ds);
		}
		MapWidget::instance()->setSelected(selectedSites);
	}

	// now that everything is up to date, update the widgets
	emit divesSelected();
	dontEmitDiveChangedSignal = false;
	return;
}

// Get index of first dive. This assumes that trips without dives are never shown.
// May return an invalid index if no dive is found.
QModelIndex DiveListView::indexOfFirstDive()
{
	// Fetch the first top-level item. If this is a trip, it is supposed to have at least
	// one child. In that case return the child. Otherwise return the top-level item, which
	// should be a dive.
	QAbstractItemModel *m = model();
	QModelIndex firstDiveOrTrip = m->index(0, 0);
	if (!firstDiveOrTrip.isValid())
		return QModelIndex();
	QModelIndex child = m->index(0, 0, firstDiveOrTrip);
	return child.isValid() ? child : firstDiveOrTrip;
}

void DiveListView::selectFirstDive()
{
	QModelIndex first = indexOfFirstDive();
	if (first.isValid())
		setCurrentIndex(first);
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
		// clear the model, repopulate with new indexes.
		rememberSelection();
		unselectDives();
		if (currentLayout == DiveTripModelBase::TREE)
			backupExpandedRows();
		currentLayout = newLayout;
		resetModel();
		sortByColumn(i, order);
		if (newLayout == DiveTripModelBase::TREE)
			restoreExpandedRows();
		restoreSelection();
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

	if (amount_selected && current_dive != NULL)
		selectDive(get_divenr(current_dive), true);
	else
		selectFirstDive();
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

void DiveListView::reloadHeaderActions()
{
	// Populate the context menu of the headers that will show
	// the menu to show / hide columns.
	if (!header()->actions().size()) {
		QSettings s;
		s.beginGroup("DiveListColumnState");
		for (int i = 0; i < model()->columnCount(); i++) {
			QString title = QString("%1").arg(model()->headerData(i, Qt::Horizontal).toString());
			QString settingName = QString("showColumn%1").arg(i);
			QAction *a = new QAction(title, header());
			bool showHeaderFirstRun = !(i == DiveTripModelBase::MAXCNS ||
						    i == DiveTripModelBase::GAS ||
						    i == DiveTripModelBase::OTU ||
						    i == DiveTripModelBase::TEMPERATURE ||
						    i == DiveTripModelBase::TOTALWEIGHT ||
						    i == DiveTripModelBase::SUIT ||
						    i == DiveTripModelBase::CYLINDER ||
						    i == DiveTripModelBase::SAC ||
						    i == DiveTripModelBase::TAGS);
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
	} else {
		for (int i = 0; i < model()->columnCount(); i++) {
			QString title = QString("%1").arg(model()->headerData(i, Qt::Horizontal).toString());
			header()->actions()[i]->setText(title);
		}
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

void DiveListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	if (diveListNotifier.inCommand()) {
		// This is a programmatical change of the selection.
		// Call the QTreeView base function to reflect the selection in the display,
		// but don't process it any further.
		QTreeView::selectionChanged(selected, deselected);
		return;
	}

	// This is a manual selection change. This means that the core does not yet know
	// of the new selection. Update the core-structures accordingly and select/deselect
	// all dives of a trip if a trip is selected/deselected.
	const QItemSelection &newDeselected = deselected;

	QItemSelection newSelected = selected.size() ? selected : selectionModel()->selection();

	Q_FOREACH (const QModelIndex &index, newDeselected.indexes()) {
		if (index.column() != 0)
			continue;
		const QAbstractItemModel *model = index.model();
		struct dive *dive = model->data(index, DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
		if (!dive) // it's a trip!
			deselect_dives_in_trip(model->data(index, DiveTripModelBase::TRIP_ROLE).value<dive_trip *>());
		else
			deselect_dive(dive);
	}
	Q_FOREACH (const QModelIndex &index, newSelected.indexes()) {
		if (index.column() != 0)
			continue;

		const QAbstractItemModel *model = index.model();
		struct dive *dive = model->data(index, DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				QItemSelection selection;
				select_dives_in_trip(model->data(index, DiveTripModelBase::TRIP_ROLE).value<dive_trip *>());
				selection.select(index.child(0, 0), index.child(model->rowCount(index) - 1, 0));
				selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::NoUpdate);
				if (!isExpanded(index))
					expand(index);
			}
		} else {
			select_dive(dive);
		}
	}
	if (!dontEmitDiveChangedSignal) {
		// When receiving the divesSelected signal the main window will
		// instruct the map to update the flags. Thus, make sure that
		// the selected maps are registered correctly.
		// But don't do this if we are in divesite mode, because then
		// the dive-site selection is controlled by the filter not
		// by the selected dives.
		if (!MultiFilterSortModel::instance()->diveSiteMode()) {
			QVector<dive_site *> selectedSites;
			for (QModelIndex index: selectionModel()->selection().indexes()) {
				const QAbstractItemModel *model = index.model();
				struct dive *dive = model->data(index, DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
				if (dive && dive->dive_site)
					selectedSites.push_back(dive->dive_site);
			}
			MapWidget::instance()->setSelected(selectedSites);
		}
		emit divesSelected();
	}

	// Display the new, processed, selection
	QTreeView::selectionChanged(selectionModel()->selection(), newDeselected);
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
	int i;
	struct dive *d;
	enum asked_user have_asked = NOTYET;

	// Collect a vector of batches of dives to merge (i.e. a vector of vector of dives)
	QVector<QVector<dive *>> merge_batches;
	QVector<dive *> current_batch;
	for_each_dive (i, d) {
		if (!d->selected)
			continue;
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
	int i;
	struct dive *dive;

	// Let's collect the dives to be split first, so that we don't catch newly inserted dives!
	QVector<struct dive *> dives;
	for_each_dive (i, dive) {
		if (dive->selected)
			dives.append(dive);
	}
	for (struct dive *d: dives)
		Command::splitDives(d, duration_t{-1});
}

void DiveListView::renumberDives()
{
	RenumberDialog::instance()->renumberOnlySelected();
	RenumberDialog::instance()->show();
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
	//TODO: move this to C-code.
	int i;
	struct dive *d;
	QVector<dive *> divesToRemove;
	for_each_dive (i, d) {
		if (d->selected && d->divetrip)
			divesToRemove.append(d);
	}
	Command::removeDivesFromTrip(divesToRemove);
}

void DiveListView::newTripAbove()
{
	struct dive *d = contextMenuIndex.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	if (!d) // shouldn't happen as we only are setting up this action if this is a dive
		return;
	//TODO: port to c-code.
	int idx;
	QVector<dive *> dives;
	for_each_dive (idx, d) {
		if (d->selected)
			dives.append(d);
	}
	Command::createTrip(dives);
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

	QVector<dive *> dives;
	if (d->selected) { // there are possibly other selected dives that we should add
		int idx;
		for_each_dive (idx, d) {
			if (d->selected)
				dives.append(d);
		}
	}
	Command::addDivesToTrip(dives, trip);
}

void DiveListView::markDiveInvalid()
{
	int i;
	struct dive *d = contextMenuIndex.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	if (!d)
		return;
	for_each_dive (i, d) {
		if (!d->selected)
			continue;
		//TODO: this should be done in the future
		// now mark the dive invalid... how do we do THAT?
		// d->invalid = true;
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
	if (prefs.display_invalid_dives == false) {
		clearSelection();
	}
}

void DiveListView::deleteDive()
{
	struct dive *d = contextMenuIndex.data(DiveTripModelBase::DIVE_ROLE).value<struct dive *>();
	if (!d)
		return;

	int i;
	QVector<struct dive*> deletedDives;
	for_each_dive (i, d) {
		if (d->selected)
			deletedDives.append(d);
	}
	Command::deleteDive(deletedDives);
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
		if (expanded_nodes > 1)
			collapseAction = popup.addAction(tr("Collapse others"), this, &QTreeView::collapseAll);


		if (d) {
			popup.addAction(tr("Remove dive(s) from trip"), this, &DiveListView::removeFromTrip);
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
					popup.addAction(tr("Add dive(s) to trip immediately above"), this, &DiveListView::addToTripAbove);
				if (is_trip_before_after(bottom, (currentOrder == Qt::DescendingOrder)))
					popup.addAction(tr("Add dive(s) to trip immediately below"), this, &DiveListView::addToTripBelow);
			}
		}
		if (trip) {
			popup.addAction(tr("Merge trip with trip above"), this, &DiveListView::mergeTripAbove);
			popup.addAction(tr("Merge trip with trip below"), this, &DiveListView::mergeTripBelow);
		}
	}
	if (d) {
		popup.addAction(tr("Delete dive(s)"), this, &DiveListView::deleteDive);
#if 0
		popup.addAction(tr("Mark dive(s) invalid", this, &DiveListView::markDiveInvalid);
#endif
	}
	if (amount_selected > 1 && consecutive_selected())
		popup.addAction(tr("Merge selected dives"), this, &DiveListView::mergeDives);
	if (amount_selected >= 1) {
		popup.addAction(tr("Renumber dive(s)"), this, &DiveListView::renumberDives);
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
		selectDive(get_divenr(current_dive), true);
		scrollTo(selectedIndexes().first());
		this->setAnimated(true);
	}
	event->accept();
}

void DiveListView::shiftTimes()
{
	ShiftTimesDialog::instance()->show();
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

void DiveListView::matchImagesToDives(QStringList fileNames)
{
	ShiftImageTimesDialog shiftDialog(this, fileNames);
	shiftDialog.setOffset(lastImageTimeOffset());
	if (!shiftDialog.exec())
		return;
	updateLastImageTimeOffset(shiftDialog.amount());

	for (const QString &fileName: fileNames)
		create_picture(qPrintable(fileName), shiftDialog.amount(), shiftDialog.matchAll());

	mark_divelist_changed(true);
	copy_dive(current_dive, &displayed_dive);
	DivePictureModel::instance()->updateDivePictures();
}

void DiveListView::loadWebImages()
{
	URLDialog urlDialog(this);
	if (!urlDialog.exec())
		return;
	loadImageFromURL(QUrl::fromUserInput(urlDialog.url()));
}

void DiveListView::loadImageFromURL(QUrl url)
{
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
			matchImagesToDives(QStringList(url.toString()));
		}
	}
}

void DiveListView::filterFinished()
{
	// first make sure the trips are no longer shown as selected
	// (but without updating the selection state of the dives... this just cleans
	// up an oddity in the filter handling)
	clearTripSelection();

	// If there are no more selected dives, select the first visible dive
	if (!selectionModel()->hasSelection())
		selectFirstDive();
	emit divesSelected();
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

void DiveListView::updateLastImageTimeOffset(const int offset)
{
	QSettings s;
	s.beginGroup("MainWindow");
	s.setValue("LastImageTimeOffset", offset);
}

void DiveListView::mouseDoubleClickEvent(QMouseEvent*)
{
	return;
}
