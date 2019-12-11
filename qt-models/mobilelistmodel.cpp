// SPDX-License-Identifier: GPL-2.0
#include "mobilelistmodel.h"
#include "core/divelist.h" // for shown_dives

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
	return roles;
}

int MobileListModel::shown() const
{
	return shown_dives;
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
// the indexes with respect to the source model. To avoid mental gymnastics
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
MobileModels *MobileModels::instance()
{
	static MobileModels self;
	return &self;
}

MobileModels::MobileModels() :
	lm(&source)
{
	reset();
}

MobileListModel *MobileModels::listModel()
{
	return &lm;
}

void MobileModels::clear()
{
	source.clear();
}

void MobileModels::reset()
{
	source.reset();
}
