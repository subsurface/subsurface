#include "filtermodels.h"
#include "dive.h"
#include "models.h"
#include "mainwindow.h"
#include "display.h"

#define CREATE_INSTANCE_METHOD( CLASS ) \
CLASS *CLASS::instance() \
{ \
	static CLASS *self = new CLASS(); \
	return self; \
}

CREATE_INSTANCE_METHOD(TagFilterModel);
CREATE_INSTANCE_METHOD(BuddyFilterModel);
CREATE_INSTANCE_METHOD(LocationFilterModel);

#undef CREATE_INSTANCE_METHOD

TagFilterModel::TagFilterModel(QObject *parent) : QStringListModel(parent)
{
}

QVariant TagFilterModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::CheckStateRole) {
		return checkState[index.row()] ? Qt::Checked : Qt::Unchecked;
	} else if (role == Qt::DisplayRole) {
		QString tag = stringList()[index.row()];
		int count = count_dives_with_tag(tag.toUtf8().data());
		return tag + QString(" (%1)").arg(count);
	}
	return QVariant();
}

Qt::ItemFlags TagFilterModel::flags(const QModelIndex &index) const
{
	return QStringListModel::flags(index) | Qt::ItemIsUserCheckable;
}

void TagFilterModel::repopulate()
{
	if (g_tag_list == NULL)
		return;
	QStringList list;
	struct tag_entry *current_tag_entry = g_tag_list->next;
	while (current_tag_entry != NULL) {
		list.append(QString(current_tag_entry->tag->name));
		current_tag_entry = current_tag_entry->next;
	}
	qSort(list);
	list << tr("Empty Tags");
	setStringList(list);
	delete[] checkState;
	checkState = new bool[list.count()];
	memset(checkState, false, list.count());
	checkState[list.count() - 1] = false;
	anyChecked = false;
}

bool TagFilterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::CheckStateRole) {
		checkState[index.row()] = value.toBool();
		anyChecked = false;
		for (int i = 0; i < rowCount(); i++) {
			if (checkState[i] == true) {
				anyChecked = true;
				break;
			}
		}
		dataChanged(index, index);
		return true;
	}
	return false;
}

bool TagFilterModel::doFilter(dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const
{
	// If there's nothing checked, this should show everything
	if (!anyChecked) {
		return true;
	}
	if (!d) { // It's a trip, only show the ones that have dives to be shown.
		for (int i = 0; i < sourceModel->rowCount(index0); i++) {
			if (filterRow(i, index0, sourceModel))
				return true;
		}
		return false;
	}
	// Checked means 'Show', Unchecked means 'Hide'.
	struct tag_entry *head = d->tag_list;

	if (!head) { // last tag means "Show empty tags";
		if (rowCount() > 0)
			return checkState[rowCount() - 1];
		else
			return true;
	}

	// have at least one tag.
	QStringList tagList = stringList();
	if (!tagList.isEmpty()) {
		tagList.removeLast(); // remove the "Show Empty Tags";
		while (head) {
			QString tagName(head->tag->name);
			int index = tagList.indexOf(tagName);
			if (checkState[index])
				return true;
			head = head->next;
		}
	}
	return false;
}

bool TagFilterModel::filterRow(int source_row, const QModelIndex &source_parent, QAbstractItemModel *sourceModel) const
{
	// If there's nothing checked, this should show everything
	if (!anyChecked) {
		return true;
	}

	QModelIndex index0 = sourceModel->index(source_row, 0, source_parent);
	QVariant diveVariant = sourceModel->data(index0, DiveTripModel::DIVE_ROLE);
	struct dive *d = (struct dive *)diveVariant.value<void *>();

	bool show = doFilter(d, index0, sourceModel);
	filter_dive(d, show);
	return show;
}

BuddyFilterModel::BuddyFilterModel(QObject *parent) : QStringListModel(parent)
{
}

bool BuddyFilterModel::doFilter(dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const
{
	// If there's nothing checked, this should show everything
	if (!anyChecked) {
		return true;
	}
	if (!d) { // It's a trip, only show the ones that have dives to be shown.
		for (int i = 0; i < sourceModel->rowCount(index0); i++) {
			if (filterRow(i, index0, sourceModel))
				return true;
		}
		return false;
	}

	// Checked means 'Show', Unchecked means 'Hide'.
	QString diveBuddy(d->buddy);
	QString divemaster(d->divemaster);
	// only show empty buddie dives if the user checked that.
	if (diveBuddy.isEmpty() && divemaster.isEmpty()) {
		if (rowCount() > 0)
			return checkState[rowCount() - 1];
		else
			return true;
	}

	// have at least one buddy
	QStringList buddyList = stringList();
	if (!buddyList.isEmpty()) {
		buddyList.removeLast(); // remove the "Show Empty Tags";
		for (int i = 0; i < rowCount(); i++) {
			if (checkState[i] && (diveBuddy.indexOf(stringList()[i]) != -1 || divemaster.indexOf(stringList()[i]) != -1)) {
				return true;
			}
		}
	}
	return false;
}

bool BuddyFilterModel::filterRow(int source_row, const QModelIndex &source_parent, QAbstractItemModel *sourceModel) const
{
	// If there's nothing checked, this should show everything
	if (!anyChecked) {
		return true;
	}

	QModelIndex index0 = sourceModel->index(source_row, 0, source_parent);
	QVariant diveVariant = sourceModel->data(index0, DiveTripModel::DIVE_ROLE);
	struct dive *d = (struct dive *)diveVariant.value<void *>();

	return doFilter(d, index0, sourceModel);
}

Qt::ItemFlags BuddyFilterModel::flags(const QModelIndex &index) const
{
	return QStringListModel::flags(index) | Qt::ItemIsUserCheckable;
}

void BuddyFilterModel::repopulate()
{
	QStringList list;
	struct dive *dive;
	int i = 0;
	for_each_dive (i, dive) {
		QString persons = QString(dive->buddy) + "," + QString(dive->divemaster);
		Q_FOREACH(const QString& person, persons.split(',', QString::SkipEmptyParts)){
			// Remove any leading spaces
			if (!list.contains(person.trimmed())) {
				list.append(person.trimmed());
			}
		}
	}
	qSort(list);
	list << tr("No buddies");
	setStringList(list);
	delete[] checkState;
	checkState = new bool[list.count()];
	memset(checkState, false, list.count());
	checkState[list.count() - 1] = false;
	anyChecked = false;
}

QVariant BuddyFilterModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::CheckStateRole) {
		return checkState[index.row()] ? Qt::Checked : Qt::Unchecked;
	} else if (role == Qt::DisplayRole) {
		QString person = stringList()[index.row()];
		int count = count_dives_with_person(person.toUtf8().data());
		return person + QString(" (%1)").arg(count);
	}
	return QVariant();
}


bool BuddyFilterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::CheckStateRole) {
		checkState[index.row()] = value.toBool();
		anyChecked = false;
		for (int i = 0; i < rowCount(); i++) {
			if (checkState[i] == true) {
				anyChecked = true;
				break;
			}
		}
		dataChanged(index, index);
		return true;
	}
	return false;
}

LocationFilterModel::LocationFilterModel(QObject *parent) : QStringListModel(parent)
{
}

QVariant LocationFilterModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::CheckStateRole) {
		return checkState[index.row()] ? Qt::Checked : Qt::Unchecked;
	} else if (role == Qt::DisplayRole) {
		QString location = stringList()[index.row()];
		int count = count_dives_with_location(location.toUtf8().data());
		return location + QString(" (%1)").arg(count);
	}
	return QVariant();
}

bool LocationFilterModel::doFilter(struct dive *d, QModelIndex &index0, QAbstractItemModel *sourceModel) const
{
	if (!anyChecked) {
		return true;
	}

	if (!d) { // It's a trip, only show the ones that have dives to be shown.
		for (int i = 0; i < sourceModel->rowCount(index0); i++) {
			if (filterRow(i, index0, sourceModel))
				return true;
		}
		return false;
	}

	// Checked means 'Show', Unchecked means 'Hide'.
	QString location(d->location);
	// only show empty location dives if the user checked that.
	if (location.isEmpty()) {
		if (rowCount() > 0)
			return checkState[rowCount() - 1];
		else
			return true;
	}

	// there is a location selected
	QStringList locationList = stringList();
	if (!locationList.isEmpty()) {
		locationList.removeLast(); // remove the "Show Empty Tags";
		for (int i = 0; i < rowCount(); i++) {
			if (checkState[i] && (location.indexOf(stringList()[i]) != -1)) {
				return true;
			}
		}
	}
	return false;
}

bool LocationFilterModel::filterRow(int source_row, const QModelIndex &source_parent, QAbstractItemModel *sourceModel) const
{

	// If there's nothing checked, this should show everything
	if (!anyChecked) {
		return true;
	}

	QModelIndex index0 = sourceModel->index(source_row, 0, source_parent);
	QVariant diveVariant = sourceModel->data(index0, DiveTripModel::DIVE_ROLE);
	struct dive *d = (struct dive *)diveVariant.value<void *>();

	return doFilter(d, index0, sourceModel);
}

Qt::ItemFlags LocationFilterModel::flags(const QModelIndex &index) const
{
	return QStringListModel::flags(index) | Qt::ItemIsUserCheckable;
}

void LocationFilterModel::repopulate()
{
	QStringList list;
	struct dive *dive;
	int i = 0;
	for_each_dive (i, dive) {
		QString location(dive->location);
		if (!location.isEmpty() && !list.contains(location)) {
			list.append(location);
		}
	}
	qSort(list);
	list << tr("No location set");
	setStringList(list);
	delete[] checkState;
	checkState = new bool[list.count()];
	memset(checkState, false, list.count());
	checkState[list.count() - 1] = false;
	anyChecked = false;
}

bool LocationFilterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::CheckStateRole) {
		checkState[index.row()] = value.toBool();
		anyChecked = false;
		for (int i = 0; i < rowCount(); i++) {
			if (checkState[i] == true) {
				anyChecked = true;
				break;
			}
		}
		dataChanged(index, index);
		return true;
	}
	return false;
}

MultiFilterSortModel *MultiFilterSortModel::instance()
{
	static MultiFilterSortModel *self = new MultiFilterSortModel();
	return self;
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent), justCleared(false)
{
}

bool MultiFilterSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	if (justCleared || models.isEmpty())
		return true;

	bool shouldShow = true;
	QModelIndex index0 = sourceModel()->index(source_row, 0, source_parent);
	QVariant diveVariant = sourceModel()->data(index0, DiveTripModel::DIVE_ROLE);
	struct dive *d = (struct dive *)diveVariant.value<void *>();

	Q_FOREACH (MultiFilterInterface *model, models) {
		if (!model->doFilter(d, index0, sourceModel()))
			shouldShow = false;
	}
	// if it's a dive, mark it accordingly
	filter_dive(d, shouldShow);
	return shouldShow;
}

void MultiFilterSortModel::myInvalidate()
{
	int i;
	struct dive *d;
	DiveListView *dlv = MainWindow::instance()->dive_list();

	invalidate();
	// first make sure the trips are no longer shown as selected
	// (but without updating the selection state of the dives... this just cleans
	//  up an oddity in the filter handling)
	// TODO: This should go internally to DiveList, to be triggered after a filter is due.
	dlv->clearTripSelection();

	// if we have no more selected dives, clean up the display - this later triggers us
	// to pick one of the dives that are shown in the list as selected dive which is the
	// natural behavior
	if (amount_selected == 0) {
		MainWindow::instance()->cleanUpEmpty();
	} else {
		// otherwise find the dives that should still be selected (the filter above unselected any
		// dive that's no longer visible) and select them again
		QList<int>curSelectedDives;
		for_each_dive (i, d) {
			if(d->selected)
				curSelectedDives.append(get_divenr(d));
		}
		dlv->selectDives(curSelectedDives);
	}
}

void MultiFilterSortModel::addFilterModel(MultiFilterInterface *model)
{
	QAbstractItemModel *itemModel = dynamic_cast<QAbstractItemModel *>(model);
	Q_ASSERT(itemModel);
	models.append(model);
	connect(itemModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(myInvalidate()));
}

void MultiFilterSortModel::removeFilterModel(MultiFilterInterface *model)
{
	QAbstractItemModel *itemModel = dynamic_cast<QAbstractItemModel *>(model);
	Q_ASSERT(itemModel);
	models.removeAll(model);
	disconnect(itemModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(myInvalidate()));
}

void MultiFilterSortModel::clearFilter()
{
	justCleared = true;
	Q_FOREACH(MultiFilterInterface *iface, models){
		iface->clearFilter();
	}
	justCleared = false;
	myInvalidate();
}

void BuddyFilterModel::clearFilter()
{
	memset(checkState, false, rowCount());
	checkState[rowCount() - 1] = false;
	anyChecked = false;
	emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
}

void LocationFilterModel::clearFilter()
{
	memset(checkState, false, rowCount());
	checkState[rowCount() - 1] = false;
	anyChecked = false;
	emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
}

void TagFilterModel::clearFilter()
{
	memset(checkState, false, rowCount());
	checkState[rowCount() - 1] = false;
	anyChecked = false;
	emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
}