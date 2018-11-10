// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divetripmodel.h"
#include "qt-models/filtermodels.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/divelist.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include <QIcon>
#include <QDebug>

static int nitrox_sort_value(const struct dive *dive)
{
	int o2, he, o2max;
	get_dive_gas(dive, &o2, &he, &o2max);
	return he * 1000 + o2;
}

static QVariant dive_table_alignment(int column)
{
	switch (column) {
	case DiveTripModel::DEPTH:
	case DiveTripModel::DURATION:
	case DiveTripModel::TEMPERATURE:
	case DiveTripModel::TOTALWEIGHT:
	case DiveTripModel::SAC:
	case DiveTripModel::OTU:
	case DiveTripModel::MAXCNS:
		// Right align numeric columns
		return int(Qt::AlignRight | Qt::AlignVCenter);
	// NR needs to be left aligned because its the indent marker for trips too
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
		return int(Qt::AlignLeft | Qt::AlignVCenter);
	}
	return QVariant();
}

QVariant DiveTripModel::tripData(const dive_trip *trip, int column, int role)
{
	bool oneDayTrip=true;

	if (role == TRIP_ROLE)
		return QVariant::fromValue<void *>((void *)trip); // Not nice: casting away a const

	if (role == Qt::DisplayRole) {
		switch (column) {
		case DiveTripModel::NR:
			QString shownText;
			int countShown = 0;
			for (int i = 0; i < trip->dives.nr; ++i) {
				struct dive *d = trip->dives.dives[i];
				if (!d->hidden_by_filter)
					countShown++;
				oneDayTrip &= is_same_day (trip->when,  d->when);
			}
			if (countShown < trip->dives.nr)
				shownText = tr("(%1 shown)").arg(countShown);
			if (!empty_string(trip->location))
				return QString(trip->location) + ", " + get_trip_date_string(trip->when, trip->dives.nr, oneDayTrip) + " "+ shownText;
			else
				return get_trip_date_string(trip->when, trip->dives.nr, oneDayTrip) + shownText;
		}
	}

	return QVariant();
}

static const QString icon_names[4] = {
	QStringLiteral(":zero"),
	QStringLiteral(":photo-in-icon"),
	QStringLiteral(":photo-out-icon"),
	QStringLiteral(":photo-in-out-icon")
};

static int countPhotos(const struct dive *d)
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

static QString displayDuration(const struct dive *d)
{
	if (prefs.units.show_units_table)
		return get_dive_duration_string(d->duration.seconds, gettextFromC::tr("h"), gettextFromC::tr("min"), "", ":", d->dc.divemode == FREEDIVE);
	else
		return get_dive_duration_string(d->duration.seconds, "", "", "", ":", d->dc.divemode == FREEDIVE);
}

static QString displayTemperature(const struct dive *d, bool units)
{
	if (!d->watertemp.mkelvin)
		return QString();
	return get_temperature_string(d->watertemp, units);
}

static QString displaySac(const struct dive *d, bool units)
{
	if (!d->sac)
		return QString();
	QString s = get_volume_string(d->sac, units);
	return units ? s + gettextFromC::tr("/min") : s;
}

static QString displayWeight(const struct dive *d, bool units)
{
	QString s = weight_string(total_weight(d));
	if (!units)
		return s;
	else if (get_units()->weight == units::KG)
		return s + gettextFromC::tr("kg");
	else
		return s + gettextFromC::tr("lbs");
}

QVariant DiveTripModel::diveData(const struct dive *d, int column, int role)
{
	switch (role) {
	case Qt::TextAlignmentRole:
		return dive_table_alignment(column);
	case Qt::DisplayRole:
		switch (column) {
		case NR:
			return d->number;
		case DATE:
			return get_dive_date_string(d->when);
		case DEPTH:
			return get_depth_string(d->maxdepth, prefs.units.show_units_table);
		case DURATION:
			return displayDuration(d);
		case TEMPERATURE:
			return displayTemperature(d, prefs.units.show_units_table);
		case TOTALWEIGHT:
			return displayWeight(d, prefs.units.show_units_table);
		case SUIT:
			return QString(d->suit);
		case CYLINDER:
			return QString(d->cylinder[0].type.description);
		case SAC:
			return displaySac(d, prefs.units.show_units_table);
		case OTU:
			return d->otu;
		case MAXCNS:
			if (prefs.units.show_units_table)
				return QString("%1%").arg(d->maxcns);
			else
				return d->maxcns;
		case TAGS:
			return get_taglist_string(d->tag_list);
		case PHOTOS:
			break;
		case COUNTRY:
			return QString(get_dive_country(d));
		case BUDDIES:
			return QString(d->buddy);
		case LOCATION:
			return QString(get_dive_location(d));
		case GAS:
			char *gas_string = get_dive_gas_string(d);
			QString ret(gas_string);
			free(gas_string);
			return ret;
		}
		break;
	case Qt::DecorationRole:
		switch (column) {
		//TODO: ADD A FLAG
		case COUNTRY:
			return QVariant();
		case LOCATION:
			if (dive_has_gps_location(d)) {
				IconMetrics im = defaultIconMetrics();
				return QIcon(":globe-icon").pixmap(im.sz_small, im.sz_small);
			}
			break;
		case PHOTOS:
			if (d->picture_list)
			{
				IconMetrics im = defaultIconMetrics();
				return QIcon(icon_names[countPhotos(d)]).pixmap(im.sz_small, im.sz_small);
			}	 // If there are photos, show one of the three photo icons: fish= photos during dive;
			break;	 // sun=photos before/after dive; sun+fish=photos during dive as well as before/after
		}
		break;
	case Qt::ToolTipRole:
		switch (column) {
		case NR:
			return tr("#");
		case DATE:
			return tr("Date");
		case RATING:
			return tr("Rating");
		case DEPTH:
			return tr("Depth(%1)").arg((get_units()->length == units::METERS) ? tr("m") : tr("ft"));
		case DURATION:
			return tr("Duration");
		case TEMPERATURE:
			return tr("Temp.(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
		case TOTALWEIGHT:
			return tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
		case SUIT:
			return tr("Suit");
		case CYLINDER:
			return tr("Cylinder");
		case GAS:
			return tr("Gas");
		case SAC:
			const char *unit;
			get_volume_units(0, NULL, &unit);
			return tr("SAC(%1)").arg(QString(unit).append(tr("/min")));
		case OTU:
			return tr("OTU");
		case MAXCNS:
			return tr("Max. CNS");
		case TAGS:
			return tr("Tags");
		case PHOTOS:
			return tr("Media before/during/after dive");
		case COUNTRY:
			return tr("Country");
		case BUDDIES:
			return tr("Buddy");
		case LOCATION:
			return tr("Location");
		}
		break;
	case STAR_ROLE:
		return d->rating;
	case DIVE_ROLE:
		return QVariant::fromValue<void *>((void *)d);  // Not nice: casting away a const
	case DIVE_IDX:
		return get_divenr(d);
	case SELECTED_ROLE:
		return d->selected;
	}
	return QVariant();
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
	return entry.d_or_t.trip ? entry.dives.size() : 0;
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
	if (orientation == Qt::Vertical)
		return QVariant();

	switch (role) {
	case Qt::TextAlignmentRole:
		return dive_table_alignment(section);
	case Qt::FontRole:
		return defaultModelFont();
	case Qt::InitialSortOrderRole:
		// By default, sort NR and DATE descending, everything else ascending.
		return section == NR || section == DATE ? Qt::DescendingOrder : Qt::AscendingOrder;
	case Qt::DisplayRole:
		switch (section) {
		case NR:
			return tr("#");
		case DATE:
			return tr("Date");
		case RATING:
			return tr("Rating");
		case DEPTH:
			return tr("Depth");
		case DURATION:
			return tr("Duration");
		case TEMPERATURE:
			return tr("Temp.");
		case TOTALWEIGHT:
			return tr("Weight");
		case SUIT:
			return tr("Suit");
		case CYLINDER:
			return tr("Cylinder");
		case GAS:
			return tr("Gas");
		case SAC:
			return tr("SAC");
		case OTU:
			return tr("OTU");
		case MAXCNS:
			return tr("Max CNS");
		case TAGS:
			return tr("Tags");
		case PHOTOS:
			return tr("Media");
		case COUNTRY:
			return tr("Country");
		case BUDDIES:
			return tr("Buddy");
		case LOCATION:
			return tr("Location");
		}
		break;
	case Qt::ToolTipRole:
		switch (section) {
		case NR:
			return tr("#");
		case DATE:
			return tr("Date");
		case RATING:
			return tr("Rating");
		case DEPTH:
			return tr("Depth(%1)").arg((get_units()->length == units::METERS) ? tr("m") : tr("ft"));
		case DURATION:
			return tr("Duration");
		case TEMPERATURE:
			return tr("Temp.(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
		case TOTALWEIGHT:
			return tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
		case SUIT:
			return tr("Suit");
		case CYLINDER:
			return tr("Cylinder");
		case GAS:
			return tr("Gas");
		case SAC:
			const char *unit;
			get_volume_units(0, NULL, &unit);
			return tr("SAC(%1)").arg(QString(unit).append(tr("/min")));
		case OTU:
			return tr("OTU");
		case MAXCNS:
			return tr("Max CNS");
		case TAGS:
			return tr("Tags");
		case PHOTOS:
			return tr("Media before/during/after dive");
		case BUDDIES:
			return tr("Buddy");
		case LOCATION:
			return tr("Location");
		}
		break;
	}

	return QVariant();
}

DiveTripModel::Item::Item(dive_trip *t, const QVector<dive *> &divesIn) : d_or_t{nullptr, t},
	dives(divesIn.toStdVector())
{
}

DiveTripModel::Item::Item(dive_trip *t, dive *d) : d_or_t{nullptr, t},
	dives({ d })
{
}

DiveTripModel::Item::Item(dive *d) : d_or_t{d, nullptr}
{
}

bool DiveTripModel::Item::isDive(const dive *d) const
{
	return d_or_t.dive == d;
}

dive *DiveTripModel::Item::getDive() const
{
	return d_or_t.dive;
}

timestamp_t DiveTripModel::Item::when() const
{
	return d_or_t.trip ? d_or_t.trip->when : d_or_t.dive->when;
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
	beginResetModel();

	items.clear();
	for (int i = 0; i < dive_table.nr ; ++i) {
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
		// that corresponds to that trip
		auto it = std::find_if(items.begin(), items.end(), [trip](const Item &item)
				       { return item.d_or_t.trip == trip; });
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

void DiveTripModel::setLayout(DiveTripModel::Layout layout)
{
	currentLayout = layout;
	setupModelData();
}

dive_or_trip DiveTripModel::tripOrDive(const QModelIndex &index) const
{
	if (!index.isValid())
		return { nullptr, nullptr };

	QModelIndex parent = index.parent();
	// An invalid parent means that we're at the top-level
	if (!parent.isValid())
		return items[index.row()].d_or_t;

	// Otherwise, we're at a leaf -> thats a dive
	return { items[parent.row()].dives[index.row()], nullptr };
}

dive *DiveTripModel::diveOrNull(const QModelIndex &index) const
{
	return tripOrDive(index).dive;
}

QVariant DiveTripModel::data(const QModelIndex &index, int role) const
{
	// Set the font for all items alike
	if (role == Qt::FontRole)
		return defaultModelFont();

	dive_or_trip entry = tripOrDive(index);
	if (entry.trip)
		return tripData(entry.trip, index.column(), role);
	else if (entry.dive)
		return diveData(entry.dive, index.column(), role);
	else
		return QVariant();
}

bool DiveTripModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	// We only support setting of data for dives and there, only the number.
	dive *d = diveOrNull(index);
	if (!d)
		return false;
	if (role != Qt::EditRole)
		return false;
	if (index.column() != NR)
		return false;

	int v = value.toInt();
	if (v == 0)
		return false;

	// Only accept numbers that are not already in use by other dives.
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->number == v)
			return false;
	}
	d->number = v;
	mark_divelist_changed(true);
	return true;
}

int DiveTripModel::findTripIdx(const dive_trip *trip) const
{
	for (int i = 0; i < (int)items.size(); ++i)
		if (items[i].d_or_t.trip == trip)
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
		if (when <= items[i].when())
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
			for (j = i + 1; j < (int)v2.size() && comp(v2[j], v1[idx]); ++j)
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
		     [](dive *d, dive *d2) { return dive_less_than(d, d2); }, // comp
		     [&](std::vector<dive *> &items, const QVector<dive *> &dives, int idx, int from, int to) { // inserter
			beginInsertRows(parent, idx, idx + to - from - 1);
			items.insert(items.begin() + idx, dives.begin() + from, dives.begin() + to);
			endInsertRows();
		     });
}

// This function is used to compare a dive to an arbitrary entry (dive or trip).
// For comparing two dives, use the core function dive_less_than_entry, which
// effectively sorts by timestamp.
// If comparing to a trip, the policy for equal-times is to place the dives
// before the trip in the case of equal timestamps.
bool DiveTripModel::dive_before_entry(const dive *d, const Item &entry)
{
	dive_or_trip d_or_t { (dive *)d, nullptr };
	return dive_or_trip_less_than(d_or_t, entry.d_or_t);
}

void DiveTripModel::divesAdded(dive_trip *trip, bool addTrip, const QVector<dive *> &dives)
{
	if (!trip || currentLayout == LIST) {
		// Either this is outside of a trip or we're in list mode.
		// Thus, add dives at the top-level in batches
		addInBatches(items, dives,
			     &dive_before_entry, // comp
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

void DiveTripModel::divesDeleted(dive_trip *trip, bool deleteTrip, const QVector<dive *> &dives)
{
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

void DiveTripModel::divesChanged(dive_trip *trip, const QVector<dive *> &dives)
{
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

QVector<dive *> filterSelectedDives(const QVector<dive *> &dives)
{
	QVector<dive *> res;
	res.reserve(dives.size());
	for (dive *d: dives)
		if (d->selected)
			res.append(d);
	return res;
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
	// Unfortunately, removing the dives means that their selection is lost.
	// Thus, remember the selection and re-add it later.
	QVector<dive *> selectedDives = filterSelectedDives(dives);
	divesAdded(to, createTo, dives);
	divesDeleted(from, deleteFrom, dives);
	divesSelected(to, selectedDives);
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
	// Unfortunately, removing the dives means that their selection is lost.
	// Thus, remember the selection and re-add it later.
	QVector<dive *> selectedDives = filterSelectedDives(dives);
	divesDeleted(trip, false, dives);
	divesAdded(trip, false, dives);
	divesSelected(trip, selectedDives);
}

void DiveTripModel::divesSelected(dive_trip *trip, const QVector<dive *> &dives)
{
	changeDiveSelection(trip, dives, true);
}

void DiveTripModel::divesDeselected(dive_trip *trip, const QVector<dive *> &dives)
{
	changeDiveSelection(trip, dives, false);
}

void DiveTripModel::changeDiveSelection(dive_trip *trip, const QVector<dive *> &dives, bool select)
{
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

void DiveTripModel::filterFinished()
{
	// If the filter finished, update all trip items to show the correct number of displayed dives
	// in each trip. Without doing this, only trip headers of expanded trips were updated.
	if (currentLayout == LIST)
		return; // No trips in list mode
	for (int idx = 0; idx < (int)items.size(); ++idx) {
		QModelIndex tripIndex = createIndex(idx, 0, noParent);
		dataChanged(tripIndex, tripIndex);
	}
}

// Simple sorting helper for sorting against a criterium and if
// that is undefined against a different criterium.
// Return true if diff1 < 0, false if diff1 > 0.
// If diff1 == 0 return true if diff2 < 0;
static bool lessThanHelper(int diff1, int diff2)
{
	return diff1 < 0 || (diff1 == 0 && diff2 < 0);
}

static int strCmp(const char *s1, const char *s2)
{
	if (!s1)
		return !s2 ? 0 : -1;
	if (!s2)
		return 1;
	return QString::localeAwareCompare(QString(s1), QString(s2)); // TODO: avoid copy
}

bool DiveTripModel::lessThan(const QModelIndex &i1, const QModelIndex &i2) const
{
	if (currentLayout != LIST) {
		// In tree mode we don't support any sorting!
		// Simply keep the original position.
		return i1.row() < i2.row();
	}

	// We assume that i1.column() == i2.column().
	// We are in list mode, so we know that we only have dives.
	int row1 = i1.row();
	int row2 = i2.row();
	if (row1 < 0 || row1 >= (int)items.size() || row2 < 0 || row2 >= (int)items.size())
		return false;
	const dive *d1 = items[i1.row()].d_or_t.dive;
	const dive *d2 = items[i2.row()].d_or_t.dive;
	// This is used as a second sort criterion: For equal values, sorting is chronologically *descending*.
	int row_diff = row2 - row1;
	switch (i1.column()) {
	case NR:
	case DATE:
	default:
		return row1 < row2;
	case RATING:
		return lessThanHelper(d1->rating - d2->rating, row_diff);
	case DEPTH:
		return lessThanHelper(d1->maxdepth.mm - d2->maxdepth.mm, row_diff);
	case DURATION:
		return lessThanHelper(d1->duration.seconds - d2->duration.seconds, row_diff);
	case TEMPERATURE:
		return lessThanHelper(d1->watertemp.mkelvin - d2->watertemp.mkelvin, row_diff);
	case TOTALWEIGHT:
		return lessThanHelper(total_weight(d1) - total_weight(d2), row_diff);
	case SUIT:
		return lessThanHelper(strCmp(d1->suit, d2->suit), row_diff);
	case CYLINDER:
		return lessThanHelper(strCmp(d1->cylinder[0].type.description, d2->cylinder[0].type.description), row_diff);
	case GAS:
		return lessThanHelper(nitrox_sort_value(d1) - nitrox_sort_value(d2), row_diff);
	case SAC:
		return lessThanHelper(d1->sac - d2->sac, row_diff);
	case OTU:
		return lessThanHelper(d1->otu - d2->otu, row_diff);
	case MAXCNS:
		return lessThanHelper(d1->maxcns - d2->maxcns, row_diff);
	case TAGS: {
		char *s1 = taglist_get_tagstring(d1->tag_list);
		char *s2 = taglist_get_tagstring(d2->tag_list);
		int diff = strCmp(s1, s2);
		free(s1);
		free(s2);
		return lessThanHelper(diff, row_diff);
	}
	case PHOTOS:
		return lessThanHelper(countPhotos(d1) - countPhotos(d2), row_diff);
	case COUNTRY:
		return lessThanHelper(strCmp(get_dive_country(d1), get_dive_country(d2)), row_diff);
	case BUDDIES:
		return lessThanHelper(strCmp(d1->buddy, d2->buddy), row_diff);
	case LOCATION:
		return lessThanHelper(strCmp(get_dive_location(d1), get_dive_location(d2)), row_diff);
	}
}
