/*
 * divelistview.cpp
 *
 * classes for the divelist of Subsurface
 *
 */
#include "divelistview.h"
#include "models.h"
#include "modeldelegates.h"
#include "mainwindow.h"
#include "../display.h"
#include <QApplication>
#include <QHeaderView>
#include <QDebug>
#include <QSettings>
#include <QKeyEvent>
#include <QSortFilterProxyModel>
#include <QAction>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QFileDialog>

DiveListView::DiveListView(QWidget *parent) : QTreeView(parent), mouseClickSelection(false),
	sortColumn(0), currentOrder(Qt::DescendingOrder), searchBox(new QLineEdit(this))
{
	setUniformRowHeights(true);
	setItemDelegateForColumn(DiveTripModel::RATING, new StarWidgetsDelegate());
	QSortFilterProxyModel *model = new QSortFilterProxyModel(this);
	model->setSortRole(DiveTripModel::SORT_ROLE);
	model->setFilterKeyColumn(-1); // filter all columns
	setModel(model);
	connect(model, SIGNAL(layoutChanged()), this, SLOT(fixMessyQtModelBehaviour()));

	setSortingEnabled(false);
	setContextMenuPolicy(Qt::DefaultContextMenu);
	header()->setContextMenuPolicy(Qt::ActionsContextMenu);
	const QFontMetrics metrics(defaultModelFont());
	header()->setMinimumHeight(metrics.height() + 10);
	header()->setStretchLastSection(true);
	QAction *showSearchBox = new QAction(tr("Show Search Box"), this);
	showSearchBox->setShortcut( Qt::CTRL + Qt::Key_F);
	showSearchBox->setShortcutContext(Qt::ApplicationShortcut);
	addAction(showSearchBox);

	searchBox->installEventFilter(this);
	searchBox->hide();
	connect(showSearchBox, SIGNAL(triggered(bool)), this, SLOT(showSearchEdit()));
	connect(searchBox, SIGNAL(textChanged(QString)), model, SLOT(setFilterFixedString(QString)));
	selectedTrips.clear();

	setupUi();
}

DiveListView::~DiveListView()
{
	QSettings settings;
	settings.beginGroup("ListWidget");
	for (int i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++){
		if (isColumnHidden(i))
			continue;
		settings.setValue(QString("colwidth%1").arg(i), columnWidth(i));
	}
	settings.endGroup();
}

void DiveListView::setupUi(){
	QSettings settings;
	static bool firstRun = true;
	if(firstRun)
		backupExpandedRows();
	settings.beginGroup("ListWidget");
	/* if no width are set, use the calculated width for each column;
	 * for that to work we need to temporarily expand all rows */
	expandAll();
	for (int i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++) {
		if(isColumnHidden(i))
			continue;
		QVariant width = settings.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			setColumnWidth(i, width.toInt());
		else
			setColumnWidth(i, 100);
	}
	settings.endGroup();
	if(firstRun)
		restoreExpandedRows();
	else
		collapseAll();
	firstRun = false;
	setColumnWidth(lastVisibleColumn(), 10);
}

int DiveListView::lastVisibleColumn()
{
	int lastColumn = -1;
	for (int i = DiveTripModel::NR; i < DiveTripModel::COLUMNS; i++) {
		if(isColumnHidden(i))
			continue;
		lastColumn = i;
	}
	return lastColumn;
}

void DiveListView::backupExpandedRows(){
	expandedRows.clear();
	for(int i = 0; i < model()->rowCount(); i++)
		if(isExpanded( model()->index(i, 0) ))
			expandedRows.push_back(i);
}

void DiveListView::restoreExpandedRows(){
	setAnimated(false);
	Q_FOREACH(const int &i, expandedRows)
		setExpanded( model()->index(i, 0), true );
	setAnimated(true);
}
void DiveListView::fixMessyQtModelBehaviour()
{
	QAbstractItemModel *m = model();
	for(int i = 0; i < model()->rowCount(); i++)
		if (m->rowCount( m->index(i, 0) ) != 0)
			setFirstColumnSpanned(i, QModelIndex(), true);
}

// this only remembers dives that were selected, not trips
void DiveListView::rememberSelection()
{
	selectedDives.clear();
	QItemSelection selection = selectionModel()->selection();
	Q_FOREACH(const QModelIndex& index , selection.indexes()) {
		if (index.column() != 0) // We only care about the dives, so, let's stick to rows and discard columns.
			continue;
		struct dive *d = (struct dive *) index.data(DiveTripModel::DIVE_ROLE).value<void*>();
		if (d)
			selectedDives.push_front(get_divenr(d));
	}
}

void DiveListView::restoreSelection()
{
	unselectDives();
	Q_FOREACH(int i, selectedDives) {
		selectDive(i);
	}
}

void DiveListView::unselectDives()
{
	selectionModel()->clearSelection();
}

void DiveListView::selectDive(int i, bool scrollto, bool toggle)
{
	if( i == -1)
		return;
	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	QModelIndexList match = m->match(m->index(0,0), DiveTripModel::DIVE_IDX, i, 2, Qt::MatchRecursive);
	QItemSelectionModel::SelectionFlags flags;
	QModelIndex idx = match.first();
	flags = toggle ? QItemSelectionModel::Toggle : QItemSelectionModel::Select;
	flags |= QItemSelectionModel::Rows;
	selectionModel()->select(idx, flags);
	if(idx.parent().isValid()){
		setAnimated(false);
		expand(idx.parent());
		setAnimated(true);
	}
	if (scrollto)
		scrollTo(idx, PositionAtCenter);
}
void DiveListView::showSearchEdit()
{
	searchBox->show();
	searchBox->setFocus();
}

bool DiveListView::eventFilter(QObject* , QEvent* event)
{
	if(event->type() != QEvent::KeyPress)
		return false;
	QKeyEvent *keyEv = static_cast<QKeyEvent*>(event);
	if (keyEv->key() != Qt::Key_Escape)
		return false;

	searchBox->clear();
	searchBox->hide();
	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	m->setFilterFixedString(QString());
	return true;
}

// NOTE! This loses trip selection, because while we remember the
// dives, we don't remember the trips (see the "currentSelectedDives"
// list). I haven't figured out how to look up the trip from the
// index. TRIP_ROLE vs DIVE_ROLE?
void DiveListView::headerClicked(int i)
{
	DiveTripModel::Layout newLayout = i == (int) DiveTripModel::NR ? DiveTripModel::TREE : DiveTripModel::LIST;
	rememberSelection();
	unselectDives();
	/* No layout change? Just re-sort, and scroll to first selection, making sure all selections are expanded */
	if (currentLayout == newLayout) {
		currentOrder = (currentOrder == Qt::DescendingOrder) ? Qt::AscendingOrder : Qt::DescendingOrder;
		sortByColumn(i, currentOrder);
	} else {
		// clear the model, repopulate with new indexes.
		if(currentLayout == DiveTripModel::TREE){
			backupExpandedRows();
		}
		reload(newLayout, false);
		currentOrder = Qt::DescendingOrder;
		sortByColumn(i, currentOrder);
		if (newLayout == DiveTripModel::TREE){
			restoreExpandedRows();
		}
	}
	restoreSelection();
}

void DiveListView::reload(DiveTripModel::Layout layout, bool forceSort)
{
	if (layout == DiveTripModel::CURRENT)
		layout = currentLayout;
	else
		currentLayout = layout;
	header()->setClickable(true);
	connect(header(), SIGNAL(sectionPressed(int)), this, SLOT(headerClicked(int)), Qt::UniqueConnection);

	QSortFilterProxyModel *m = qobject_cast<QSortFilterProxyModel*>(model());
	QAbstractItemModel *oldModel = m->sourceModel();
	if (oldModel){
		oldModel->deleteLater();
	}
	DiveTripModel *tripModel = new DiveTripModel(this);
	tripModel->setLayout(layout);

	m->setSourceModel(tripModel);

	if(!forceSort)
		return;

	sortByColumn(sortColumn, currentOrder);
	if (amount_selected && current_dive != NULL) {
		selectDive(selected_dive, true);
	} else {
		QModelIndex firstDiveOrTrip = m->index(0,0);
		if (firstDiveOrTrip.isValid()) {
			if (m->index(0,0, firstDiveOrTrip).isValid())
				setCurrentIndex(m->index(0,0, firstDiveOrTrip));
			else
				setCurrentIndex(firstDiveOrTrip);
		}
	}
	setupUi();
	if(selectedIndexes().count()){
		QModelIndex curr = selectedIndexes().first();
		curr = curr.parent().isValid() ? curr.parent() : curr;
		if(!isExpanded(curr)){
			setAnimated(false);
			expand(curr);
			setAnimated(true);
		}
	}
	if(currentLayout == DiveTripModel::TREE){
		fixMessyQtModelBehaviour();
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
			bool showHeaderFirstRun = !(
				i == DiveTripModel::MAXCNS
				|| i == DiveTripModel::NITROX
				|| i == DiveTripModel::OTU
				|| i == DiveTripModel::TEMPERATURE
				|| i == DiveTripModel::TOTALWEIGHT
				|| i == DiveTripModel::SUIT
				|| i == DiveTripModel::CYLINDER
				|| i == DiveTripModel::SAC );
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
		// Skip first QAction item ( static text Visible )
		for(int i = 0; i < model()->columnCount(); i++) {
			QString title = QString("%1").arg(model()->headerData(i, Qt::Horizontal).toString());
			header()->actions()[i+1]->setText( title );
		}
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
	setColumnWidth(lastVisibleColumn(), 10);
}

void DiveListView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
	if (!current.isValid())
		return;
	scrollTo(current);
}

void DiveListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	QItemSelection newSelected = selected.size() ? selected : selectionModel()->selection();
	QItemSelection newDeselected = deselected;

	disconnect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),         this, SLOT(currentChanged(QModelIndex,QModelIndex)));

	Q_FOREACH(const QModelIndex& index, newDeselected.indexes()) {
		if (index.column() != 0)
			continue;
		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, DiveTripModel::DIVE_ROLE).value<void*>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				struct dive *child = (struct dive*) model->data(index.child(0,0), DiveTripModel::DIVE_ROLE).value<void*>();
				if (child && child->divetrip)
					selectedTrips.remove(child->divetrip);
				while (child) {
					deselect_dive(get_divenr(child));
					child = child->next;
				}
			}
		} else {
			deselect_dive(get_divenr(dive));
		}
	}
	Q_FOREACH(const QModelIndex& index, newSelected.indexes()) {
		if (index.column() != 0)
			continue;

		const QAbstractItemModel *model = index.model();
		struct dive *dive = (struct dive*) model->data(index, DiveTripModel::DIVE_ROLE).value<void*>();
		if (!dive) { // it's a trip!
			if (model->rowCount(index)) {
				QItemSelection selection;
				struct dive *child = (struct dive*) model->data(index.child(0,0), DiveTripModel::DIVE_ROLE).value<void*>();
				if (child && child->divetrip)
					selectedTrips.insert(child->divetrip);
				while (child) {
					select_dive(get_divenr(child));
					child = child->next;
				}
				selection.select(index.child(0,0), index.child(model->rowCount(index) -1 , 0));
				selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
				selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select | QItemSelectionModel::NoUpdate);
				if (!isExpanded(index))
					expand(index);
			}
		} else {
			select_dive(get_divenr(dive));
		}
	}
	QTreeView::selectionChanged(selectionModel()->selection(), newDeselected);
	connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
	connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),         this, SLOT(currentChanged(QModelIndex,QModelIndex)));
	// now that everything is up to date, update the widgets
	Q_EMIT currentDiveChanged(selected_dive);
}

static bool can_merge(const struct dive *a, const struct dive *b)
{
	if (!a || !b)
		return false;
	if (a->when > b->when)
		return false;
	/* Don't merge dives if there's more than half an hour between them */
	if (a->when + a->duration.seconds + 30*60 < b->when)
		return false;
	return true;
}

void DiveListView::mergeDives()
{
	int i;
	struct dive *dive, *maindive = NULL;

	for_each_dive(i, dive) {
		if (dive->selected) {
			if (!can_merge(maindive, dive)) {
				maindive = dive;
			} else {
				maindive = merge_two_dives(maindive, dive);
				i--; // otherwise we skip a dive in the freshly changed list
			}
		}
	}
	mainWindow()->refreshDisplay();
}

void DiveListView::merge_trip(const QModelIndex &a, int offset)
{
	int i = a.row() + offset;
	QModelIndex b = a.sibling(i,0);

	dive_trip_t *trip_a = (dive_trip_t *) a.data(DiveTripModel::TRIP_ROLE).value<void*>();
	dive_trip_t *trip_b = (dive_trip_t *) b.data(DiveTripModel::TRIP_ROLE).value<void*>();

	if (trip_a == trip_b || !trip_a || !trip_b)
		return;

	if (!trip_a->location && trip_b->location)
		trip_a->location = strdup(trip_b->location);
	if (!trip_a->notes && trip_b->notes)
		trip_a->notes = strdup(trip_b->notes);
	while (trip_b->dives)
		add_dive_to_trip(trip_b->dives, trip_a);
	rememberSelection();
	reload(currentLayout, false);
	fixMessyQtModelBehaviour();
	restoreSelection();
	mark_divelist_changed(TRUE);
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
	int i;
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (!d) // shouldn't happen as we only are setting up this action if this is a dive
		return;
	for_each_dive(i, d) {
		if (d->selected)
			remove_dive_from_trip(d);
	}
	mark_divelist_changed(TRUE);
	reload(currentLayout, false);
}

void DiveListView::newTripAbove()
{
	dive_trip_t *trip;
	int idx;
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (!d) // shouldn't happen as we only are setting up this action if this is a dive
		return;
	rememberSelection();
	trip = create_and_hookup_trip_from_dive(d);
	for_each_dive(idx, d) {
		if (d->selected)
			add_dive_to_trip(d, trip);
	}
	trip->expanded = 1;
	mark_divelist_changed(TRUE);
	reload(currentLayout, false);
	restoreSelection();
}

void DiveListView::deleteDive()
{
	int i, nr;
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (!d)
		return;
	// after a dive is deleted the ones following it move forward in the dive_table
	// so instead of using the for_each_dive macro I'm using an explicit for loop
	// to make this easier to understand
	int lastDiveNr = -1;
	for (i = 0; i < dive_table.nr; i++) {
		d = get_dive(i);
		if (!d->selected)
			continue;
		delete_single_dive(i);
		i--; // so the next dive isn't skipped... it's now #i
		lastDiveNr = i;
	}
	if (amount_selected == 0) {
		if (i > 0)
			select_dive(nr - 1);
		else
			mainWindow()->cleanUpEmpty();
	}
	mark_divelist_changed(TRUE);
	mainWindow()->refreshDisplay();
	if(lastDiveNr != -1){
		clearSelection();
		selectDive(lastDiveNr);
		rememberSelection();
	}
}

void DiveListView::testSlot()
{
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	if (d) {
		qDebug("testSlot called on dive #%d", d->number);
	} else {
		QModelIndex child = contextMenuIndex.child(0, 0);
		d = (struct dive *) child.data(DiveTripModel::DIVE_ROLE).value<void*>();
		if (d)
			qDebug("testSlot called on trip including dive #%d", d->number);
		else
			qDebug("testSlot called on trip with no dive");
	}
}

void DiveListView::contextMenuEvent(QContextMenuEvent *event)
{
	QAction *collapseAction = NULL;
	// let's remember where we are
	contextMenuIndex = indexAt(event->pos());
	struct dive *d = (struct dive *) contextMenuIndex.data(DiveTripModel::DIVE_ROLE).value<void*>();
	dive_trip_t *trip = (dive_trip_t *) contextMenuIndex.data(DiveTripModel::TRIP_ROLE).value<void*>();
	QMenu popup(this);
	if (currentLayout == DiveTripModel::TREE) {
		popup.addAction(tr("expand all"), this, SLOT(expandAll()));
		popup.addAction(tr("collapse all"), this, SLOT(collapseAll()));
		collapseAction = popup.addAction(tr("collapse others"), this, SLOT(collapseAll()));
		if (d) {
			popup.addAction(tr("remove dive(s) from trip"), this, SLOT(removeFromTrip()));
			popup.addAction(tr("create new trip above"), this, SLOT(newTripAbove()));
		}
		if (trip) {
			popup.addAction(tr("merge trip with trip above"), this, SLOT(mergeTripAbove()));
			popup.addAction(tr("merge trip with trip below"), this, SLOT(mergeTripBelow()));
		}
	}
	if (d)
		popup.addAction(tr("delete dive(s)"), this, SLOT(deleteDive()));
	if (amount_selected > 1 && consecutive_selected())
		popup.addAction(tr("merge selected dives"), this, SLOT(mergeDives()));
	if (amount_selected >= 1) {
		popup.addAction(tr("save As"), this, SLOT(saveSelectedDivesAs()));
		popup.addAction(tr("export As UDDF"), this, SLOT(exportSelectedDivesAsUDDF()));
	}
	// "collapse all" really closes all trips,
	// "collapse" keeps the trip with the selected dive open
	QAction * actionTaken = popup.exec(event->globalPos());
	if (actionTaken == collapseAction && collapseAction) {
		this->setAnimated(false);
		selectDive(selected_dive, true);
		scrollTo(selectedIndexes().first());
		this->setAnimated(true);
	}
	event->accept();
}

void DiveListView::saveSelectedDivesAs()
{
	QSettings settings;
	QString lastDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastDir")) {
		if(QDir::setCurrent(settings.value("LastDir").toString())) {
			lastDir = settings.value("LastDir").toString();
		}
	}
	settings.endGroup();

	QString fileName = QFileDialog::getSaveFileName(mainWindow(), tr("Save Dives As..."), QDir::homePath());
	if (fileName.isEmpty())
		return;

	// Keep last open dir
	QFileInfo fileInfo(fileName);
	settings.beginGroup("FileDialog");
	settings.setValue("LastDir",fileInfo.dir().path());
	settings.endGroup();

	QByteArray bt = fileName.toLocal8Bit();
	save_dives_logic(bt.data(), TRUE);
}

void DiveListView::exportSelectedDivesAsUDDF()
{
	QString filename;
	QFileInfo fi(system_default_filename());

	filename = QFileDialog::getSaveFileName(this, tr("Save File as"), fi.absolutePath(),
						tr("UDDF files (*.uddf *.UDDF)"));
	if (!filename.isNull() && !filename.isEmpty())
		export_dives_uddf(filename.toUtf8(), true);
}
