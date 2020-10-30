// SPDX-License-Identifier: GPL-2.0
#include "mobilelistmodel.h"
#include "core/divefilter.h" // for shown_dives

MobileListModelBase::MobileListModelBase(DiveTripModelBase *sourceIn) : source(sourceIn)
{
}

QHash<int, QByteArray> MobileListModelBase::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[DiveTripModelBase::IS_TRIP_ROLE] = "isTrip";
	roles[DiveTripModelBase::CURRENT_ROLE] = "current";
	roles[IsTopLevelRole] = "isTopLevel";
	roles[DiveDateRole] = "date";
	roles[TripIdRole] = "tripId";
	roles[TripNrDivesRole] = "tripNrDives";
	roles[TripShortDateRole] = "tripShortDate";
	roles[TripTitleRole] = "tripTitle";
	roles[DateTimeRole] = "dateTime";
	roles[IdRole] = "id";
	roles[NumberRole] = "number";
	roles[LocationRole] = "location";
	roles[DepthRole] = "depth";
	roles[DurationRole] = "duration";
	roles[DepthDurationRole] = "depthDuration";
	roles[RatingRole] = "rating";
	roles[VizRole] = "viz";
	roles[SuitRole] = "suit";
	roles[AirTempRole] = "airTemp";
	roles[WaterTempRole] = "waterTemp";
	roles[SacRole] = "sac";
	roles[SumWeightRole] = "sumWeight";
	roles[DiveMasterRole] = "diveMaster";
	roles[BuddyRole] = "buddy";
	roles[TagsRole] = "tags";
	roles[NotesRole]= "notes";
	roles[GpsRole] = "gps";
	roles[GpsDecimalRole] = "gpsDecimal";
	roles[NoDiveRole] = "noDive";
	roles[DiveSiteRole] = "diveSite";
	roles[CylinderRole] = "cylinder";
	roles[GetCylinderRole] = "getCylinder";
	roles[CylinderListRole] = "cylinderList";
	roles[SingleWeightRole] = "singleWeight";
	roles[StartPressureRole] = "startPressure";
	roles[EndPressureRole] = "endPressure";
	roles[FirstGasRole] = "firstGas";
	roles[SelectedRole] = "selected";
	roles[DiveInTripRole] = "diveInTrip";
	roles[TripAbove] = "tripAbove";
	roles[TripBelow] = "tripBelow";
	roles[TripLocationRole] = "tripLocation";
	roles[TripNotesRole] = "tripNotes";
	roles[IsInvalidRole] = "isInvalid";
	return roles;
}

int MobileListModel::shown() const
{
	return DiveFilter::instance()->shownDives();
}

int MobileListModelBase::columnCount(const QModelIndex &parent) const
{
	return source->columnCount(parent);
}

QModelIndex MobileListModelBase::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	return createIndex(row, column);
}

QModelIndex MobileListModelBase::parent(const QModelIndex &index) const
{
	// These are flat models - there is no parent
	return QModelIndex();
}

MobileListModel::MobileListModel(DiveTripModelBase *source) : MobileListModelBase(source),
	expandedRow(-1)
{
	connect(source, &DiveTripModelBase::modelAboutToBeReset, this, &MobileListModel::beginResetModel);
	connect(source, &DiveTripModelBase::modelReset, this, &MobileListModel::endResetModel);
	connect(source, &DiveTripModelBase::rowsAboutToBeRemoved, this, &MobileListModel::prepareRemove);
	connect(source, &DiveTripModelBase::rowsRemoved, this, &MobileListModel::doneRemove);
	connect(source, &DiveTripModelBase::rowsAboutToBeInserted, this, &MobileListModel::prepareInsert);
	connect(source, &DiveTripModelBase::rowsInserted, this, &MobileListModel::doneInsert);
	connect(source, &DiveTripModelBase::rowsAboutToBeMoved, this, &MobileListModel::prepareMove);
	connect(source, &DiveTripModelBase::rowsMoved, this, &MobileListModel::doneMove);
	connect(source, &DiveTripModelBase::dataChanged, this, &MobileListModel::changed);
	connect(&diveListNotifier, &DiveListNotifier::numShownChanged, this, &MobileListModel::shownChanged);
}

// We want to show the newest dives first. Therefore, we have to invert
// the indices with respect to the source model. To avoid mental gymnastics
// in the rest of the code, we do this right before sending to the
// source model and just after recieving from the source model, respectively.
QModelIndex MobileListModel::sourceIndex(int row, int col, int parentRow) const
{
	if (row < 0 || col < 0)
		return QModelIndex();
	QModelIndex parent;
	if (parentRow >= 0) {
		int numTop = source->rowCount(QModelIndex());
		parent = source->index(numTop - 1 - parentRow, 0);
	}
	int numItems = source->rowCount(parent);
	return source->index(numItems - 1 - row, col, parent);
}

int MobileListModel::numSubItems() const
{
	if (expandedRow < 0)
		return 0;
	return source->rowCount(sourceIndex(expandedRow, 0));
}

int MobileListModel::invertRow(const QModelIndex &parent, int row) const
{
	int numItems = source->rowCount(parent);
	return numItems - 1 - row;
}

int MobileListModel::mapRowFromSourceTopLevel(int row) const
{
	// This is a top-level item. If it is after the expanded row,
	// we have to add the items of the expanded row.
	row = invertRow(QModelIndex(), row);
	return expandedRow >= 0 && row > expandedRow ? row + numSubItems() : row;
}

int MobileListModel::mapRowFromSourceTopLevelForInsert(int row) const
{
	// This is a top-level item. If it is after the expanded row,
	// we have to add the items of the expanded row.
	row = invertRow(QModelIndex(), row) + 1;
	return expandedRow >= 0 && row > expandedRow ? row + numSubItems() : row;
}

// The parentRow parameter is the row of the expanded trip converted into
// local "coordinates" as a premature optimization.
int MobileListModel::mapRowFromSourceTrip(const QModelIndex &parent, int parentRow, int row) const
{
	row = invertRow(parent, row);
	if (parentRow != expandedRow) {
		qWarning("MobileListModel::mapRowFromSourceTrip() called on non-extended row");
		return -1;
	}
	return expandedRow + 1 + row; // expandedRow + 1 is the row of the first subitem
}

int MobileListModel::mapRowFromSource(const QModelIndex &parent, int row) const
{
	if (row < 0)
		return -1;

	if (!parent.isValid()) {
		return mapRowFromSourceTopLevel(row);
	} else {
		int parentRow = invertRow(QModelIndex(), parent.row());
		return mapRowFromSourceTrip(parent, parentRow, row);
	}
}

MobileListModel::IndexRange MobileListModel::mapRangeFromSource(const QModelIndex &parent, int first, int last) const
{
	int num = last - first; // Actually, that is num - 1 owing to Qt's bizarre range semantics!
	// Since we invert the direction, the last will be the first.
	if (!parent.isValid()) {
		first = mapRowFromSourceTopLevel(last);
		// If this includes the extended row, we have to add the subitems
		if (first <= expandedRow && first + num >= expandedRow)
			num += numSubItems();
		return { true, first, first + num };
	} else {
		int parentRow = invertRow(QModelIndex(), parent.row());
		if (parentRow == expandedRow) {
			first = mapRowFromSourceTrip(parent, parentRow, last);
			return { true, first, first + num };
		} else {
			return { false, -1, -1 };
		}
	}
}

// This is fun: when inserting, we point to the item *before* which we
// want to insert. But by inverting the direction we turn that into the item
// *after* which we want to insert. Thus, we have to add one to the range.
// Moreover, here we have to use the first item. TODO: We can remove this
// function once the core-model is sorted appropriately.
MobileListModel::IndexRange MobileListModel::mapRangeFromSourceForInsert(const QModelIndex &parent, int first, int last) const
{
	int num = last - first;
	if (!parent.isValid()) {
		first = mapRowFromSourceTopLevelForInsert(first);
		return { true, first, first + num };
	} else {
		int parentRow = invertRow(QModelIndex(), parent.row());
		if (parentRow == expandedRow) {
			first = mapRowFromSourceTrip(parent, parentRow, first);
			return { true, first + 1, first + 1 + num };
		} else {
			return { false, -1, -1 };
		}
	}
}

QModelIndex MobileListModel::mapFromSource(const QModelIndex &idx) const
{
	return createIndex(mapRowFromSource(idx.parent(), idx.row()), idx.column());
}

QModelIndex MobileListModel::mapToSource(const QModelIndex &idx) const
{
	if (!idx.isValid())
		return idx;
	int row = idx.row();
	int col = idx.column();
	if (expandedRow < 0 || row <= expandedRow)
		return sourceIndex(row, col);

	int numSub = numSubItems();
	if (row > expandedRow + numSub)
		return sourceIndex(row - numSub, col);

	return sourceIndex(row - expandedRow - 1, col, expandedRow);
}

int MobileListModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0; // There is no parent
	return source->rowCount() + numSubItems();
}

QVariant MobileListModel::data(const QModelIndex &index, int role) const
{
	if (role == IsTopLevelRole)
		return index.row() <= expandedRow || index.row() > expandedRow + numSubItems();

	return source->data(mapToSource(index), role);
}

// Trivial helper to return and erase the last element of a stack
template<typename T>
static T pop(std::vector<T> &v)
{
	T res = v.back();
	v.pop_back();
	return res;
}

void MobileListModel::prepareRemove(const QModelIndex &parent, int first, int last)
{
	IndexRange range = mapRangeFromSource(parent, first, last);

	// Check whether we remove a dive from an expanded trip
	if (range.visible && parent.isValid()) {
		for (int i = first; i <= last; ++i) {
			QModelIndex index = source->index(i, 0, parent);
			if (source->data(index, DiveTripModelBase::CURRENT_ROLE).value<bool>()) {
				// Hack alert: we remove the currently selected dive from a visible trip.
				// Therefore, simply collapse the expanded trip. This is done in prepareRemove(),
				// i.e. before the base model has actually done any removal and thus things should
				// be consistent. Then, we can simply pretend that the range was invisible all along,
				// i.e. the removal is a no-op.
				unexpand();
				range.visible = false;
				break;
			}
		}
	}

	rangeStack.push_back(range);
	if (range.visible)
		beginRemoveRows(QModelIndex(), range.first, range.last);
}

void MobileListModel::updateRowAfterRemove(const IndexRange &range, int &row)
{
	if (row < 0)
		return;
	else if (range.first <= row && range.last >= row)
		row = -1;
	else if (range.first <= row)
		row -= range.last - range.first + 1;
}

void MobileListModel::doneRemove(const QModelIndex &parent, int first, int last)
{
	IndexRange range = pop(rangeStack);
	if (range.visible) {
		// Check if we have to move or remove the expanded or current item
		updateRowAfterRemove(range, expandedRow);

		endRemoveRows();
	}
}

void MobileListModel::prepareInsert(const QModelIndex &parent, int first, int last)
{
	IndexRange range = mapRangeFromSourceForInsert(parent, first, last);
	rangeStack.push_back(range);
	if (range.visible)
		beginInsertRows(QModelIndex(), range.first, range.last);
}

void MobileListModel::doneInsert(const QModelIndex &parent, int first, int last)
{
	IndexRange range = pop(rangeStack);
	if (range.visible) {
		// Check if we have to move the expanded item
		if (!parent.isValid() && expandedRow >= 0 && range.first <= expandedRow)
			expandedRow += last - first + 1;
		endInsertRows();
	} else {
		// The range was not visible, thus we inserted into a non-expanded trip.
		// However, we might have inserted the current item. This means that we
		// have to expand that trip.
		// If we inserted a dive that is the current item
		QModelIndex index = source->index(parent.row(), 0, QModelIndex());
		if (source->data(index, DiveTripModelBase::TRIP_HAS_CURRENT_ROLE).value<bool>()) {
			int row = mapRowFromSourceTopLevel(parent.row());
			expand(row);
		}
	}

	if (!parent.isValid()) {
		// If we inserted a trip that contains the current item, expand that trip
		for (int i = first; i <= last; ++i) {
			// Accessing data via the model/view API is annoying.
			// Perhaps we should simply add a tripHasCurrent(int row) function?
			QModelIndex index = source->index(i, 0, QModelIndex());
			if (source->data(index, DiveTripModelBase::TRIP_HAS_CURRENT_ROLE).value<bool>()) {
				int row = mapRowFromSourceTopLevel(i);
				expand(row);
				break;
			}
		}
	}
}

// Moving rows is annoying, as there are numerous cases to be considered.
// Some of them degrade to removing or inserting rows.
void MobileListModel::prepareMove(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int destRow)
{
	IndexRange range = mapRangeFromSource(parent, first, last);
	IndexRange rangeDest = mapRangeFromSourceForInsert(dest, destRow, destRow);
	rangeStack.push_back(range);
	rangeStack.push_back(rangeDest);
	if (!range.visible && !rangeDest.visible)
		return;
	if (range.visible && !rangeDest.visible)
		return prepareRemove(parent, first, last);
	if (!range.visible && rangeDest.visible)
		return prepareInsert(parent, first, last);
	beginMoveRows(QModelIndex(), range.first, range.last, QModelIndex(), rangeDest.first);
}

void MobileListModel::updateRowAfterMove(const IndexRange &range, const IndexRange &rangeDest, int &row)
{
	if (row >= 0 && (rangeDest.first < range.first || rangeDest.first > range.last + 1)) {
		if (range.first <= row && range.last >= row) {
			// Case 1: the expanded row is in the moved range
			if (rangeDest.first <= range.first)
				row -=  range.first - rangeDest.first;
			else if (rangeDest.first > range.last + 1)
				row +=  rangeDest.first - (range.last + 1);
		} else if (range.first > row && rangeDest.first <= row) {
			// Case 2: moving things from behind to before the expanded row
			row += range.last - range.first + 1;
		} else if (range.first < row && rangeDest.first > row)  {
			// Case 3: moving things from before to behind the expanded row
			row -= range.last - range.first + 1;
		}
	}
}

void MobileListModel::doneMove(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int destRow)
{
	IndexRange rangeDest = pop(rangeStack);
	IndexRange range = pop(rangeStack);
	if (!range.visible && !rangeDest.visible)
		return;
	if (range.visible && !rangeDest.visible)
		return doneRemove(parent, first, last);
	if (!range.visible && rangeDest.visible)
		return doneInsert(parent, first, last);
	if (expandedRow >= 0 && (rangeDest.first < range.first || rangeDest.first > range.last + 1)) {
		if (!parent.isValid() && range.first <= expandedRow && range.last >= expandedRow) {
			// Case 1: the expanded row is in the moved range
			// Since we don't support sub-trips, this means that we can't move into another trip
			if (dest.isValid())
				qWarning("MobileListModel::doneMove(): moving trips into a subtrip");
			else if (rangeDest.first <= range.first)
				expandedRow -=  range.first - rangeDest.first;
			else if (rangeDest.first > range.last + 1)
				expandedRow +=  rangeDest.first - (range.last + 1);
		} else if (range.first > expandedRow && rangeDest.first <= expandedRow) {
			// Case 2: moving things from behind to before the expanded row
			expandedRow += range.last - range.first + 1;
		} else if (range.first < expandedRow && rangeDest.first > expandedRow)  {
			// Case 3: moving things from before to behind the expanded row
			expandedRow -= range.last - range.first + 1;
		}
	}
	updateRowAfterMove(range, rangeDest, expandedRow);
	endMoveRows();
}

void MobileListModel::expand(int row)
{
	// First, let us treat the trivial cases: expand an invalid row
	// or the row is already expanded.
	if (row < 0) {
		unexpand();
		return;
	}
	if (row == expandedRow)
		return;

	// Collapse the old expanded row, if any.
	if (expandedRow >= 0) {
		int numSub = numSubItems();
		if (row > expandedRow) {
			if (row <= expandedRow + numSub) {
				qWarning("MobileListModel::expand(): trying to expand row in trip");
				return;
			}
			row -= numSub;
		}
		unexpand();
	}

	int first = row + 1;
	QModelIndex tripIdx = sourceIndex(row, 0);
	int numRow = source->rowCount(tripIdx);
	int last = first + numRow - 1;
	if (last < first) {
		// Amazingly, Qt's model API doesn't properly handle empty ranges!
		expandedRow = row;
		return;
	}
	beginInsertRows(QModelIndex(), first, last);
	expandedRow = row;
	endInsertRows();
}

void MobileListModel::changed(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
	// We don't support changes beyond levels, sorry.
	if (topLeft.parent().isValid() != bottomRight.parent().isValid()) {
		qWarning("MobileListModel::changed(): changes across different levels. Ignoring.");
		return;
	}

	// Special case CURRENT_ROLE: if a dive in a collapsed trip becomes current, expand that trip
	// and if a dive outside of a trip becomes current, collapse any expanded trip.
	// Note: changes to current must not be combined with other changes, therefore we can
	// assume that roles.size() == 1.
	if (roles.size() == 1 && roles[0] == DiveTripModelBase::CURRENT_ROLE &&
	    source->data(topLeft, DiveTripModelBase::CURRENT_ROLE).value<bool>()) {
	    if (topLeft.parent().isValid()) {
		int parentRow = mapRowFromSourceTopLevel(topLeft.parent().row());
		if (parentRow != expandedRow) {
			expand(parentRow);
			return;
		}
	    } else {
		    unexpand();
	    }
	}

	if (topLeft.parent().isValid()) {
		// This is a range in a trip. First do a sanity check.
		if (topLeft.parent().row() != bottomRight.parent().row()) {
			qWarning("MobileListModel::changed(): changes inside different trips. Ignoring.");
			return;
		}

		// Now check whether this is expanded
		IndexRange range = mapRangeFromSource(topLeft.parent(), topLeft.row(), bottomRight.row());
		if (!range.visible)
			return;

		dataChanged(createIndex(range.first, topLeft.column()), createIndex(range.last, bottomRight.column()), roles);
	} else {
		// This is a top-level range.
		IndexRange range = mapRangeFromSource(topLeft.parent(), topLeft.row(), bottomRight.row());

		// If the expanded row is outside the region to be updated
		// or the last entry in the region to be updated, we can simply
		// forward the signal.
		if (expandedRow < 0 || expandedRow < range.first || expandedRow >= range.last) {
			dataChanged(createIndex(range.first, topLeft.column()), createIndex(range.last, bottomRight.column()), roles);
			return;
		}

		// We have to split this in two parts: before and including the expanded row
		// and everything after the expanded row.
		int numSub = numSubItems();
		dataChanged(createIndex(range.first, topLeft.column()), createIndex(expandedRow, bottomRight.column()), roles);
		dataChanged(createIndex(expandedRow + 1 + numSub, topLeft.column()), createIndex(range.last, bottomRight.column()), roles);
	}
}

void MobileListModel::invalidate()
{
	// Qt's model/view API can't handle empty ranges and we have to subtract one from the last item,
	// because ranges are given as [first,last] (i.e. last inclusive).
	int rows = rowCount(QModelIndex());
	if (rows <= 0)
		return;
	QModelIndex fromIdx = createIndex(0, 0);
	QModelIndex toIdx = createIndex(rows - 1, 0);
	dataChanged(fromIdx, toIdx);
}

void MobileListModel::unexpand()
{
	if (expandedRow < 0)
		return;
	int first = expandedRow + 1;
	int numRows = numSubItems();
	int last = first + numRows - 1;
	if (last < first) {
		// Amazingly, Qt's model API doesn't properly handle empty ranges!
		expandedRow = -1;
		return;
	}
	beginRemoveRows(QModelIndex(), first, last);
	expandedRow = -1;
	endRemoveRows();
}

void MobileListModel::toggle(int row)
{
	if (row < 0)
		return;
	else if (row == expandedRow)
		unexpand();
	else
		expand(row);
}

MobileSwipeModel::MobileSwipeModel(DiveTripModelBase *source) : MobileListModelBase(source)
{
	connect(source, &DiveTripModelBase::modelAboutToBeReset, this, &MobileSwipeModel::beginResetModel);
	connect(source, &DiveTripModelBase::modelReset, this, &MobileSwipeModel::doneReset);
	connect(source, &DiveTripModelBase::rowsAboutToBeRemoved, this, &MobileSwipeModel::prepareRemove);
	connect(source, &DiveTripModelBase::rowsRemoved, this, &MobileSwipeModel::doneRemove);
	connect(source, &DiveTripModelBase::rowsAboutToBeInserted, this, &MobileSwipeModel::prepareInsert);
	connect(source, &DiveTripModelBase::rowsInserted, this, &MobileSwipeModel::doneInsert);
	connect(source, &DiveTripModelBase::rowsAboutToBeMoved, this, &MobileSwipeModel::prepareMove);
	connect(source, &DiveTripModelBase::rowsMoved, this, &MobileSwipeModel::doneMove);
	connect(source, &DiveTripModelBase::dataChanged, this, &MobileSwipeModel::changed);

	initData();
}

// Return the size of a top level item in the source model. Whereby size
// is the number of items it represents in the swipe model:
// A dive has size one, a trip has the size of the number of its items.
// Attention: the given row is expressed in source-coordinates!
int MobileSwipeModel::topLevelRowCountInSource(int sourceRow) const
{
	QModelIndex index = source->index(sourceRow, 0, QModelIndex());
	return source->data(index, DiveTripModelBase::IS_TRIP_ROLE).value<bool>() ?
		source->rowCount(index) : 1;
}

void MobileSwipeModel::initData()
{
	rows = 0;
	int act = 0;
	int topLevelRows = source->rowCount();
	firstElement.resize(topLevelRows);
	for (int i = 0; i < topLevelRows; ++i) {
		firstElement[i] = act;
		// Note: we populate the model in reverse order, because we show the newest dives first.
		act += topLevelRowCountInSource(topLevelRows - i - 1);
	}
	rows = act;
	invalidateSourceRowCache();
}

void MobileSwipeModel::doneReset()
{
	initData();
	endResetModel();
}

void MobileSwipeModel::invalidateSourceRowCache() const
{
	cachedRow = -1;
	cacheSourceParent = QModelIndex();
	cacheSourceRow = -1;
}

void MobileSwipeModel::updateSourceRowCache(int localRow) const
{
	if (firstElement.empty())
		return invalidateSourceRowCache();

	cachedRow = localRow;

	// Do a binary search for the first top-level item that starts after the given row
	auto idx = std::upper_bound(firstElement.begin(), firstElement.end(), localRow);
	if (idx == firstElement.begin())
		return invalidateSourceRowCache(); // Huh? localRow was negative? Then index->isValid() should have returned true.

	--idx;
	int topLevelRow = idx - firstElement.begin();
	int topLevelRowSource = firstElement.end() - idx - 1; // Reverse direction.
	int indexInRow = localRow - *idx;
	if (indexInRow == 0) {
		// This might be a top-level dive or a one-dive trip. Perhaps we should save which one it is.
		if (!source->data(source->index(topLevelRowSource, 0), DiveTripModelBase::IS_TRIP_ROLE).value<bool>()) {
			cacheSourceParent = QModelIndex();
			cacheSourceRow = topLevelRowSource;
			return;
		}
	}
	cacheSourceParent = source->index(topLevelRowSource, 0);
	int numElements = elementCountInTopLevel(topLevelRow);
	cacheSourceRow = numElements - indexInRow - 1;
}

QModelIndex MobileSwipeModel::mapToSource(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();
	if (index.row() != cachedRow)
		updateSourceRowCache(index.row());

	return cacheSourceRow >= 0 ? source->index(cacheSourceRow, index.column(), cacheSourceParent) : QModelIndex();
}

int MobileSwipeModel::mapTopLevelFromSource(int row) const
{
	return firstElement.size() - row - 1;
}

int MobileSwipeModel::mapTopLevelFromSourceForInsert(int row) const
{
	return firstElement.size() - row;
}

int MobileSwipeModel::elementCountInTopLevel(int row) const
{
	if (row < 0 || row >= (int)firstElement.size())
		return 0;
	if (row + 1 < (int)firstElement.size())
		return firstElement[row + 1] - firstElement[row];
	else
		return rows - firstElement[row];
}

int MobileSwipeModel::mapRowFromSource(const QModelIndex &parent, int row) const
{
	if (parent.isValid()) {
		int topLevelRow = mapTopLevelFromSource(parent.row());
		int count = elementCountInTopLevel(topLevelRow);
		return firstElement[topLevelRow] + count - row - 1; // Note: we invert the direction!
	} else {
		int topLevelRow = mapTopLevelFromSource(row);
		return firstElement[topLevelRow];
	}
}

int MobileSwipeModel::mapRowFromSource(const QModelIndex &idx) const
{
	return mapRowFromSource(idx.parent(), idx.row());
}

int MobileSwipeModel::mapRowFromSourceForInsert(const QModelIndex &parent, int row) const
{
	if (parent.isValid()) {
		int topLevelRow = mapTopLevelFromSource(parent.row());
		int count = elementCountInTopLevel(topLevelRow);
		return firstElement[topLevelRow] + count - row; // Note: we invert the direction!
	} else {
		if (row == 0)
			return rows;	// Insert at the end
		int topLevelRow = mapTopLevelFromSource(row - 1);
		return firstElement[topLevelRow]; // Note: we invert the direction!
	}
}

MobileSwipeModel::IndexRange MobileSwipeModel::mapRangeFromSource(const QModelIndex &parent, int first, int last) const
{
	// Since we invert the direction, the last will be the first.
	if (!parent.isValid()) {
		int localFirst = mapRowFromSource(QModelIndex(), last);
		// Point to the *last* item in the topLevelRange. Yay for Qt's bizzare [first,last] range-semantics.
		int localLast = mapRowFromSource(QModelIndex(), first);
		int topLevelLast = mapTopLevelFromSource(first);
		localLast += elementCountInTopLevel(topLevelLast) - 1;
		return { localFirst, localLast };
	} else {
		// For items inside trips we can simply translate them, as they cannot contain subitems.
		// Remember to reverse the direction, though.
		return { mapRowFromSource(parent, last), mapRowFromSource(parent, first) };
	}
}

// Remove top-level items. Parameters with standard range semantics (pointer to first and past last element).
int MobileSwipeModel::removeTopLevel(int begin, int end)
{
	auto it1 = firstElement.begin() + begin;
	auto it2 = firstElement.begin() + end;
	int count = 0; // Number of items we have to subtract from rest
	for (int row = begin; row < end; ++row)
		count += elementCountInTopLevel(row);
	firstElement.erase(it1, it2); // Remove items
	for (auto act = firstElement.begin() + begin; act != firstElement.end(); ++act)
		*act -= count; // Subtract removed items
	rows -= count;
	return count;
}

// Add or remove subitems from top-level items
void MobileSwipeModel::updateTopLevel(int row, int delta)
{
	for (int i = row + 1; i < (int)firstElement.size(); ++i)
		firstElement[i] += delta;
	rows += delta;
}

// Add items at top-level. The number of subelements of each items is given in the second parameter.
void MobileSwipeModel::addTopLevel(int row, std::vector<int> items)
{
	// We get an array with the number of items per inserted row.
	// Transform that to the first element in each row.
	int nextEl = row < (int)firstElement.size() ? firstElement[row] : rows;
	int count = 0;
	for (int &item: items) {
		int num = item;
		item = nextEl;
		nextEl += num;
		count += num;
	}

	// Now, increase the first element of the items after the inserted range
	// by the number of inserted items.
	auto it = firstElement.begin() + row;
	for (auto act = it; act != firstElement.end(); ++act)
		*act += count;
	rows += count;

	// Insert the range
	firstElement.insert(it, items.begin(), items.end());
}

void MobileSwipeModel::prepareRemove(const QModelIndex &parent, int first, int last)
{
	IndexRange range = mapRangeFromSource(parent, first, last);
	rangeStack.push_back(range);
	if (range.last >= range.first)
		beginRemoveRows(QModelIndex(), range.first, range.last);
}

void MobileSwipeModel::doneRemove(const QModelIndex &parent, int first, int last)
{
	IndexRange range = pop(rangeStack);
	if (range.last < range.first)
		return;
	if (!parent.isValid()) {
		// This is a top-level range. This means that we have to remove top-level items.
		// Remember to invert the direction.
		removeTopLevel(mapTopLevelFromSource(last), mapTopLevelFromSource(first) + 1);
	} else {
		// This is part of a trip. Only the number of items has to be changed.
		updateTopLevel(mapTopLevelFromSource(parent.row()), -(last - first + 1));
	}
	invalidateSourceRowCache();
	endRemoveRows();
}

void MobileSwipeModel::prepareInsert(const QModelIndex &parent, int first, int last)
{
	// We can not call beginInsertRows here, because before the source model
	// has inserted its rows we don't know how many subitems there are!
}

void MobileSwipeModel::doneInsert(const QModelIndex &parent, int first, int last)
{
	if (!parent.isValid()) {
		// This is a top-level range. This means that we have to add top-level items.

		// Create vector of new top-level items
		std::vector<int> items;
		items.reserve(last - first + 1);
		int count = 0;
		for (int row = last; row >= first; --row) {
			items.push_back(topLevelRowCountInSource(row));
			count += items.back();
		}

		int firstLocal = mapTopLevelFromSourceForInsert(first);
		if (firstLocal >= 0) {
			beginInsertRows(QModelIndex(), firstLocal, firstLocal + count - 1);
			addTopLevel(firstLocal, std::move(items));
			endInsertRows();
		} else {
			qWarning("MobileSwipeModel::doneInsert(): invalid source index!\n");
		}
	} else {
		// This is part of a trip. Only the number of items has to be changed.
		int row = mapRowFromSourceForInsert(parent, first);
		int count = last - first + 1;
		beginInsertRows(QModelIndex(), row, row + count - 1);
		updateTopLevel(mapTopLevelFromSource(parent.row()), count);
		endInsertRows();
	}
	invalidateSourceRowCache();
}

void MobileSwipeModel::prepareMove(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int destRow)
{
	IndexRange range = mapRangeFromSource(parent, first, last);
	rangeStack.push_back(range);
	if (range.last >= range.first)
		beginMoveRows(QModelIndex(), range.first, range.last, QModelIndex(), mapRowFromSourceForInsert(dest, destRow));
}

void MobileSwipeModel::doneMove(const QModelIndex &parent, int first, int last, const QModelIndex &dest, int destRow)
{
	IndexRange range = pop(rangeStack);
	if (range.last < range.first)
		return;

	// Moving is annoying. There are four cases to consider, depending whether
	// we move in / out of a top-level item!
	if (!parent.isValid() && !dest.isValid()) {
		// From top-level to top-level
		if (destRow < first || destRow > last + 1) {
			int beginLocal = mapTopLevelFromSource(last);
			int endLocal = mapTopLevelFromSource(first) + 1;
			int destLocal = mapTopLevelFromSourceForInsert(destRow);
			int count = endLocal - beginLocal;
			std::vector<int> items;
			items.reserve(count);
			for (int row = beginLocal; row < endLocal; ++row) {
				items.push_back(row < (int)firstElement.size() - 1 ? firstElement[row + 1] - firstElement[row]
									      : rows - firstElement[row]);
			}
			removeTopLevel(mapTopLevelFromSource(last), mapTopLevelFromSource(first) + 1);

			if (destLocal >= beginLocal)
				destLocal -= count;
			addTopLevel(destLocal, std::move(items));
		}
	} else if (!parent.isValid() && dest.isValid()) {
		// From top-level to trip
		int beginLocal = mapTopLevelFromSource(last);
		int endLocal = mapTopLevelFromSource(first) + 1;
		int destLocal = mapTopLevelFromSourceForInsert(dest.row());
		int count = endLocal - beginLocal;
		int numMoved = removeTopLevel(beginLocal, endLocal);
		if (destLocal >= beginLocal)
			destLocal -= count;
		updateTopLevel(destLocal, numMoved);
	} else if (parent.isValid() && !dest.isValid()) {
		// From trip to top-level
		int fromLocal = mapTopLevelFromSource(parent.row());
		int toLocal = mapTopLevelFromSourceForInsert(dest.row());
		int numMoved = last - first + 1;
		std::vector<int> items(numMoved, 1); // This can only be dives -> item count is 1
		updateTopLevel(fromLocal, -numMoved);
		addTopLevel(toLocal, std::move(items));
	} else {
		// From trip to other trip
		int fromLocal = mapTopLevelFromSource(parent.row());
		int toLocal = mapTopLevelFromSourceForInsert(dest.row());
		int numMoved = last - first + 1;
		if (fromLocal != toLocal) {
			updateTopLevel(fromLocal, -numMoved);
			updateTopLevel(toLocal, numMoved);
		}
	}
	invalidateSourceRowCache();
	endMoveRows();
}

void MobileSwipeModel::changed(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
	if (!topLeft.isValid() || !bottomRight.isValid())
		return;

	// We don't display trips in the swipe model. If we get changed signals for that - ignore it.
	// Subtle: we only check that for single-row changes, because the source model sends changes
	// to trips one-by-one. The way we query the source model is ... not nice to read.
	if (topLeft.row() == bottomRight.row() &&
	    source->data(topLeft, DiveTripModelBase::IS_TRIP_ROLE).value<bool>())
		return;

	int fromSource = mapRowFromSource(bottomRight);
	int toSource = mapRowFromSource(topLeft);
	QModelIndex fromIdx = createIndex(fromSource, topLeft.column());
	QModelIndex toIdx = createIndex(toSource, bottomRight.column());

	dataChanged(fromIdx, toIdx, roles);

	// Special case CURRENT_ROLE: if a dive becomes current, we send a signal so that the
	// dive-details page can update the current dive. It would be nicer if the frontend could
	// hook into the changed-signal, but currently I don't know how this works in QML.
	// Note: changes to current must not be combined with other changes, therefore we can
	// assume that roles.size() == 1.
	if (roles.size() == 1 && roles[0] == DiveTripModelBase::CURRENT_ROLE &&
	    source->data(topLeft, DiveTripModelBase::CURRENT_ROLE).value<bool>())
		emit currentDiveChanged(fromIdx);
}

void MobileSwipeModel::invalidate()
{
	// Qt's model/view API can't handle empty ranges and we have to subtract one from the last item,
	// because ranges are given as [first,last] (i.e. last inclusive).
	int rows = rowCount(QModelIndex());
	if (rows <= 0)
		return;
	QModelIndex fromIdx = createIndex(0, 0);
	QModelIndex toIdx = createIndex(rows - 1, 0);
	dataChanged(fromIdx, toIdx);
}

QVariant MobileSwipeModel::data(const QModelIndex &index, int role) const
{
	return source->data(mapToSource(index), role);
}

int MobileSwipeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0; // There is no parent
	return rows;
}

MobileModels *MobileModels::instance()
{
	static MobileModels self;
	return &self;
}

MobileModels::MobileModels() :
	lm(&source),
	sm(&source)
{
}

MobileListModel *MobileModels::listModel()
{
	return &lm;
}

MobileSwipeModel *MobileModels::swipeModel()
{
	return &sm;
}

// This is called when the settings changed. Instead of rebuilding the model, send a changed signal on all entries.
void MobileModels::invalidate()
{
	sm.invalidate();
	sm.invalidate();
}
