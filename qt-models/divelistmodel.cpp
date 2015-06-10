#include "divelistmodel.h"
#include "helpers.h"

Dive::Dive(dive *d)
{
	m_thisDive = d;
	setDiveNumber(QString::number(d->number));
	setDate(get_dive_date_string(d->when));
	setDepth(get_depth_string(d->maxdepth));
	setDuration(get_dive_duration_string(d->duration.seconds, "h:","min"));

	if (!d->watertemp.mkelvin)
		m_depth = "";

	if (get_units()->temperature == units::CELSIUS)
		m_depth = QString::number(mkelvin_to_C(d->watertemp.mkelvin), 'f', 1);
	else
		m_depth = QString::number(mkelvin_to_F(d->watertemp.mkelvin), 'f', 1);

	weight_t tw = { total_weight(d) };
	setWeight(weight_string(tw.grams));

	setSuit(QString(d->suit));
	setCylinder(QString(d->cylinder[0].type.description));
	setSac(QString::number(d->sac));
	setLocation(get_dive_location(d));
	setNotes(d->notes);
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
QString Dive::temp() const
{
	return m_temp;
}

void Dive::setTemp(const QString &temp)
{
	m_temp = temp;
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
	else if (role == DiveDateRole)
		return dive.date();
	else if (role == DiveRatingRole)
		return dive.rating();
	else if (role == DiveDepthRole)
		return dive.depth();
	else if (role == DiveDurationRole)
		return dive.duration();
	else if (role == DiveTemperatureRole)
		return dive.temp();
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
	return QVariant();


}

QHash<int, QByteArray> DiveListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[DiveNumberRole] = "diveNumber";
	roles[DiveDateRole] = "date";
	roles[DiveRatingRole] = "rating";
	roles[DiveDepthRole] = "depth";
	roles[DiveDurationRole] = "duration";
	roles[DiveTemperatureRole] = "temp";
	roles[DiveWeightRole] = "weight";
	roles[DiveSuitRole] = "suit";
	roles[DiveCylinderRole] = "cylinder";
	roles[DiveGasRole] = "gas";
	roles[DiveSacRole] = "sac";
	roles[DiveLocationRole] = "location";
	roles[DiveNotesRole] = "notes";

	return roles;
}

DiveListModel *DiveListModel::instance()
{
	return m_instance;
}
