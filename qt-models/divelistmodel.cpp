// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divelistmodel.h"
#include "core/divefilter.h"
#include "core/divesite.h"
#include "core/qthelper.h"
#include "core/trip.h"
#include "core/settings/qPrefGeneral.h"
#include "core/errorhelper.h" // for verbose
#include <QDateTime>
#include <QtConcurrent>
#include <QDebug>

// the DiveListSortModel creates the sorted, filtered list of dives that the user
// can flip through horizontally
// the CollapsedDiveListSortModel creates the vertical dive list which is a second
// filter on top of the one applied to the DiveListSortModel

CollapsedDiveListSortModel::CollapsedDiveListSortModel()
{
	setSourceModel(DiveListSortModel::instance());
	// make sure that we after changes to the underlying model (and therefore the dive list
	// we update the filter state
	connect(DiveListModel::instance(), &DiveListModel::rowsInserted, this, &CollapsedDiveListSortModel::updateFilterState);
	connect(DiveListModel::instance(), &DiveListModel::rowsMoved, this, &CollapsedDiveListSortModel::updateFilterState);
	connect(DiveListModel::instance(), &DiveListModel::rowsRemoved, this, &CollapsedDiveListSortModel::updateFilterState);
}

CollapsedDiveListSortModel *CollapsedDiveListSortModel::instance()
{
	static CollapsedDiveListSortModel self;
	return &self;
}

void CollapsedDiveListSortModel::setSourceModel(QAbstractItemModel *sourceModel)
{
	QSortFilterProxyModel::setSourceModel(sourceModel);
	// make sure we sort descending and have the filters correct
	setDynamicSortFilter(true);
	setSortRole(DiveListModel::DiveDateRole);
	sort(0, Qt::DescendingOrder);
	updateFilterState();
}

// In QtQuick ListView, section headings can only be strings. To identify dives
// that belong to the same trip, a string containing the trip-id is passed in.
// To format the trip heading, the string is then converted back with this function.
static dive_trip *tripIdToObject(const QString &s)
{
	if (s.isEmpty())
		return nullptr;
	int id = s.toInt();
	dive_trip **trip = std::find_if(&trip_table.trips[0], &trip_table.trips[trip_table.nr],
					[id] (const dive_trip *t) { return t->id == id; });
	if (trip == &trip_table.trips[trip_table.nr]) {
		fprintf(stderr, "Warning: unknown trip id passed through QML: %d\n", id);
		return nullptr;
	}
	return *trip;
}

// the trip title is designed to be location (# dives)
// or, if there is no location name date range (# dives)
// where the date range is given as "month year" or "month-month year" or "month year - month year"
QString CollapsedDiveListSortModel::tripTitle(const QString &section)
{
	const dive_trip *dt = tripIdToObject(section);
	if (!dt)
		return QString();
	QString numDives = tr("(%n dive(s))", "", dt->dives.nr);
	int shown = trip_shown_dives(dt);
	QString shownDives = shown != dt->dives.nr ? QStringLiteral(" ") + tr("(%L1 shown)").arg(shown) : QString();
	QString title(dt->location);

	if (title.isEmpty()) {
		// so use the date range
		QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(1000*trip_date(dt), Qt::UTC);
		QString firstMonth = firstTime.toString("MMM");
		QString firstYear = firstTime.toString("yyyy");
		QDateTime lastTime = QDateTime::fromMSecsSinceEpoch(1000*dt->dives.dives[0]->when, Qt::UTC);
		QString lastMonth = lastTime.toString("MMM");
		QString lastYear = lastTime.toString("yyyy");
		if (lastMonth == firstMonth && lastYear == firstYear)
			title = firstMonth + " " + firstYear;
		else if (lastMonth != firstMonth && lastYear == firstYear)
			title = firstMonth + "-" + lastMonth + " " + firstYear;
		else
			title = firstMonth + " " + firstYear + " - " + lastMonth + " " + lastYear;
	}
	return QStringLiteral("%1 %2%3").arg(title, numDives, shownDives);
}

QString CollapsedDiveListSortModel::tripShortDate(const QString &section)
{
	const dive_trip *dt = tripIdToObject(section);
	if (!dt)
		return QString();
	QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(1000*trip_date(dt), Qt::UTC);
	QString firstMonth = firstTime.toString("MMM");
	return QStringLiteral("%1\n'%2").arg(firstMonth,firstTime.toString("yy"));
}

void CollapsedDiveListSortModel::setActiveTrip(const QString &trip)
{
	m_activeTrip = trip;
	// we can't update the filter state from the this function as that is called from
	// a slot in the QML code which could cause the object that is executing the slot
	// to be destroyed before this function returns.
	// Instead do this asynchronously
	QtConcurrent::run(QThreadPool::globalInstance(),
			  []{
				CollapsedDiveListSortModel::instance()->updateFilterState();
			  });
}

QString CollapsedDiveListSortModel::activeTrip() const
{
	return m_activeTrip;
}

// tell us if this dive is the first dive in a trip that has at least one
// dive that isn't hidden (even if this dive is hidden)
static bool isFirstInNotCompletelyHiddenTrip(struct dive *d)
{
	struct dive_trip *dt = d->divetrip;
	if (dt->dives.nr > 0 && dt->dives.dives[0] == d) {
		// ok, this is the first dive in its trip
		int i = -1;
		while (++i < dt->dives.nr)
			if (!dt->dives.dives[i]->hidden_by_filter)
				return true;
	}
	return false;
}

// the mobile app allows only one selected dive
// that means there are either zero or exactly one expanded trip -
bool CollapsedDiveListSortModel::isExpanded(struct dive_trip *dt) const
{
	return !m_activeTrip.isEmpty() && dt == tripIdToObject(m_activeTrip);
}

void CollapsedDiveListSortModel::updateFilterState()
{
	// now do something clever to show the right dives
	// first make sure that the underlying filtering is taken care of
	DiveListSortModel::instance()->updateFilterState();
	int i;
	struct dive *d;
	for_each_dive(i, d) {
		CollapsedState state = DontShow;
		struct dive_trip *dt = d->divetrip;

		// we show the dives that are outside of a trip or inside of the one expanded trip
		if (!d->hidden_by_filter && (dt == nullptr || isExpanded(dt)))
			state = ShowDive;
		// we mark the first dive of a trip that contains any unfiltered dives as ShowTrip  or ShowDiveAndTrip (if this is the one expanded trip)
		if (dt != nullptr && isFirstInNotCompletelyHiddenTrip(d))
			state = (state == ShowDive) ? ShowDiveAndTrip : ShowTrip;
		d->collapsed = state;
	}
	// everything up to here can be done even if we don't have a source model
	if (sourceModel() != nullptr) {
		DiveListModel *dlm = DiveListModel::instance();
		dlm->dataChanged(dlm->index(0,0), dlm->index(dlm->rowCount() - 1, 0));
	}
}

void CollapsedDiveListSortModel::updateSelectionState()
{
	QVector<int> changedRoles = { DiveListModel::SelectedRole };
	dataChanged(index(0,0), index(rowCount() - 1, 0), changedRoles);
}

bool CollapsedDiveListSortModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
	// get the corresponding dive from the DiveListModel and check if we should show it
	const dive *d = DiveListModel::instance()->getDive(source_row);
	if (verbose > 1)
		qDebug() << "FAR source row" << source_row << "dive" << (d ? QString::number(d->number) : "NULL") << "is" << (d != nullptr && d->collapsed != DontShow) <<
			    (d != nullptr ? QString::number(d->collapsed) : "");
	return d != nullptr && d->collapsed != DontShow;
}

DiveListSortModel::DiveListSortModel()
{
	setSourceModel(DiveListModel::instance());
}

DiveListSortModel *DiveListSortModel::instance()
{
	static DiveListSortModel self;
	return &self;
}

void DiveListSortModel::updateFilterState()
{
	DiveFilter::instance()->updateAll();
}

void DiveListSortModel::setSourceModel(QAbstractItemModel *sourceModel)
{
	QSortFilterProxyModel::setSourceModel(sourceModel);
	// make sure we sort descending and have the filters correct
	setDynamicSortFilter(true);
	setSortRole(DiveListModel::DiveDateRole);
	sort(0, Qt::DescendingOrder);
	updateFilterState();
}

void DiveListSortModel::setFilter(QString f, FilterData::Mode mode)
{
	f = f.trimmed();
	FilterData data;
	if (!f.isEmpty()) {
		data.mode = mode;
		if (mode == FilterData::Mode::FULLTEXT)
			data.fullText = f;
		else
			data.tags = f.split(",", QString::SkipEmptyParts);
	}
	DiveFilter::instance()->setFilter(data);
	CollapsedDiveListSortModel::instance()->updateFilterState();
	invalidateFilter();
}

void DiveListSortModel::resetFilter()
{
	int i;
	struct dive *d;
	for_each_dive(i, d)
		d->hidden_by_filter = false;
	invalidateFilter();
}

// filtering is way too slow on mobile. Maybe we should roll our own?
bool DiveListSortModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
	DiveListModel *mySourceModel = qobject_cast<DiveListModel *>(sourceModel());
	const dive *d = mySourceModel->getDive(source_row);
	return d && !d->hidden_by_filter;
}

int DiveListSortModel::shown()
{
	return rowCount();
}

int DiveListSortModel::getIdxForId(int id)
{
	DiveListModel *mySourceModel = qobject_cast<DiveListModel *>(sourceModel());
	QModelIndex sourceIdx = mySourceModel->getDiveQIdx(id);
	if (!sourceIdx.isValid())
		return -1;
	QModelIndex localIdx = mapFromSource(sourceIdx);
	return localIdx.row();
}

void DiveListSortModel::reload()
{
	DiveListModel *mySourceModel = qobject_cast<DiveListModel *>(sourceModel());
	mySourceModel->resetInternalData();
}

DiveListModel::DiveListModel()
{
}

void DiveListModel::insertDive(int i)
{
	beginInsertRows(QModelIndex(), i, i);
	endInsertRows();
}

void DiveListModel::removeDive(int i)
{
	beginRemoveRows(QModelIndex(), i, i);
	endRemoveRows();
}

void DiveListModel::removeDiveById(int id)
{
	for (int i = 0; i < dive_table.nr; i++) {
		if (dive_table.dives[i]->id == id) {
			removeDive(i);
			return;
		}
	}
}

void DiveListModel::updateDive(int i, dive *d)
{
	// we need to make sure that QML knows that this dive has changed -
	// the only reliable way I've found is to remove and re-insert it
	removeDive(i);
	insertDive(i);
}

void DiveListModel::clear()
{
	beginResetModel();
	clear_dive_file_data();
	endResetModel();
}

void DiveListModel::reload()
{
	// Note: instead of doing a (logical) beginResetModel()/endResetModel(),
	// we add the rows (if any). The reason is that a beginResetModel()/endResetModel()
	// pair resulted in the DiveDetailsPage being renedered for *every* dive in
	// the list. It is unclear whether this is a Qt-bug or intended insanity.
	// Therefore, this function must only be called after having called clear().
	// Otherwise the model will become inconsistent!
	if (dive_table.nr > 0) {
		beginInsertRows(QModelIndex(), 0, dive_table.nr - 1);
		endInsertRows();
	}
}

void DiveListModel::resetInternalData()
{
	// this is a hack. There is a long standing issue, that seems related to a
	// sync problem between QML engine and underlying model data. It causes delete
	// from divelist (on mobile) to crash. But not always. This function is part of
	// an attempt to fix this. See commit.
	beginResetModel();
	endResetModel();
}

int DiveListModel::rowCount(const QModelIndex &) const
{
	return dive_table.nr;
}

// Get the index of a dive in the global dive list by the dive's unique id. Returns an integer [0..nrdives).
int DiveListModel::getDiveIdx(int id) const
{
	return get_idx_by_uniq_id(id);
}

// Get an index of a dive. In contrast to getDiveIdx, this returns a Qt model-index,
// which can be used to access data of a Qt model.
QModelIndex DiveListModel::getDiveQIdx(int id)
{
	int idx = getDiveIdx(id);
	return idx >= 0 ? createIndex(idx, 0) : QModelIndex();
}

QVariant DiveListModel::data(const QModelIndex &index, int role) const
{
	if(index.row() < 0 || index.row() >= dive_table.nr)
		return QVariant();

	dive *d = dive_table.dives[index.row()];
	if (!d)
		return QVariant();
	switch(role) {
	case DiveDateRole: return (qlonglong)d->when;
	// We have to return a QString as trip-id, because that will be used as section
	// variable in the QtQuick list view. That has to be a string because it will try
	// to do locale-aware sorting. And amazingly this can't be changed.
	case TripIdRole: return d->divetrip ? QString::number(d->divetrip->id) : QString();
	case TripNrDivesRole: return d->divetrip ? d->divetrip->dives.nr : 0;
	case DateTimeRole: {
		QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000 * d->when, Qt::UTC);
		localTime.setTimeSpec(Qt::UTC);
		return QStringLiteral("%1 %2").arg(localTime.date().toString(prefs.date_format_short),
						   localTime.time().toString(prefs.time_format));
		}
	case IdRole: return d->id;
	case NumberRole: return d->number;
	case LocationRole: return get_dive_location(d);
	case DepthRole: return get_depth_string(d->dc.maxdepth.mm, true, true);
	case DurationRole: return get_dive_duration_string(d->duration.seconds, gettextFromC::tr("h"), gettextFromC::tr("min"));
	case DepthDurationRole: return QStringLiteral("%1 / %2").arg(get_depth_string(d->dc.maxdepth.mm, true, true),
								     get_dive_duration_string(d->duration.seconds, gettextFromC::tr("h"), gettextFromC::tr("min")));
	case RatingRole: return d->rating;
	case VizRole: return d->visibility;
	case SuitRole: return d->suit;
	case AirTempRole: return get_temperature_string(d->airtemp, true);
	case WaterTempRole: return get_temperature_string(d->watertemp, true);
	case SacRole: return formatSac(d);
	case SumWeightRole: return get_weight_string(weight_t { total_weight(d) }, true);
	case DiveMasterRole: return d->divemaster ? d->divemaster : QString();
	case BuddyRole: return d->buddy ? d->buddy : QString();
	case NotesRole: return formatNotes(d);
	case GpsRole: return d->dive_site ? printGPSCoords(&d->dive_site->location) : QString();
	case GpsDecimalRole: return format_gps_decimal(d);
	case NoDiveRole: return d->duration.seconds == 0 && d->dc.duration.seconds == 0;
	case DiveSiteRole: return QVariant::fromValue(d->dive_site);
	case CylinderRole: return formatGetCylinder(d).join(", ");
	case GetCylinderRole: return formatGetCylinder(d);
	case CylinderListRole: return getFullCylinderList();
	case SingleWeightRole: return d->weightsystems.nr <= 1;
	case StartPressureRole: return getStartPressure(d);
	case EndPressureRole: return getEndPressure(d);
	case FirstGasRole: return getFirstGas(d);
	case CollapsedRole: return d->collapsed;
	case SelectedRole: return d->selected;
	}
	return QVariant();
}

QHash<int, QByteArray> DiveListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[DiveDateRole] = "date";
	roles[TripIdRole] = "tripId";
	roles[TripNrDivesRole] = "tripNrDives";
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
	roles[CollapsedRole] = "collapsed";
	roles[SelectedRole] = "selected";
	return roles;
}

// create a new dive. set the current time and add it to the end of the dive list
QString DiveListModel::startAddDive()
{
	struct dive *d;
	d = alloc_dive();
	d->when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset();

	// find the highest dive nr we have and pick the next one
	struct dive *pd;
	int i, nr = 0;
	for_each_dive(i, pd) {
		if (pd->number > nr)
			nr = pd->number;
	}
	nr++;
	d->number = nr;
	d->dc.model = strdup("manually added dive");
	append_dive(d);
	insertDive(get_idx_by_uniq_id(d->id));
	return QString::number(d->id);
}

DiveListModel *DiveListModel::instance()
{
	static DiveListModel self;
	return &self;
}

struct dive *DiveListModel::getDive(int i)
{
	if (i < 0 || i >= dive_table.nr) {
		qWarning("DiveListModel::getDive(): accessing invalid dive with id %d", i);
		return nullptr;
	}
	return dive_table.dives[i];
}

DiveObjectHelper DiveListModel::at(int i)
{
	// For an invalid id, returns an invalid DiveObjectHelper that will crash on access.
	dive *d = getDive(i);
	return d ? DiveObjectHelper(d) : DiveObjectHelper();
}
