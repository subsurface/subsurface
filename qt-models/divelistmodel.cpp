#include "divelistmodel.h"
#include "helpers.h"

MobileDive::MobileDive(dive *d)
{
	m_thisDive = d;
	setDiveNumber(QString::number(d->number));

	dive_trip *trip = d->divetrip;

	if(trip) {
		//trip is valid
		setTrip(trip->location);
	}

	setDate(get_dive_date_string(d->when));
	setDepth(get_depth_string(d->maxdepth));
	setDuration(get_dive_duration_string(d->duration.seconds, "h:","min"));

	setupDiveTempDetails();

	weight_t tw = { total_weight(d) };
	setWeight(weight_string(tw.grams));

	setSuit(QString(d->suit));
	setCylinder(QString(d->cylinder[0].type.description));
	setSac(QString::number(d->sac));
	setLocation(get_dive_location(d));
	setNotes(d->notes);
	setBuddy(d->buddy);
	setDivemaster(d->divemaster);
}

QString MobileDive::date() const
{
	return m_date;
}

void MobileDive::setDate(const QString &date)
{
	m_date = date;
}
QString MobileDive::location() const
{
	return m_location;
}

void MobileDive::setLocation(const QString &location)
{
	m_location = location;
}
QString MobileDive::sac() const
{
	return m_sac;
}

void MobileDive::setSac(const QString &sac)
{
	m_sac = sac;
}
QString MobileDive::gas() const
{
	return m_gas;
}

void MobileDive::setGas(const QString &gas)
{
	m_gas = gas;
}
QString MobileDive::cylinder() const
{
	return m_cylinder;
}

void MobileDive::setCylinder(const QString &cylinder)
{
	m_cylinder = cylinder;
}
QString MobileDive::suit() const
{
	return m_suit;
}

void MobileDive::setSuit(const QString &suit)
{
	m_suit = suit;
}
QString MobileDive::weight() const
{
	return m_weight;
}

void MobileDive::setWeight(const QString &weight)
{
	m_weight = weight;
}
QString MobileDive::airtemp() const
{
	return m_airtemp;
}

void MobileDive::setAirTemp(const QString &airtemp)
{
	m_airtemp = airtemp;
}
QString MobileDive::duration() const
{
	return m_duration;
}

void MobileDive::setDuration(const QString &duration)
{
	m_duration = duration;
}
QString MobileDive::depth() const
{
	return m_depth;
}

void MobileDive::setDepth(const QString &depth)
{
	m_depth = depth;
}
QString MobileDive::rating() const
{
	return m_rating;
}

void MobileDive::setRating(const QString &rating)
{
	m_rating = rating;
}
dive *MobileDive::thisDive() const
{
	return m_thisDive;
}

void MobileDive::setThisDive(dive *thisDive)
{
	m_thisDive = thisDive;
}
QString MobileDive::diveNumber() const
{
	return m_diveNumber;
}

void MobileDive::setDiveNumber(const QString &diveNumber)
{
	m_diveNumber = diveNumber;
}
QString MobileDive::notes() const
{
	return m_notes;
}

void MobileDive::setNotes(const QString &notes)
{
	m_notes = notes;
}
QString MobileDive::trip() const
{
	return m_trip;
}

void MobileDive::setTrip(const QString &trip)
{
	m_trip = trip;
}
QString MobileDive::buddy() const
{
    return m_buddy;
}

void MobileDive::setBuddy(const QString &buddy)
{
    m_buddy = buddy;
}
QString MobileDive::divemaster() const
{
    return m_divemaster;
}

void MobileDive::setDivemaster(const QString &divemaster)
{
    m_divemaster = divemaster;
}
QString MobileDive::watertemp() const
{
	return m_watertemp;
}

void MobileDive::setWatertemp(const QString &watertemp)
{
	m_watertemp = watertemp;
}

void MobileDive::setupDiveTempDetails()
{
	setWatertemp(get_temperature_string(m_thisDive->watertemp, true));
	setAirTemp(get_temperature_string(m_thisDive->airtemp, true));
}






DiveListModel *DiveListModel::m_instance = NULL;

DiveListModel::DiveListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_instance = this;
}

void DiveListModel::addDive(dive *d)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_dives.append(MobileDive(d));
	endInsertRows();
}

int DiveListModel::rowCount(const QModelIndex &) const
{
	return m_dives.count();
}

QVariant DiveListModel::data(const QModelIndex &index, int role) const
{
	if(index.row() < 0 || index.row() > m_dives.count())
		return QVariant();

	const MobileDive &dive = m_dives[index.row()];

	if (role == DiveNumberRole)
		return dive.diveNumber();
	else if (role == DiveTripRole)
		return dive.trip();
	else if (role == DiveDateRole)
		return dive.date();
	else if (role == DiveRatingRole)
		return dive.rating();
	else if (role == DiveDepthRole)
		return dive.depth();
	else if (role == DiveDurationRole)
		return dive.duration();
	else if (role == DiveAirTemperatureRole)
		return dive.airtemp();
	else if (role == DiveWaterTemperatureRole)
		return dive.watertemp();
	else if (role == DiveWeightRole)
		return dive.weight();
	else if (role == DiveSuitRole)
		return dive.suit();
	else if (role == DiveCylinderRole)
		return dive.cylinder();
	else if (role == DiveGasRole)
		return dive.gas();
	else if (role == DiveSacRole)
		return dive.sac();
	else if (role == DiveLocationRole)
		return dive.location();
	else if (role == DiveNotesRole)
		return dive.notes();
	else if (role == DiveBuddyRole)
		return dive.buddy();
	else if (role == DiveMasterRole)
		return dive.divemaster();
	return QVariant();


}

QHash<int, QByteArray> DiveListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[DiveNumberRole] = "diveNumber";
	roles[DiveTripRole] = "trip";
	roles[DiveDateRole] = "date";
	roles[DiveRatingRole] = "rating";
	roles[DiveDepthRole] = "depth";
	roles[DiveDurationRole] = "duration";
	roles[DiveAirTemperatureRole] = "airtemp";
	roles[DiveWaterTemperatureRole] = "watertemp";
	roles[DiveWeightRole] = "weight";
	roles[DiveSuitRole] = "suit";
	roles[DiveCylinderRole] = "cylinder";
	roles[DiveGasRole] = "gas";
	roles[DiveSacRole] = "sac";
	roles[DiveLocationRole] = "location";
	roles[DiveNotesRole] = "notes";
	roles[DiveBuddyRole] = "buddy";
	roles[DiveMasterRole] = "divemaster";

	return roles;
}

DiveListModel *DiveListModel::instance()
{
	return m_instance;
}
