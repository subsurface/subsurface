#include "gpslistmodel.h"
#include "helpers.h"

GpsTracker::GpsTracker()
{
	m_latitude = 0;
	m_longitude = 0;
	m_when = 0;
	m_name = QString();
}

GpsTracker::~GpsTracker()
{
}

uint64_t GpsTracker::when() const
{
	return m_when;
}

int32_t GpsTracker::latitude() const
{
	return m_latitude;
}

int32_t GpsTracker::longitude() const
{
	return m_longitude;
}

QString GpsTracker::name() const
{
	return m_name;
}

GpsListModel *GpsListModel::m_instance = NULL;

GpsListModel::GpsListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_instance = this;
}

void GpsListModel::addGpsFix(gpsTracker *g)
{
	beginInsertColumns(QModelIndex(), rowCount(), rowCount());
	m_gpsFixes.append(GpsTracker(g));
	endInsertRows();
}

void GpsListModel::clear()
{
	if (m_gpsFixes.count()) {
		beginRemoveRows(QModelIndex(), 0, m_gpsFixes.count() - 1);
		m_gpsFixes.clear();
		endRemoveRows();
	}
}

int GpsListModel::rowCount(const QModelIndex &parent) const
{
	return m_gpsFixes.count();
}

QVariant GpsListModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() > m_gpsFixes.count())
		return QVariant();

	const GpsTracker &gt = m_gpsFixes[index.row()];

	if (role == GpsDateRole)
		return get_short_dive_date_string(gt.when());
	else if (role == GpsNameRole)
		return QString(gt.name());
	else if (role == GpsLatitudeRole)
		return QString::number(gt.latitude() / 1000000.0, 'f', 6);
	else if (role == GpsLongitudeRole)
		return QString::number(gt.longitude() / 1000000.0, 'f', 6);
	return QVariant();
}

QHash<int, QByteArray> GpsListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[GpsDateRole] = "when";
	roles[GpsNameRole] = "name";
	roles[GpsLatitudeRole] = "latitude";
	roles[GpsLongitudeRole] = "longitude";
	return roles;
}

GpsListModel *GpsListModel::instance()
{
	return m_instance;
}

