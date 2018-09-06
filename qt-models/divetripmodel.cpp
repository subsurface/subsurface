// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divetripmodel.h"
#include "core/gettextfromc.h"
#include "core/metrics.h"
#include "core/divelist.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
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
	}

	if (role == DiveTripModel::STAR_ROLE) {
		retVal = d->rating;
	}
	if (role == DiveTripModel::DIVE_ROLE) {
		retVal = QVariant::fromValue<void *>(d);
	}
	if (role == DiveTripModel::DIVE_IDX) {
		retVal = get_divenr(d);
	}
	return retVal;
}

Qt::ItemFlags DiveItem::flags(const QModelIndex &index) const
{
	if (index.column() == NR) {
		return TreeItem::flags(index) | Qt::ItemIsEditable;
	}
	return TreeItem::flags(index);
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
	TreeModel(parent),
	currentLayout(TREE)
{
	columns = COLUMNS;
}

Qt::ItemFlags DiveTripModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	TripItem *item = static_cast<TripItem *>(index.internalPointer());
	return item->flags(index);
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

void DiveTripModel::setupModelData()
{
	int i = dive_table.nr;

	beginResetModel();

	clear();
	if (autogroup)
		autogroup_dives();
	QMap<dive_trip_t *, TripItem *> trips;
	while (--i >= 0) {
		struct dive *dive = get_dive(i);
		update_cylinder_related_info(dive);
		dive_trip_t *trip = dive->divetrip;

		DiveItem *diveItem = new DiveItem();
		diveItem->d = dive;

		if (!trip || currentLayout == LIST) {
			diveItem->parent = rootItem.get();
			rootItem->children.push_back(diveItem);
			continue;
		}
		if (currentLayout == LIST)
			continue;

		if (!trips.keys().contains(trip)) {
			TripItem *tripItem = new TripItem();
			tripItem->trip = trip;
			tripItem->parent = rootItem.get();
			tripItem->children.push_back(diveItem);
			trips[trip] = tripItem;
			rootItem->children.push_back(tripItem);
			continue;
		}
		TripItem *tripItem = trips[trip];
		tripItem->children.push_back(diveItem);
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

bool DiveTripModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
	DiveItem *diveItem = dynamic_cast<DiveItem *>(item);
	if (!diveItem)
		return false;
	return diveItem->setData(index, value, role);
}
