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
	emit shownChanged();
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
