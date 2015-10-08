#include "divetripmodel.h"
#include "gettextfromc.h"
#include "metrics.h"
#include "divelist.h"
#include "helpers.h"
#include <QIcon>

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
	case DiveTripModel::LOCATION:
		retVal = int(Qt::AlignLeft | Qt::AlignVCenter);
		break;
	}
	return retVal;
}

QVariant TripItem::data(int column, int role) const
{
	QVariant ret;

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
				d = d->next;
			}
			if (countShown < trip->nrdives)
				shownText = tr("(%1 shown)").arg(countShown);
			if (trip->location && *trip->location)
				ret = QString(trip->location) + ", " + get_trip_date_string(trip->when, trip->nrdives) + " " + shownText;
			else
				ret = get_trip_date_string(trip->when, trip->nrdives) + shownText;
			break;
		}
	}

	return ret;
}


QVariant DiveItem::data(int column, int role) const
{
	QVariant retVal;
	struct dive *dive = get_dive_by_uniq_id(diveId);
	if (!dive)
		return QVariant();

	switch (role) {
	case Qt::TextAlignmentRole:
		retVal = dive_table_alignment(column);
		break;
	case DiveTripModel::SORT_ROLE:
		Q_ASSERT(dive != NULL);
		switch (column) {
		case NR:
			retVal = (qulonglong)dive->when;
			break;
		case DATE:
			retVal = (qulonglong)dive->when;
			break;
		case RATING:
			retVal = dive->rating;
			break;
		case DEPTH:
			retVal = dive->maxdepth.mm;
			break;
		case DURATION:
			retVal = dive->duration.seconds;
			break;
		case TEMPERATURE:
			retVal = dive->watertemp.mkelvin;
			break;
		case TOTALWEIGHT:
			retVal = total_weight(dive);
			break;
		case SUIT:
			retVal = QString(dive->suit);
			break;
		case CYLINDER:
			retVal = QString(dive->cylinder[0].type.description);
			break;
		case GAS:
			retVal = nitrox_sort_value(dive);
			break;
		case SAC:
			retVal = dive->sac;
			break;
		case OTU:
			retVal = dive->otu;
			break;
		case MAXCNS:
			retVal = dive->maxcns;
			break;
		case LOCATION:
			retVal = QString(get_dive_location(dive));
			break;
		}
		break;
	case Qt::DisplayRole:
		Q_ASSERT(dive != NULL);
		switch (column) {
		case NR:
			retVal = dive->number;
			break;
		case DATE:
			retVal = displayDate();
			break;
		case DEPTH:
			retVal = displayDepth();
			break;
		case DURATION:
			retVal = displayDuration();
			break;
		case TEMPERATURE:
			retVal = displayTemperature();
			break;
		case TOTALWEIGHT:
			retVal = displayWeight();
			break;
		case SUIT:
			retVal = QString(dive->suit);
			break;
		case CYLINDER:
			retVal = QString(dive->cylinder[0].type.description);
			break;
		case SAC:
			retVal = displaySac();
			break;
		case OTU:
			retVal = dive->otu;
			break;
		case MAXCNS:
			retVal = dive->maxcns;
			break;
		case LOCATION:
			retVal = QString(get_dive_location(dive));
			break;
		case GAS:
			const char *gas_string = get_dive_gas_string(dive);
			retVal = QString(gas_string);
			free((void*)gas_string);
			break;
		}
		break;
	case Qt::DecorationRole:
		if (column == LOCATION)
			if (dive_has_gps_location(dive)) {
				IconMetrics im = defaultIconMetrics();
				retVal = QIcon(":satellite").pixmap(im.sz_small, im.sz_small);
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
			retVal = tr("Temp(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
			break;
		case TOTALWEIGHT:
			retVal = tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
			break;
		case SUIT:
			retVal = tr("Suit");
			break;
		case CYLINDER:
			retVal = tr("Cyl");
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
			retVal = tr("Max CNS");
			break;
		case LOCATION:
			retVal = tr("Location");
			break;
		}
		break;
	}

	if (role == DiveTripModel::STAR_ROLE) {
		Q_ASSERT(dive != NULL);
		retVal = dive->rating;
	}
	if (role == DiveTripModel::DIVE_ROLE) {
		retVal = QVariant::fromValue<void *>(dive);
	}
	if (role == DiveTripModel::DIVE_IDX) {
		Q_ASSERT(dive != NULL);
		retVal = get_divenr(dive);
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
	struct dive *d;
	for_each_dive (i, d) {
		if (d->number == v)
			return false;
	}
	d = get_dive_by_uniq_id(diveId);
	d->number = value.toInt();
	mark_divelist_changed(true);
	return true;
}

QString DiveItem::displayDate() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	return get_dive_date_string(dive->when);
}

QString DiveItem::displayDepth() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	return get_depth_string(dive->maxdepth);
}

QString DiveItem::displayDepthWithUnit() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	return get_depth_string(dive->maxdepth, true);
}

QString DiveItem::displayDuration() const
{
	int hrs, mins, fullmins, secs;
	struct dive *dive = get_dive_by_uniq_id(diveId);
	mins = (dive->duration.seconds + 59) / 60;
	fullmins = dive->duration.seconds / 60;
	secs = dive->duration.seconds - 60 * fullmins;
	hrs = mins / 60;
	mins -= hrs * 60;

	QString displayTime;
	if (hrs)
		displayTime = QString("%1:%2").arg(hrs).arg(mins, 2, 10, QChar('0'));
	else if (mins < 15 || dive->dc.divemode == FREEDIVE)
		displayTime = QString("%1m%2s").arg(fullmins).arg(secs, 2, 10, QChar('0'));
	else
		displayTime = QString("%1").arg(mins);
	return displayTime;
}

QString DiveItem::displayTemperature() const
{
	QString str;
	struct dive *dive = get_dive_by_uniq_id(diveId);
	if (!dive->watertemp.mkelvin)
		return str;
	if (get_units()->temperature == units::CELSIUS)
		str = QString::number(mkelvin_to_C(dive->watertemp.mkelvin), 'f', 1);
	else
		str = QString::number(mkelvin_to_F(dive->watertemp.mkelvin), 'f', 1);
	return str;
}

QString DiveItem::displaySac() const
{
	QString str;
	struct dive *dive = get_dive_by_uniq_id(diveId);
	if (dive->sac) {
		const char *unit;
		int decimal;
		double value = get_volume_units(dive->sac, &decimal, &unit);
		return QString::number(value, 'f', decimal);
	}
	return QString("");
}

QString DiveItem::displayWeight() const
{
	QString str = weight_string(weight());
	return str;
}

int DiveItem::weight() const
{
	struct dive *dive = get_dive_by_uniq_id(diveId);
	weight_t tw = { total_weight(dive) };
	return tw.grams;
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
			ret = tr("Temp");
			break;
		case TOTALWEIGHT:
			ret = tr("Weight");
			break;
		case SUIT:
			ret = tr("Suit");
			break;
		case CYLINDER:
			ret = tr("Cyl");
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
			ret = tr("Temp(%1%2)").arg(UTF8_DEGREE).arg((get_units()->temperature == units::CELSIUS) ? "C" : "F");
			break;
		case TOTALWEIGHT:
			ret = tr("Weight(%1)").arg((get_units()->weight == units::KG) ? tr("kg") : tr("lbs"));
			break;
		case SUIT:
			ret = tr("Suit");
			break;
		case CYLINDER:
			ret = tr("Cyl");
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

	if (rowCount()) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		endRemoveRows();
	}

	if (autogroup)
		autogroup_dives();
	dive_table.preexisting = dive_table.nr;
	while (--i >= 0) {
		struct dive *dive = get_dive(i);
		update_cylinder_related_info(dive);
		dive_trip_t *trip = dive->divetrip;

		DiveItem *diveItem = new DiveItem();
		diveItem->diveId = dive->id;

		if (!trip || currentLayout == LIST) {
			diveItem->parent = rootItem;
			rootItem->children.push_back(diveItem);
			continue;
		}
		if (currentLayout == LIST)
			continue;

		if (!trips.keys().contains(trip)) {
			TripItem *tripItem = new TripItem();
			tripItem->trip = trip;
			tripItem->parent = rootItem;
			tripItem->children.push_back(diveItem);
			trips[trip] = tripItem;
			rootItem->children.push_back(tripItem);
			continue;
		}
		TripItem *tripItem = trips[trip];
		tripItem->children.push_back(diveItem);
	}

	if (rowCount()) {
		beginInsertRows(QModelIndex(), 0, rowCount() - 1);
		endInsertRows();
	}
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
