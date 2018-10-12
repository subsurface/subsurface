// SPDX-License-Identifier: GPL-2.0
#include "qt-models/filtermodels.h"
#include "qt-models/models.h"
#include "core/display.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "qt-models/divetripmodel.h"

#if !defined(SUBSURFACE_MOBILE)
#include "desktop-widgets/divelistview.h"
#include "desktop-widgets/mainwindow.h"
#endif

#include <QDebug>
#include <algorithm>

#define CREATE_INSTANCE_METHOD(CLASS)             \
	CLASS *CLASS::instance()                  \
	{                                         \
		static CLASS *self = new CLASS(); \
		return self;                      \
	}

CREATE_INSTANCE_METHOD(TagFilterModel)
CREATE_INSTANCE_METHOD(BuddyFilterModel)
CREATE_INSTANCE_METHOD(LocationFilterModel)
CREATE_INSTANCE_METHOD(SuitsFilterModel)
CREATE_INSTANCE_METHOD(MultiFilterSortModel)

FilterModelBase::FilterModelBase(QObject *parent) : QAbstractListModel(parent),
	anyChecked(false),
	negate(false)
{
}

// Get index of item with given name, but ignore last item, as this
// is the "Show Empty Tags" item. Return -1 for not found.
int FilterModelBase::indexOf(const QString &name) const
{
	for (int i = 0; i < rowCount() - 1; i++) {
		if (name == items[i].name)
			return i;
	}
	return -1;
}

int FilterModelBase::findInsertionIndex(const QString &name)
{
	// Find insertion position. Note: we search only up to the last
	// item, because the last item is the "Show Empty Tags" item.
	// N.B: We might do a binary search using std::lower_bound()
	int i;
	for (i = 0; i < rowCount() - 1; i++) {
		if (name < items[i].name)
			return i;
	}
	return i;
}

void FilterModelBase::addItem(const QString &name, bool checked, int count)
{
	int idx = findInsertionIndex(name);
	beginInsertRows(QModelIndex(), idx, idx);
	items.insert(items.begin() + idx, { name, checked, count });
	endInsertRows();
}

void FilterModelBase::changeName(const QString &oldName, const QString &newName)
{
	if (oldName.isEmpty() || newName.isEmpty() || oldName == newName)
		return;
	int oldIndex = indexOf(oldName);
	if (oldIndex < 0)
		return;
	int newIndex = indexOf(newName);

	if (newIndex >= 0) {
		// If there was already an entry with the new name, we are merging entries.
		// Thus, if the old entry was selected, also select the new entry.
		if (items[oldIndex].checked && !items[newIndex].checked) {
			items[newIndex].checked = true;
			dataChanged(createIndex(newIndex, 0), createIndex(newIndex, 0));
		}
		// Now, delete the old item
		beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
		items.erase(items.begin() + oldIndex);
		endRemoveRows();
	} else {
		// There was no entry of the same name. We might have to move the item.
		newIndex = findInsertionIndex(newName);
		if (oldIndex != newIndex && oldIndex + 1 != newIndex) {
			beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), newIndex);
			moveInVector(items, oldIndex, oldIndex + 1, newIndex);
			endMoveRows();
		}

		// The item was moved, but the name still has to be modified
		items[newIndex].name = newName;
		dataChanged(createIndex(newIndex, 0), createIndex(newIndex, 0));
	}
}

// Update the the items array.
// The last item is supposed to be the "Show Empty Tags" entry.
// All other items will be sorted alphabetically. Attention: the passed-in list is modified!
void FilterModelBase::updateList(QStringList &newList)
{
	// Sort list, but leave out last element by using std::prev()
	if (!newList.empty())
		std::sort(newList.begin(), std::prev(newList.end()));

	beginResetModel();

	// Keep copy of the old items array to reimport the checked state later.
	// Note that by using std::move(), this is an essentially free operation:
	// The data is moved from the old array to the new one and the old array
	// is reset to zero size.
	std::vector<Item> oldItems = std::move(items);

	// Resize the cleared array to the new size. This leaves the checked
	// flag in an undefined state (since we didn't define a default constructor).
	items.resize(newList.count());

	// First, reset all checked states to false and set the names
	anyChecked = false;
	for (int i = 0; i < rowCount(); ++i) {
		items[i].name = newList[i];
		items[i].checked = false;
	}

	// Then, restore the checked state.  Ignore the last item, since
	// this is the "Show Empty Tags" entry.
	for (int i = 0; i < (int)oldItems.size() - 1; i++) {
		if (oldItems[i].checked) {
			int ind = newList.indexOf(oldItems[i].name);
			if (ind >= 0 && ind < newList.count() - 1) {
				items[ind].checked = true;
				anyChecked = true;
			}
		}
	}

	// Reset the state of the "Show Empty Tags" entry. But be careful:
	// on program startup, the old list is empty.
	if (!oldItems.empty() && !items.empty() && oldItems.back().checked) {
		items.back().checked = true;
		anyChecked = true;
	}

	// Finally, calculate and cache the counts. Ignore the last item, since
	// this is the "Show Empty Tags" entry.
	for (int i = 0; i < (int)newList.size() - 1; i++)
		items[i].count = countDives(qPrintable(newList[i]));

	// Calculate count of "Empty Tags".
	if (!items.empty())
		items.back().count = countDives("");

	endResetModel();
}

// Decrease count of entry with given name. Remove if count reaches zero.
// Exception: Don't remove the "Show Empty Tags" entry.
void FilterModelBase::decreaseCount(const QString &name)
{
	if (name.isEmpty()) {
		// Decrease the "Show Empty Tags" entry. Keep it even if count reaches 0.
		if (items.empty() || items.back().count <= 0)
			return; // Shouldn't happen!
		--items.back().count;
		int idx = items.size() - 1;
		dataChanged(createIndex(idx, 0), createIndex(idx, 0));
		return;
	}

	int idx = indexOf(name);
	if (idx < 0 || items[idx].count <= 0)
		return; // Shouldn't happen

	if(--items[idx].count == 0) {
		beginRemoveRows(QModelIndex(), idx, idx);
		items.erase(items.begin() + idx);
		endRemoveRows();
	} else {
		dataChanged(createIndex(idx, 0), createIndex(idx, 0));
	}
}

// Increase count of entry with given name. If entry doesn't yet exist, add it.
void FilterModelBase::increaseCount(const QString &name)
{
	if (name.isEmpty()) {
		// Increase the "Show Empty Tags" entry. Keep it even if count reaches 0.
		if (items.empty())
			return; // Shouldn't happen!
		++items.back().count;
		int idx = items.size() - 1;
		dataChanged(createIndex(idx, 0), createIndex(idx, 0));
		return;
	}

	int idx = indexOf(name);
	if (idx < 0) {
		idx = findInsertionIndex(name);
		beginInsertRows(QModelIndex(), idx, idx);
		items.insert(items.begin() + idx, { name, anyChecked, 1 });
		endInsertRows();
	} else {
		++items[idx].count;
		dataChanged(createIndex(idx, 0), createIndex(idx, 0));
	}
}

Qt::ItemFlags FilterModelBase::flags(const QModelIndex &index) const
{
	return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
}

bool FilterModelBase::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::CheckStateRole) {
		items[index.row()].checked = value.toBool();
		anyChecked = false;
		for (const Item &item: items) {
			if (item.checked) {
				anyChecked = true;
				break;
			}
		}
		dataChanged(index, index, { role });
		return true;
	}
	return false;
}

int FilterModelBase::rowCount(const QModelIndex &) const
{
	return items.size();
}

QVariant FilterModelBase::data(const QModelIndex &index, int role) const
{
	if (role == Qt::CheckStateRole) {
		return items[index.row()].checked ? Qt::Checked : Qt::Unchecked;
	} else if (role == Qt::DisplayRole) {
		const Item &item = items[index.row()];
		return QStringLiteral("%1 (%2)").arg(item.name, QString::number(item.count));
	}
	return QVariant();
}

void FilterModelBase::clearFilter()
{
	for (Item &item: items)
		item.checked = false;
	anyChecked = false;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

void FilterModelBase::selectAll()
{
	for (Item &item: items)
		item.checked = true;
	anyChecked = true;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

void FilterModelBase::invertSelection()
{
	for (Item &item: items)
		item.checked = !item.checked;
	anyChecked = std::any_of(items.begin(), items.end(), [](Item &item) { return !!item.checked; });
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

void FilterModelBase::setNegate(bool negateParam)
{
	negate = negateParam;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0));
}

SuitsFilterModel::SuitsFilterModel(QObject *parent) : FilterModelBase(parent)
{
}

int SuitsFilterModel::countDives(const char *s) const
{
	return count_dives_with_suit(s);
}

bool SuitsFilterModel::doFilter(const dive *d) const
{
	// rowCount() == 0 should never happen, because we have the "no suits" row
	// let's handle it gracefully anyway.
	if (!anyChecked || rowCount() == 0)
		return true;

	// Checked means 'Show', Unchecked means 'Hide'.
	QString suit(d->suit);
	// only show empty suit dives if the user checked that.
	if (suit.isEmpty())
		return items[rowCount() - 1].checked != negate;

	// there is a suit selected
	// Ignore last item, since this is the "Show Empty Tags" entry
	for (int i = 0; i < rowCount() - 1; i++) {
		if (items[i].checked && suit == items[i].name)
			return !negate;
	}
	return negate;
}

void SuitsFilterModel::diveAdded(const dive *d)
{
	increaseCount(QString(d->suit));
}

void SuitsFilterModel::diveDeleted(const dive *d)
{
	decreaseCount(QString(d->suit));
}

void SuitsFilterModel::repopulate()
{
	QStringList list;
	struct dive *dive;
	int i = 0;
	for_each_dive (i, dive) {
		QString suit(dive->suit);
		if (!suit.isEmpty() && !list.contains(suit)) {
			list.append(suit);
		}
	}
	list << tr("No suit set");
	updateList(list);
}

TagFilterModel::TagFilterModel(QObject *parent) : FilterModelBase(parent)
{
}

int TagFilterModel::countDives(const char *s) const
{
	return count_dives_with_tag(s);
}

void TagFilterModel::repopulate()
{
	if (g_tag_list == NULL)
		return;
	QStringList list;
	struct tag_entry *current_tag_entry = g_tag_list;
	while (current_tag_entry != NULL) {
		if (count_dives_with_tag(current_tag_entry->tag->name) > 0)
			list.append(QString(current_tag_entry->tag->name));
		current_tag_entry = current_tag_entry->next;
	}
	list << tr("Empty tags");
	updateList(list);
}

bool TagFilterModel::doFilter(const dive *d) const
{
	// If there's nothing checked, this should show everything
	// rowCount() == 0 should never happen, because we have the "no tags" row
	// let's handle it gracefully anyway.
	if (!anyChecked || rowCount() == 0)
		return true;

	// Checked means 'Show', Unchecked means 'Hide'.
	struct tag_entry *head = d->tag_list;

	if (!head) // last tag means "Show empty tags";
		return items[rowCount() - 1].checked != negate;

	// have at least one tag.
	while (head) {
		QString tagName(head->tag->name);
		int index = indexOf(tagName);
		if (index >= 0 && items[index].checked)
			return !negate;
		head = head->next;
	}
	return negate;
}

void TagFilterModel::diveAdded(const dive *d)
{
	struct tag_entry *head = d->tag_list;
	if (!head) {
		increaseCount(QString());
		return;
	}
	while (head) {
		increaseCount(QString());
		increaseCount(QString(head->tag->name));
		head = head->next;
	}
}

void TagFilterModel::diveDeleted(const dive *d)
{
	struct tag_entry *head = d->tag_list;
	if (!head) {
		decreaseCount(QString());
		return;
	}
	while (head) {
		decreaseCount(QString(head->tag->name));
		head = head->next;
	}
}

BuddyFilterModel::BuddyFilterModel(QObject *parent) : FilterModelBase(parent)
{
}

int BuddyFilterModel::countDives(const char *s) const
{
	return count_dives_with_person(s);
}

static QStringList getDiveBuddies(const dive *d)
{
	QString persons = QString(d->buddy) + "," + QString(d->divemaster);
	QStringList personsList = persons.split(',', QString::SkipEmptyParts);
	for (QString &s: personsList)
		s = s.trimmed();
	return personsList;
}

bool BuddyFilterModel::doFilter(const dive *d) const
{
	// If there's nothing checked, this should show everything
	// rowCount() == 0 should never happen, because we have the "no tags" row
	// let's handle it gracefully anyway.
	if (!anyChecked || rowCount() == 0)
		return true;

	QStringList personsList = getDiveBuddies(d);
	// only show empty buddie dives if the user checked that.
	if (personsList.isEmpty())
		return items[rowCount() - 1].checked != negate;

	// have at least one buddy
	// Ignore last item, since this is the "Show Empty Tags" entry
	for (int i = 0; i < rowCount() - 1; i++) {
		if (items[i].checked && personsList.contains(items[i].name, Qt::CaseInsensitive))
			return !negate;
	}
	return negate;
}

void BuddyFilterModel::diveAdded(const dive *d)
{
	QStringList buddies = getDiveBuddies(d);
	if (buddies.empty()) {
		increaseCount(QString());
		return;
	}
	for(const QString &buddy: buddies)
		increaseCount(buddy);
}

void BuddyFilterModel::diveDeleted(const dive *d)
{
	QStringList buddies = getDiveBuddies(d);
	if (buddies.empty()) {
		decreaseCount(QString());
		return;
	}
	for(const QString &buddy: buddies)
		decreaseCount(buddy);
}

void BuddyFilterModel::repopulate()
{
	QStringList list;
	struct dive *dive;
	int i = 0;
	for_each_dive (i, dive) {
		QString persons = QString(dive->buddy) + "," + QString(dive->divemaster);
		Q_FOREACH (const QString &person, persons.split(',', QString::SkipEmptyParts)) {
			// Remove any leading spaces
			if (!list.contains(person.trimmed())) {
				list.append(person.trimmed());
			}
		}
	}
	list << tr("No buddies");
	updateList(list);
}

LocationFilterModel::LocationFilterModel(QObject *parent) : FilterModelBase(parent)
{
}

int LocationFilterModel::countDives(const char *s) const
{
	return count_dives_with_location(s);
}

bool LocationFilterModel::doFilter(const dive *d) const
{
	// rowCount() == 0 should never happen, because we have the "no location" row
	// let's handle it gracefully anyway.
	if (!anyChecked || rowCount() == 0)
		return true;

	// Checked means 'Show', Unchecked means 'Hide'.
	QString location(get_dive_location(d));
	// only show empty location dives if the user checked that.
	if (location.isEmpty())
		return items[rowCount() - 1].checked != negate;

	// There is a location selected
	// Ignore last item, since this is the "Show Empty Tags" entry
	for (int i = 0; i < rowCount() - 1; i++) {
		if (items[i].checked && location == items[i].name)
			return !negate;
	}
	return negate;
}

void LocationFilterModel::diveAdded(const dive *d)
{
	increaseCount(get_dive_location(d));
}

void LocationFilterModel::diveDeleted(const dive *d)
{
	decreaseCount(get_dive_location(d));
}

void LocationFilterModel::repopulate()
{
	QStringList list;
	struct dive *dive;
	int i = 0;
	for_each_dive (i, dive) {
		QString location(get_dive_location(dive));
		if (!location.isEmpty() && !list.contains(location)) {
			list.append(location);
		}
	}
	list << tr("No location set");
	updateList(list);
}

void LocationFilterModel::addName(const QString &newName)
{
	if (newName.isEmpty() || indexOf(newName) >= 0)
		return;
	int count = countDives(qPrintable(newName));
	// If any old item was checked, also check the new one so that
	// dives with the added dive site are shown.
	addItem(newName, anyChecked, count);
}

MultiFilterSortModel::MultiFilterSortModel(QObject *parent) : QSortFilterProxyModel(parent),
	divesDisplayed(0),
	curr_dive_site(NULL)
{
}

void MultiFilterSortModel::divesAdded(const QVector<dive *> &dives)
{
	// TODO: We call diveAdded for every dive and model.
	// If multiple dives are added (e.g. import dive) this will lead to a large
	// number of model changes and might be a pessimization compared to a full
	// model reload. Instead, the models should take the vector, calculate the
	// new fields and add them at once.
	for (FilterModelBase *model: models) {
		for (const dive *d: dives)
			model->diveAdded(d);
	}
}

void MultiFilterSortModel::divesDeleted(const QVector<dive *> &dives)
{
	// TODO: See comment for divesDeleted
	for (FilterModelBase *model: models) {
		for (const dive *d: dives)
			model->diveDeleted(d);
	}
}

bool MultiFilterSortModel::showDive(const struct dive *d) const
{
	if (curr_dive_site) {
		dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
		if (!ds)
			return false;
		return same_string(ds->name, curr_dive_site->name) || ds->uuid == curr_dive_site->uuid;
	}

	if (models.isEmpty())
		return true;

	for (const FilterModelBase *model: models) {
		if (!model->doFilter(d))
			return false;
	}

	return true;
}

bool MultiFilterSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex index0 = sourceModel()->index(source_row, 0, source_parent);
	QVariant diveVariant = sourceModel()->data(index0, DiveTripModel::DIVE_ROLE);
	struct dive *d = (struct dive *)diveVariant.value<void *>();

	// For dives, simply check the hidden_by_filter flag
	if (d)
		return !d->hidden_by_filter;

	// Since this is not a dive, it must be a trip
	QVariant tripVariant = sourceModel()->data(index0, DiveTripModel::TRIP_ROLE);
	dive_trip *trip = (dive_trip *)tripVariant.value<void *>();

	if (!trip)
		return false; // Oops. Neither dive nor trip, something is seriously wrong.

	// Show the trip if any dive is visible
	for (d = trip->dives; d; d = d->next) {
		if (!d->hidden_by_filter)
			return true;
	}
	return false;
}

void MultiFilterSortModel::filterChanged(const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles)
{
	// Only redo the filter if a checkbox changed. If the count of an entry changed,
	// we do *not* want to recalculate the filters.
	if (roles.contains(Qt::CheckStateRole))
		myInvalidate();
}

void MultiFilterSortModel::myInvalidate()
{
#if !defined(SUBSURFACE_MOBILE)
	int i;
	struct dive *d;
	DiveListView *dlv = MainWindow::instance()->dive_list;

	divesDisplayed = 0;

	// Apply filter for each dive
	for_each_dive (i, d) {
		bool show = showDive(d);
		filter_dive(d, show);
		if (show)
			divesDisplayed++;
	}

	invalidateFilter();

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
		QList<int> curSelectedDives;
		for_each_dive (i, d) {
			if (d->selected)
				curSelectedDives.append(get_divenr(d));
		}
		dlv->selectDives(curSelectedDives);
	}

	emit filterFinished();

	if (curr_dive_site) {
		dlv->expandAll();
	}
#endif
}

void MultiFilterSortModel::addFilterModel(FilterModelBase *model)
{
	models.append(model);
	connect(model, &FilterModelBase::dataChanged, this, &MultiFilterSortModel::filterChanged);
}

void MultiFilterSortModel::removeFilterModel(FilterModelBase *model)
{
	models.removeAll(model);
	disconnect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(myInvalidate()));
}

void MultiFilterSortModel::clearFilter()
{
	Q_FOREACH (FilterModelBase *iface, models) {
		iface->clearFilter();
	}
	myInvalidate();
}

void MultiFilterSortModel::startFilterDiveSite(uint32_t uuid)
{
	curr_dive_site = get_dive_site_by_uuid(uuid);
	myInvalidate();
}

void MultiFilterSortModel::stopFilterDiveSite()
{
	curr_dive_site = NULL;
	myInvalidate();
}
