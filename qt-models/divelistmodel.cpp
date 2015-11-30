#include "divelistmodel.h"
#include "helpers.h"

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

void DiveListModel::clear()
{
	beginRemoveRows(QModelIndex(), 0, m_dives.count() - 1);
	m_dives.clear();
	endRemoveRows();
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
		return dive.number();
	else if (role == DiveTripRole)
		return dive.trip();
	else if (role == DiveDateRole)
		return (qlonglong)dive.timestamp();
	else if (role == DiveDateStringRole)
		return dive.date() + " " + dive.time();
	else if (role == DiveRatingRole)
		return QString::number(dive.rating());
	else if (role == DiveDepthRole)
		return dive.depth();
	else if (role == DiveDurationRole)
		return dive.duration();
	else if (role == DiveAirTemperatureRole)
		return dive.airTemp();
	else if (role == DiveWaterTemperatureRole)
		return dive.waterTemp();
	else if (role == DiveWeightRole)
		return dive.weight(0);
	else if (role == DiveSuitRole)
		return dive.suit();
	else if (role == DiveCylinderRole)
		return dive.cylinder(0);
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
	else if (role == DiveIdRole)
		return QString::number(dive.id());
	return QVariant();


}

QHash<int, QByteArray> DiveListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[DiveNumberRole] = "diveNumber";
	roles[DiveTripRole] = "trip";
	roles[DiveDateStringRole] = "date";
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
	roles[DiveIdRole] = "id";

	return roles;
}

void DiveListModel::startAddDive()
{
	struct dive *d;
	d = alloc_dive();
	add_single_dive(get_divenr(d), d);
	addDive(d);
}

DiveListModel *DiveListModel::instance()
{
	return m_instance;
}
