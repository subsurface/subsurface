// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divetripmodel.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/divelist.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include <QIcon>
#include <QDebug>

static int nitrox_sort_value(struct dive *dive)
{
	int o2, he, o2max;
	get_dive_gas(dive, &o2, &he, &o2max);
	return he * 1000 + o2;
}

static QVariant dive_table_alignment(int column)
{
	QVariant retVal;
	switch (column) {
	case DiveTripModel::DEPTH:
	case DiveTripModel::DURATION:
	case DiveTripModel::TEMPERATURE:
	case DiveTripModel::TOTALWEIGHT:
	case DiveTripModel::SAC:
	case DiveTripModel::OTU:
	case DiveTripModel::MAXCNS:
		// Right align numeric columns
		retVal = int(Qt::AlignRight | Qt::AlignVCenter);
		break;
	// NR needs to be left aligned becase its the indent marker for trips too
	case DiveTripModel::NR:
	case DiveTripModel::DATE:
	case DiveTripModel::RATING:
	case DiveTripModel::SUIT:
	case DiveTripModel::CYLINDER:
	case DiveTripModel::GAS:
	case DiveTripModel::TAGS:
	case DiveTripModel::PHOTOS:
	case DiveTripModel::COUNTRY:
	case DiveTripModel::BUDDIES:
	case DiveTripModel::LOCATION:
		retVal = int(Qt::AlignLeft | Qt::AlignVCenter);
		break;
	}
	return retVal;
}

QVariant TripItem::data(int column, int role) const
{
	QVariant ret;
	bool oneDayTrip=true;

	if (role == DiveTripModel::TRIP_ROLE)
		return QVariant::fromValue<void *>(trip);

	if (role == DiveTripModel::SORT_ROLE)
		return (qulonglong)trip->when;

	if (role == Qt::DisplayRole) {
		switch (column) {
		case DiveTripModel::NR:
			QString shownText;
			struct dive *d = trip->dives;
			int countShown = 0;
			while (d) {
				if (!d->hidden_by_filter)
					countShown++;
				oneDayTrip &= is_same_day (trip->when,  d->when);
				d = d->next;
			}
			if (countShown < trip->nrdives)
				shownText = tr("(%1 shown)").arg(countShown);
			if (!empty_string(trip->location))
				ret = QString(trip->location) + ", " + get_trip_date_string(trip->when, trip->nrdives, oneDayTrip) + " "+ shownText;
			else
				ret = get_trip_date_string(trip->when, trip->nrdives, oneDayTrip) + shownText;
			break;
		}
	}

	return ret;
}

static const QString icon_names[4] = {
	QStringLiteral(":zero"),
	QStringLiteral(":photo-in-icon"),
	QStringLiteral(":photo-out-icon"),
	QStringLiteral(":photo-in-out-icon")
};

QVariant DiveItem::data(int column, int role) const
{
	QVariant retVal;

	switch (role) {
	case Qt::TextAlignmentRole:
		retVal = dive_table_alignment(column);
		break;
	case DiveTripModel::SORT_ROLE:
		switch (column) {
		case NR:
			retVal = (qlonglong)d->when;
			break;
		case DATE:
			retVal = (qlonglong)d->when;
			break;
		case RATING:
			retVal = d->rating;
			break;
		case DEPTH:
			retVal = d->maxdepth.mm;
			break;
		case DURATION:
			retVal = d->duration.seconds;
			break;
		case TEMPERATURE:
			retVal = d->watertemp.mkelvin;
			break;
		case TOTALWEIGHT:
			retVal = total_weight(d);
			break;
		case SUIT:
			retVal = QString(d->suit);
			break;
		case CYLINDER:
			retVal = QString(d->cylinder[0].type.description);
			break;
		case GAS:
			retVal = nitrox_sort_value(d);
			break;
		case SAC:
			retVal = d->sac;
			break;
		case OTU:
			retVal = d->otu;
			break;
		case MAXCNS:
			retVal = d->maxcns;
			break;
		case TAGS:
			retVal = displayTags();
			break;
		case PHOTOS:
			retVal = countPhotos();
			break;
		case COUNTRY:
			retVal = QString(get_dive_country(d));
			break;
		case BUDDIES:
			retVal = QString(d->buddy);
			break;
		case LOCATION:
			retVal = QString(get_dive_location(d));
			break;
		}
		break;
	case Qt::DisplayRole:
		switch (column) {
		case NR:
			retVal = d->number;
			break;
		case DATE:
			retVal = displayDate();
			break;
		case DEPTH:
			retVal = prefs.units.show_units_table ? displayDepthWithUnit() : displayDepth();
			break;
		case DURATION:
			retVal = displayDuration();
			break;
		case TEMPERATURE:
			retVal = prefs.units.show_units_table ? retVal = displayTemperatureWithUnit() : displayTemperature();
			break;
		case TOTALWEIGHT:
			retVal = prefs.units.show_units_table ? retVal = displayWeightWithUnit() : displayWeight();
			break;
		case SUIT:
			retVal = QString(d->suit);
			break;
		case CYLINDER:
			retVal = QString(d->cylinder[0].type.description);
			break;
		case SAC:
			retVal = prefs.units.show_units_table ? retVal = displaySacWithUnit() : displaySac();
			break;
		case OTU:
			retVal = d->otu;
			break;
		case MAXCNS:
			if (prefs.units.show_units_table)
				retVal = QString("%1%").arg(d->maxcns);
			else
				retVal = d->maxcns;
			break;
		case TAGS:
			retVal = displayTags();
			break;
		case PHOTOS:
			break;
		case COUNTRY:
			retVal = QString(get_dive_country(d));
			break;
		case BUDDIES:
			retVal = QString(d->buddy);
			break;
		case LOCATION:
			retVal = QString(get_dive_location(d));
			break;
		case GAS:
			const char *gas_string = get_dive_gas_string(d);
			retVal = QString(gas_string);
			free((void*)gas_string);
			break;
		}
		break;
	case Qt::DecorationRole:
		switch (column) {
		//TODO: ADD A FLAG
		case COUNTRY:
			retVal = QVariant();
			break;
		case LOCATION:
			if (dive_has_gps_location(d)) {
				IconMetrics im = defaultIconMetrics();
				retVal = QIcon(":globe-icon").pixmap(im.sz_small, im.sz_small);
			}
			break;
		case PHOTOS:
			if (d->picture_list)
			{
				IconMetrics im = defaultIconMetrics();
				retVal = QIcon(icon_names[countPhotos()]).pixmap(im.sz_small, im.sz_small);
			}	 // If there are photos, show one of the three photo icons: fish= photos during dive;
			break;	 // sun=photos before/after dive; sun+fish=photos during dive as well as before/after
		}
		break;
	case Qt::ToolTipRole:
		switch (column) {
		case NR:
			retVal = tr("#");
			break;
		case DATE:
			retVal = tr("Date");
			break;
		case RATING:
			retVal = tr("Rating");
			break;
		case DEPTH:
			retVal = tr("Depth(%1)").arg((get_units()->length == units::METERS) ? tr("m") : tr("ft"));
			break;
		case DURATION:
			retVal = tr("Duration");
			break;
		case TEMPERATURE:
			retVal = tr("Temp.(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
			break;
		case TOTALWEIGHT:
			retVal = tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
			break;
		case SUIT:
			retVal = tr("Suit");
			break;
		case CYLINDER:
			retVal = tr("Cylinder");
			break;
		case GAS:
			retVal = tr("Gas");
			break;
		case SAC:
			const char *unit;
			get_volume_units(0, NULL, &unit);
			retVal = tr("SAC(%1)").arg(QString(unit).append(tr("/min")));
			break;
		case OTU:
			retVal = tr("OTU");
			break;
		case MAXCNS:
			retVal = tr("Max. CNS");
			break;
		case TAGS:
			retVal = tr("Tags");
			break;
		case PHOTOS:
			retVal = tr("Media before/during/after dive");
			break;
		case COUNTRY:
			retVal = tr("Country");
			break;
		case BUDDIES:
			retVal = tr("Buddy");
			break;
		case LOCATION:
			retVal = tr("Location");
			break;
		}
		break;
	case DiveTripModel::STAR_ROLE:
		retVal = d->rating;
		break;
	case DiveTripModel::DIVE_ROLE:
		retVal = QVariant::fromValue<void *>(d);
		break;
	case DiveTripModel::DIVE_IDX:
		retVal = get_divenr(d);
		break;
	case DiveTripModel::SELECTED_ROLE:
		retVal = d->selected;
		break;
	}
	return retVal;
}

bool DiveItem::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::EditRole)
		return false;
	if (index.column() != NR)
		return false;

	int v = value.toInt();
	if (v == 0)
		return false;

	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->number == v)
			return false;
	}
	d->number = value.toInt();
	mark_divelist_changed(true);
	return true;
}

QString DiveItem::displayDate() const
{
	return get_dive_date_string(d->when);
}

QString DiveItem::displayDepth() const
{
	return get_depth_string(d->maxdepth);
}

QString DiveItem::displayDepthWithUnit() const
{
	return get_depth_string(d->maxdepth, true);
}

int DiveItem::countPhotos() const
{	// Determine whether dive has pictures, and whether they were taken during or before/after dive.
	const int bufperiod = 120; // A 2-min buffer period. Photos within 2 min of dive are assumed as
	int diveTotaltime = dive_endtime(d) - d->when;	// taken during the dive, not before/after.
	int pic_offset, icon_index = 0;
	FOR_EACH_PICTURE (d) {			// Step through each of the pictures for this dive:
		pic_offset = picture->offset.seconds;
		if  ((pic_offset < -bufperiod) | (pic_offset > diveTotaltime+bufperiod)) {
			icon_index |= 0x02;	// If picture is before/after the dive
		}				//  then set the appropriate bit ...
		else {
			icon_index |= 0x01;	// else set the bit for picture during the dive
		}
	}
	return icon_index;	// return value: 0=no pictures; 1=pictures during dive;
}				// 2=pictures before/after; 3=pictures during as well as before/after

QString DiveItem::displayDuration() const
{
	if (prefs.units.show_units_table)
		return get_dive_duration_string(d->duration.seconds, tr("h"), tr("min"), "", ":", d->dc.divemode == FREEDIVE);
	else
		return get_dive_duration_string(d->duration.seconds, "", "", "", ":", d->dc.divemode == FREEDIVE);
}

QString DiveItem::displayTemperature() const
{
	if (!d->watertemp.mkelvin)
		return QString();
	return get_temperature_string(d->watertemp, false);
}

QString DiveItem::displayTemperatureWithUnit() const
{
	if (!d->watertemp.mkelvin)
		return QString();
	return get_temperature_string(d->watertemp, true);
}

QString DiveItem::displaySac() const
{
	if (!d->sac)
		return QString();
	return get_volume_string(d->sac, false);
}

QString DiveItem::displaySacWithUnit() const
{
	if (!d->sac)
		return QString();
	return get_volume_string(d->sac, true).append(tr("/min"));
}

QString DiveItem::displayWeight() const
{
	return weight_string(weight());
}

QString DiveItem::displayWeightWithUnit() const
{
	return weight_string(weight()) + ((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
}

QString DiveItem::displayTags() const
{
	return get_taglist_string(d->tag_list);
}

int DiveItem::weight() const
{
	weight_t tw = { total_weight(d) };
	return tw.grams;
}

DiveTripModel *DiveTripModel::instance()
{
	static DiveTripModel self;
	return &self;
}

DiveTripModel::DiveTripModel(QObject *parent) :
	QAbstractItemModel(parent),
	currentLayout(TREE)
{
	// Stay informed of changes to the divelist
	connect(&diveListNotifier, &DiveListNotifier::divesAdded, this, &DiveTripModel::divesAdded);
	connect(&diveListNotifier, &DiveListNotifier::divesDeleted, this, &DiveTripModel::divesDeleted);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &DiveTripModel::divesChanged);
	connect(&diveListNotifier, &DiveListNotifier::divesMovedBetweenTrips, this, &DiveTripModel::divesMovedBetweenTrips);
	connect(&diveListNotifier, &DiveListNotifier::divesTimeChanged, this, &DiveTripModel::divesTimeChanged);
	connect(&diveListNotifier, &DiveListNotifier::divesSelected, this, &DiveTripModel::divesSelected);
	connect(&diveListNotifier, &DiveListNotifier::divesDeselected, this, &DiveTripModel::divesDeselected);
	connect(&diveListNotifier, &DiveListNotifier::currentDiveChanged, this, &DiveTripModel::currentDiveChanged);
}

int DiveTripModel::columnCount(const QModelIndex&) const
{
	return COLUMNS;
}

int DiveTripModel::rowCount(const QModelIndex &parent) const
{
	// No parent means top level - return the number of top-level items
	if (!parent.isValid())
		return items.size();

	// If the parent has a parent, this is a dive -> no entries
	if (parent.parent().isValid())
		return 0;

	// If this is outside of our top-level list -> no entries
	int row = parent.row();
	if (row < 0 || row >= (int)items.size())
		return 0;

	// Only trips have items
	const Item &entry =  items[parent.row()];
	return entry.trip ? entry.dives.size() : 0;
}

static const quintptr noParent = ~(quintptr)0; // This is the "internalId" marker for top-level item

QModelIndex DiveTripModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	// In the "internalId", we store either ~0 no top-level items or the
	// index of the parent item. A top-level item has an invalid parent.
	return createIndex(row, column, parent.isValid() ? parent.row() : noParent);
}

QModelIndex DiveTripModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	// In the "internalId", we store either ~0 for top-level items
	// or the index of the parent item.
	quintptr id = index.internalId();
	if (id == noParent)
		return QModelIndex();

	// Parent must be top-level item
	return createIndex(id, 0, noParent);
}

Qt::ItemFlags DiveTripModel::flags(const QModelIndex &index) const
{
	dive *d = diveOrNull(index);
	Qt::ItemFlags base = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	// Only dives have editable fields and only the number is editable
	return d && index.column() == NR ? base | Qt::ItemIsEditable : base;
}

QVariant DiveTripModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if (orientation == Qt::Vertical)
		return ret;

	switch (role) {
	case Qt::TextAlignmentRole:
		ret = dive_table_alignment(section);
		break;
	case Qt::FontRole:
		ret = defaultModelFont();
		break;
	case Qt::DisplayRole:
		switch (section) {
		case NR:
			ret = tr("#");
			break;
		case DATE:
			ret = tr("Date");
			break;
		case RATING:
			ret = tr("Rating");
			break;
		case DEPTH:
			ret = tr("Depth");
			break;
		case DURATION:
			ret = tr("Duration");
			break;
		case TEMPERATURE:
			ret = tr("Temp.");
			break;
		case TOTALWEIGHT:
			ret = tr("Weight");
			break;
		case SUIT:
			ret = tr("Suit");
			break;
		case CYLINDER:
			ret = tr("Cylinder");
			break;
		case GAS:
			ret = tr("Gas");
			break;
		case SAC:
			ret = tr("SAC");
			break;
		case OTU:
			ret = tr("OTU");
			break;
		case MAXCNS:
			ret = tr("Max CNS");
			break;
		case TAGS:
			ret = tr("Tags");
			break;
		case PHOTOS:
			ret = tr("Media");
			break;
		case COUNTRY:
			ret = tr("Country");
			break;
		case BUDDIES:
			ret = tr("Buddy");
			break;
		case LOCATION:
			ret = tr("Location");
			break;
		}
		break;
	case Qt::ToolTipRole:
		switch (section) {
		case NR:
			ret = tr("#");
			break;
		case DATE:
			ret = tr("Date");
			break;
		case RATING:
			ret = tr("Rating");
			break;
		case DEPTH:
			ret = tr("Depth(%1)").arg((get_units()->length == units::METERS) ? tr("m") : tr("ft"));
			break;
		case DURATION:
			ret = tr("Duration");
			break;
		case TEMPERATURE:
			ret = tr("Temp.(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
			break;
		case TOTALWEIGHT:
			ret = tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
			break;
		case SUIT:
			ret = tr("Suit");
			break;
		case CYLINDER:
			ret = tr("Cylinder");
			break;
		case GAS:
			ret = tr("Gas");
			break;
		case SAC:
			const char *unit;
			get_volume_units(0, NULL, &unit);
			ret = tr("SAC(%1)").arg(QString(unit).append(tr("/min")));
			break;
		case OTU:
			ret = tr("OTU");
			break;
		case MAXCNS:
			ret = tr("Max CNS");
			break;
		case TAGS:
			ret = tr("Tags");
			break;
		case PHOTOS:
			ret = tr("Media before/during/after dive");
			break;
		case BUDDIES:
			ret = tr("Buddy");
			break;
		case LOCATION:
			ret = tr("Location");
			break;
		}
		break;
	}

	return ret;
}

DiveTripModel::Item::Item(dive_trip *t, const QVector<dive *> &divesIn) : trip(t),
	dives(divesIn.toStdVector())
{
}

DiveTripModel::Item::Item(dive_trip *t, dive *d) : trip(t),
	dives({ d })
{
}

DiveTripModel::Item::Item(dive *d) : trip(nullptr),
	dives({ d })
{
}

bool DiveTripModel::Item::isDive(const dive *d) const
{
	return !trip && dives.size() == 1 && dives[0] == d;
}

dive *DiveTripModel::Item::getDive() const
{
	return !trip && dives.size() == 1 ? dives[0] : nullptr;
}

timestamp_t DiveTripModel::Item::when() const
{
	return trip ? trip->when : dives[0]->when;
}

// Find a range of matching elements in a vector.
// Input parameters:
//	v: vector to be searched
//	first: first element to search
//	cond: a function that is fed elements and returns an integer:
//		- >0: matches
//		-  0: doesn't match
//		- <0: stop searching, no more elements will be found
//	      cond is called exactly once per element and from the beginning of the range.
// Returns a pair [first, last) with usual C++ semantics: last-first is the size of the found range.
// If no items were found, first and last are set to the size of the vector.
template <typename Vector, typename Predicate>
std::pair<int, int> findRangeIf(const Vector &v, int first, Predicate cond)
{
	int size = (int)v.size();
	for (int i = first; i < size; ++i) {
		int res = cond(v[i]);
		if (res > 0) {
			for (int j = i + 1; j < size; ++j) {
				if (cond(v[j]) <= 0)
					return { i, j };
			}
			return { i, size };
		} else if (res < 0) {
			break;
		}
	}
	return { size, size };
}

// Ideally, Qt's model/view functions are processed in batches of contiguous
// items. Therefore, this template is used to process actions on ranges of
// contiguous elements of a vector.
// Input paremeters:
//	- items: vector to process, wich must allow random access via [] and the size() function
//	- cond: a predicate that is tested for each element. contiguous ranges of elements which
//		test for true are collected. cond is fed an element and should return:
//		- >0: matches
//		-  0: doesn't match
//		- <0: stop searching, no more elements will be found
//	- action: action that is called with the vector, first and last element of the range.
template<typename Vector, typename Predicate, typename Action>
void processRanges(Vector &items, Predicate cond, Action action)
{
	// Note: the "i++" is correct: We know that the last element tested
	// negatively -> we can skip it. Thus we avoid checking any element
	// twice.
	for(int i = 0;; i++) {
		std::pair<int,int> range = findRangeIf(items, i, cond);
		if (range.first >= (int)items.size())
			break;
		int delta = action(items, range.first, range.second);
		i = range.second + delta;
	}
}

// processRangesZip() is a refined version of processRanges(), which operates on two vectors.
// The vectors are supposed to be sorted equivalently. That is, the first matching
// item will of the first vector will match to the first item of the second vector.
// It is supposed that all elements of the second vector will match to an element of
// the first vector.
// Input parameters:
//	- items1: vector to process, wich must allow random access via [] and the size() function
//	- items2: second vector to process. every item in items2 must match to an item in items1
//		  in ascending order.
//	- cond1: a predicate that is tested for each element of items1 with the next unmatched element
//		 of items2. returns a boolean
//	- action: action that is called with the vectors, first and last element of the first range
//		  and first element of the last range.
template<typename Vector1, typename Vector2, typename Predicate, typename Action>
void processRangesZip(Vector1 &items1, Vector2 &items2, Predicate cond, Action action)
{
	int actItem = 0;
	processRanges(items1,
		      [&](typename Vector1::const_reference &e) mutable -> int { // Condition. Marked mutable so that it can change actItem
			if (actItem >= items2.size())
				return -1; // No more items -> bail
			if (!cond(e, items2[actItem]))
				return 0;
			++actItem;
			return 1;
		      },
		      [&](Vector1 &v1, int from, int to) { // Action
		      	return action(v1, items2, from, to, actItem);
		      });
}

void DiveTripModel::setupModelData()
{
	int i = dive_table.nr;

	beginResetModel();

	items.clear();
	while (--i >= 0) {
		dive *d = get_dive(i);
		update_cylinder_related_info(d);
		dive_trip_t *trip = d->divetrip;

		// If this dive doesn't have a trip or we are in list-mode, add
		// as top-level item.
		if (!trip || currentLayout == LIST) {
			items.emplace_back(d);
			continue;
		}

		// Check if that trip is already known to us: search for the first item
		// where item->trip is equal to trip.
		auto it = std::find_if(items.begin(), items.end(), [trip](const Item &item)
				       { return item.trip == trip; });
		if (it == items.end()) {
			// We didn't find an entry for this trip -> add one
			items.emplace_back(trip, d);
		} else {
			// We found the trip -> simply add the dive
			it->dives.push_back(d);
		}
	}

	endResetModel();
}

DiveTripModel::Layout DiveTripModel::layout() const
{
	return currentLayout;
}

void DiveTripModel::setLayout(DiveTripModel::Layout layout)
{
	currentLayout = layout;
	setupModelData();
}

QPair<dive_trip *, dive *> DiveTripModel::tripOrDive(const QModelIndex &index) const
{
	if (!index.isValid())
		return { nullptr, nullptr };

	QModelIndex parent = index.parent();
	// An invalid parent means that we're at the top-level
	if (!parent.isValid()) {
		const Item &entry = items[index.row()];
		if (entry.trip)
			return { entry.trip, nullptr };		// A trip
		else
			return { nullptr, entry.dives[0] };	// A dive
	}

	// Otherwise, we're at a leaf -> thats a dive
	return { nullptr, items[parent.row()].dives[index.row()] };
}

dive *DiveTripModel::diveOrNull(const QModelIndex &index) const
{
	return tripOrDive(index).second;
}

QVariant DiveTripModel::data(const QModelIndex &index, int role) const
{
	// Set the font for all items alike
	if (role == Qt::FontRole)
		return defaultModelFont();

	auto entry = tripOrDive(index);
	if (entry.first)
		return TripItem(entry.first).data(index.column(), role);
	else if (entry.second)
		return DiveItem(entry.second).data(index.column(), role);
	else
		return QVariant();
}

bool DiveTripModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	dive *d = diveOrNull(index);
	return d ? DiveItem(d).setData(index, value, role) : false;
}

int DiveTripModel::findTripIdx(const dive_trip *trip) const
{
	for (int i = 0; i < (int)items.size(); ++i)
		if (items[i].trip == trip)
			return i;
	return -1;
}

int DiveTripModel::findDiveIdx(const dive *d) const
{
	for (int i = 0; i < (int)items.size(); ++i)
		if (items[i].isDive(d))
			return i;
	return -1;
}

int DiveTripModel::findDiveInTrip(int tripIdx, const dive *d) const
{
	const Item &item = items[tripIdx];
	for (int i = 0; i < (int)item.dives.size(); ++i)
		if (item.dives[i] == d)
			return i;
	return -1;
}

int DiveTripModel::findInsertionIndex(timestamp_t when) const
{
	for (int i = 0; i < (int)items.size(); ++i) {
		if (when > items[i].when())
			return i;
	}
	return items.size();
}

// Add items from vector "v2" to vector "v1" in batches of contiguous objects.
// The items are inserted at places according to a sort order determined by "comp".
// "v1" and "v2" are supposed to be ordered accordingly.
// TODO: We might use binary search with std::lower_bound(), but not sure if it's worth it.
// Input parameters:
//	- v1: destination vector
//	- v2: source vector
//	- comp: compare-function, which is fed elements from v2 and v1. returns true for "insert here".
//	- adder: performs the insertion. Perameters: v1, v2, insertion index, from, to range in v2.
template <typename Vector1, typename Vector2, typename Comparator, typename Inserter>
void addInBatches(Vector1 &v1, const Vector2 &v2, Comparator comp, Inserter insert)
{
	int idx = 0; // Index where dives will be inserted
	int i, j; // Begin and end of range to insert
	for (i = 0; i < (int)v2.size(); i = j) {
		for (; idx < (int)v1.size() && !comp(v2[i], v1[idx]); ++idx)
			; // Pass

		// We found the index of the first item to add.
		// Now search how many items we should insert there.
		if (idx == (int)v1.size()) {
			// We were at end -> insert the remaining items
			j = v2.size();
		} else {
			for (j = i + 1; j < (int)v2.size() && comp(v2[i], v1[idx]); ++j)
				; // Pass
		}

		// Now add the batch
		insert(v1, v2, idx, i, j);

		// Skip over inserted dives for searching the new insertion position plus one.
		// If we added at the end, the loop will end anyway.
		idx += j - i + 1;
	}
}

void DiveTripModel::addDivesToTrip(int trip, const QVector<dive *> &dives)
{
		// Construct the parent index, ie. the index of the trip.
		QModelIndex parent = createIndex(trip, 0, noParent);

		// Either this is outside of a trip or we're in list mode.
		// Thus, add dives at the top-level in batches
		addInBatches(items[trip].dives, dives,
			     [](dive *d, dive *d2) { return d->when >= d2->when; }, // comp
			     [&](std::vector<dive *> &items, const QVector<dive *> &dives, int idx, int from, int to) { // inserter
				beginInsertRows(parent, idx, idx + to - from - 1);
				items.insert(items.begin() + idx, dives.begin() + from, dives.begin() + to);
				endInsertRows();
			     });
}

void DiveTripModel::divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &divesIn)
{
	// The dives come from the backend sorted by start-time. But our model is sorted
	// reverse-chronologically. Therefore, invert the list.
	// TODO: Change sorting of the model to reflect core!
	QVector<dive *> dives = divesIn;
	std::reverse(dives.begin(), dives.end());

	if (!trip || currentLayout == LIST) {
		// Either this is outside of a trip or we're in list mode.
		// Thus, add dives at the top-level in batches
		addInBatches(items, dives,
			     [](dive *d, const Item &entry) { return d->when >= entry.when(); }, // comp
			     [&](std::vector<Item> &items, const QVector<dive *> &dives, int idx, int from, int to) { // inserter
				beginInsertRows(QModelIndex(), idx, idx + to - from - 1);
				items.insert(items.begin() + idx, dives.begin() + from, dives.begin() + to);
				endInsertRows();
			     });
	} else if (addTrip) {
		// We're supposed to add the whole trip. Just insert the trip.
		int idx = findInsertionIndex(trip->when); // Find the place where we have to insert the thing
		beginInsertRows(QModelIndex(), idx, idx);
		items.insert(items.begin() + idx, { trip, dives });
		endInsertRows();
	} else {
		// Ok, we have to add dives to an existing trip
		// Find the trip...
		int idx = findTripIdx(trip);
		if (idx < 0) {
			// We don't know the trip - this shouldn't happen. We seem to have
			// missed some signals!
			qWarning() << "DiveTripModel::divesAdded(): unknown trip";
			return;
		}

		// ...and add dives.
		addDivesToTrip(idx, dives);

		// We have to signal that the trip changed, so that the number of dives in th header is updated
		QModelIndex tripIndex = createIndex(idx, 0, noParent);
		dataChanged(tripIndex, tripIndex);
	}
}

void DiveTripModel::divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &divesIn)
{
	// TODO: dives comes sorted by ascending time, but the model is sorted by descending time.
	// Instead of being smart, simply reverse the input array.
	QVector<dive *> dives = divesIn;
	std::reverse(dives.begin(), dives.end());

	if (!trip || currentLayout == LIST) {
		// Either this is outside of a trip or we're in list mode.
		// Thus, delete top-level dives. We do this range-wise.
		processRangesZip(items, dives,
				 [](const Item &e, dive *d) { return e.getDive() == d; }, // Condition
				 [&](std::vector<Item> &items, const QVector<dive *> &, int from, int to, int) -> int { // Action
					beginRemoveRows(QModelIndex(), from, to - 1);
					items.erase(items.begin() + from, items.begin() + to);
					endRemoveRows();
					return from - to; // Delta: negate the number of items deleted
					 });
	} else {
		// Find the trip
		int idx = findTripIdx(trip);
		if (idx < 0) {
			// We don't know the trip - this shouldn't happen. We seem to have
			// missed some signals!
			qWarning() << "DiveTripModel::divesDeleted(): unknown trip";
			return;
		}

		if (deleteTrip) {
			// We're supposed to delete the whole trip. Nice, we don't have to
			// care about individual dives. Just remove the row.
			beginRemoveRows(QModelIndex(), idx, idx);
			items.erase(items.begin() + idx);
			endRemoveRows();
		} else {
			// Construct the parent index, ie. the index of the trip.
			QModelIndex parent = createIndex(idx, 0, noParent);

			// Delete a number of dives in a trip. We do this range-wise.
			processRangesZip(items[idx].dives, dives,
					 [](dive *d1, dive *d2) { return d1 == d2; }, // Condition
					 [&](std::vector<dive *> &diveList, const QVector<dive *> &, int from, int to, int) -> int { // Action
						beginRemoveRows(parent, from, to - 1);
						diveList.erase(diveList.begin() + from, diveList.begin() + to);
						endRemoveRows();
						return from - to; // Delta: negate the number of items deleted
					 });

			// We have to signal that the trip changed, so that the number of dives in th header is updated
			QModelIndex tripIndex = createIndex(idx, 0, noParent);
			dataChanged(tripIndex, tripIndex);
		}
	}
}

void DiveTripModel::divesChanged(dive_trip *trip, const QVector<dive *> &divesIn)
{
	// TODO: dives comes sorted by ascending time, but the model is sorted by descending time.
	// Instead of being smart, simply reverse the input array.
	QVector<dive *> dives = divesIn;
	std::reverse(dives.begin(), dives.end());

	if (!trip || currentLayout == LIST) {
		// Either this is outside of a trip or we're in list mode.
		// Thus, these are top-level dives. We do this range-wise.

		// Since we know that the dive list is sorted, we will only ever search for the first element
		// in dives as this must be the first that we encounter. Once we find a range, increase the
		// index accordingly.
		processRangesZip(items, dives,
				 [](const Item &e, dive *d) { return e.getDive() == d; }, // Condition
				 [&](const std::vector<Item> &, const QVector<dive *> &, int from, int to, int) -> int { // Action
					// TODO: We might be smarter about which columns changed!
					dataChanged(createIndex(from, 0, noParent), createIndex(to - 1, COLUMNS - 1, noParent));
					return 0; // No items added or deleted
				 });
	} else {
		// Find the trip.
		int idx = findTripIdx(trip);
		if (idx < 0) {
			// We don't know the trip - this shouldn't happen. We seem to have
			// missed some signals!
			qWarning() << "DiveTripModel::divesChanged(): unknown trip";
			return;
		}

		// Change the dives in the trip. We do this range-wise.
		processRangesZip(items[idx].dives, dives,
				 [](dive *d1, dive *d2) { return d1 == d2; }, // Condition
				 [&](const std::vector<dive *> &, const QVector<dive *> &, int from, int to, int) -> int { // Action
					// TODO: We might be smarter about which columns changed!
					dataChanged(createIndex(from, 0, idx), createIndex(to - 1, COLUMNS - 1, idx));
					return 0; // No items added or deleted
				 });
	}
}

void DiveTripModel::divesMovedBetweenTrips(dive_trip *from, dive_trip *to, bool deleteFrom, bool createTo, const QVector<dive *> &dives)
{
	// Move dives between trips. This is an "interesting" problem, as we might
	// move from trip to trip, from trip to top-level or from top-level to trip.
	// Moreover, we might have to add a trip first or delete an old trip.
	// For simplicity, we will simply used the already existing divesAdded() / divesDeleted()
	// functions. This *is* cheating. But let's just try this and see how graceful
	// this is handled by Qt and if it gives some ugly UI behavior!

	// But first let's just rule out the trivial cases: same-to-same trip move
	// and list view (in which case we don't care).
	if (from == to || currentLayout == LIST)
		return;

	// Cheating!
	divesAdded(to, createTo, dives);
	divesDeleted(from, deleteFrom, dives);
}

void DiveTripModel::divesTimeChanged(dive_trip *trip, timestamp_t delta, const QVector<dive *> &dives)
{
	// As in the case of divesMovedBetweenTrips(), this is a tricky, but solvable, problem.
	// We have to consider the direction (delta < 0 or delta >0) and that dives at their destination
	// position have different contiguous batches than at their original position. For now,
	// cheat and simply do a remove/add pair. Note that for this to work it is crucial the the
	// order of the dives don't change. This is indeed the case, as all starting-times where
	// moved by the same delta.

	// Cheating!
	divesDeleted(trip, false, dives);
	divesAdded(trip, false, dives);
}

void DiveTripModel::divesSelected(dive_trip *trip, const QVector<dive *> &dives)
{
	changeDiveSelection(trip, dives, true);
}

void DiveTripModel::divesDeselected(dive_trip *trip, const QVector<dive *> &dives)
{
	changeDiveSelection(trip, dives, false);
}

void DiveTripModel::changeDiveSelection(dive_trip *trip, const QVector<dive *> &divesIn, bool select)
{
	// TODO: dives comes sorted by ascending time, but the model is sorted by descending time.
	// Instead of being smart, simply reverse the input array.
	QVector<dive *> dives = divesIn;
	std::reverse(dives.begin(), dives.end());

	// We got a number of dives that have been selected. Turn this into QModelIndexes and
	// emit a signal, so that views can change the selection.
	QVector<QModelIndex> indexes;
	indexes.reserve(dives.count());

	if (!trip || currentLayout == LIST) {
		// Either this is outside of a trip or we're in list mode.
		// Since both lists are sorted, we can do this linearly. Perhaps a binary search
		// would be better?
		int j = 0; // Index in items array
		for (int i = 0; i < dives.size(); ++i) {
			while (j < (int)items.size() && !items[j].isDive(dives[i]))
				++j;
			if (j >= (int)items.size())
				break;
			indexes.append(createIndex(j, 0, noParent));
		}
	} else {
		// Find the trip.
		int idx = findTripIdx(trip);
		if (idx < 0) {
			// We don't know the trip - this shouldn't happen. We seem to have
			// missed some signals!
			qWarning() << "DiveTripModel::divesSelected(): unknown trip";
			return;
		}
		// Locate the indices inside the trip.
		// Since both lists are sorted, we can do this linearly. Perhaps a binary search
		// would be better?
		int j = 0; // Index in items array
		const Item &entry = items[idx];
		for (int i = 0; i < dives.size(); ++i) {
			while (j < (int)entry.dives.size() && entry.dives[j] != dives[i])
				++j;
			if (j >= (int)entry.dives.size())
				break;
			indexes.append(createIndex(j, 0, idx));
		}
	}

	emit selectionChanged(indexes, select);
}

void DiveTripModel::currentDiveChanged()
{
	// The current dive has changed. Transform the current dive into an index and pass it on to the view.
	if (!current_dive) {
		emit newCurrentDive(QModelIndex()); // No current dive -> tell view to clear current index with an invalid index
		return;
	}

	dive_trip *trip = current_dive->divetrip;
	if (!trip || currentLayout == LIST) {
		// Either this is outside of a trip or we're in list mode.
		int idx = findDiveIdx(current_dive);
		if (idx < 0) {
			// We don't know this dive. Something is wrong. Warn and bail.
			qWarning() << "DiveTripModel::currentDiveChanged(): unknown top-level dive";
			emit newCurrentDive(QModelIndex());
			return;
		}
		emit newCurrentDive(createIndex(idx, 0, noParent));
	} else {
		int idx = findTripIdx(trip);
		if (idx < 0) {
			// We don't know the trip - this shouldn't happen. Warn and bail.
			qWarning() << "DiveTripModel::currentDiveChanged(): unknown trip";
			emit newCurrentDive(QModelIndex());
			return;
		}
		int diveIdx = findDiveInTrip(idx, current_dive);
		if (diveIdx < 0) {
			// We don't know this dive. Something is wrong. Warn and bail.
			qWarning() << "DiveTripModel::currentDiveChanged(): unknown top-level dive";
			emit newCurrentDive(QModelIndex());
			return;
		}
		emit newCurrentDive(createIndex(diveIdx, 0, idx));
	}
}
