#include "divelistmodel.h"
#include "helpers.h"

Dive::Dive(dive *d)
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

QString Dive::date() const
{
	return m_date;
}

void Dive::setDate(const QString &date)
{
	m_date = date;
}
QString Dive::location() const
{
	return m_location;
}

void Dive::setLocation(const QString &location)
{
	m_location = location;
}
QString Dive::sac() const
{
	return m_sac;
}

void Dive::setSac(const QString &sac)
{
	m_sac = sac;
}
QString Dive::gas() const
{
	return m_gas;
}

void Dive::setGas(const QString &gas)
{
	m_gas = gas;
}
QString Dive::cylinder() const
{
	return m_cylinder;
}

void Dive::setCylinder(const QString &cylinder)
{
	m_cylinder = cylinder;
}
QString Dive::suit() const
{
	return m_suit;
}

void Dive::setSuit(const QString &suit)
{
	m_suit = suit;
}
QString Dive::weight() const
{
	return m_weight;
}

void Dive::setWeight(const QString &weight)
{
	m_weight = weight;
}
QString Dive::airtemp() const
{
	return m_airtemp;
}

void Dive::setAirTemp(const QString &airtemp)
{
	m_airtemp = airtemp;
}
QString Dive::duration() const
{
	return m_duration;
}

void Dive::setDuration(const QString &duration)
{
	m_duration = duration;
}
QString Dive::depth() const
{
	return m_depth;
}

void Dive::setDepth(const QString &depth)
{
	m_depth = depth;
}
QString Dive::rating() const
{
	return m_rating;
}

void Dive::setRating(const QString &rating)
{
	m_rating = rating;
}
dive *Dive::thisDive() const
{
	return m_thisDive;
}

void Dive::setThisDive(dive *thisDive)
{
	m_thisDive = thisDive;
}
QString Dive::diveNumber() const
{
	return m_diveNumber;
}

void Dive::setDiveNumber(const QString &diveNumber)
{
	m_diveNumber = diveNumber;
}
QString Dive::notes() const
{
	return m_notes;
}

void Dive::setNotes(const QString &notes)
{
	m_notes = notes;
}
QString Dive::trip() const
{
	return m_trip;
}

void Dive::setTrip(const QString &trip)
{
	m_trip = trip;
}
QString Dive::buddy() const
{
    return m_buddy;
}

void Dive::setBuddy(const QString &buddy)
{
    m_buddy = buddy;
}
QString Dive::divemaster() const
{
    return m_divemaster;
}

void Dive::setDivemaster(const QString &divemaster)
{
    m_divemaster = divemaster;
}
QString Dive::watertemp() const
{
	return m_watertemp;
}

void Dive::setWatertemp(const QString &watertemp)
{
	m_watertemp = watertemp;
}

void Dive::setupDiveTempDetails()
{
	const char *unit;
	double d_airTemp, d_waterTemp;

	d_airTemp = get_temp_units(m_thisDive->airtemp.mkelvin, &unit);
	d_waterTemp = get_temp_units(m_thisDive->watertemp.mkelvin, &unit);

	setAirTemp(QString::number(d_airTemp) + unit);
	setWatertemp(QString::number(d_waterTemp) + unit);

	if (!m_thisDive->airtemp.mkelvin)
		setAirTemp("");

	if (!m_thisDive->watertemp.mkelvin)
		setWatertemp("");
}






DiveListModel *DiveListModel::m_instance = NULL;

DiveListModel::DiveListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_instance = this;
}

void DiveListModel::addDive(dive *d)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_dives.append(Dive(d));
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

	const Dive &dive = m_dives[index.row()];

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
