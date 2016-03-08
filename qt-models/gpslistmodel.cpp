#include "gpslistmodel.h"
#include "helpers.h"
#include <QVector>

GpsListModel *GpsListModel::m_instance = NULL;

GpsListModel::GpsListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_instance = this;
}

void GpsListModel::addGpsFix(gpsTracker g)
{
	beginInsertColumns(QModelIndex(), rowCount(), rowCount());
	m_gpsFixes.append(g);
	endInsertRows();
}

void GpsListModel::update()
{
	QVector<gpsTracker> trackers = QVector<gpsTracker>::fromList(GpsLocation::instance()->currentGPSInfo().values());
	beginResetModel();
	m_gpsFixes = trackers;
	endResetModel();
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
	Q_UNUSED(parent);
	return m_gpsFixes.count();
}

QVariant GpsListModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() > m_gpsFixes.count())
		return QVariant();

	const gpsTracker &gt = m_gpsFixes[index.row()];

	if (role == GpsDateRole)
		return get_short_dive_date_string(gt.when);
	else if (role == GpsWhenRole)
		return gt.when;
	else if (role == GpsNameRole)
		return gt.name;
	else if (role == GpsLatitudeRole)
		return QString::number(gt.latitude.udeg / 1000000.0, 'f', 6);
	else if (role == GpsLongitudeRole)
		return QString::number(gt.longitude.udeg / 1000000.0, 'f', 6);
	return QVariant();
}

QHash<int, QByteArray> GpsListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[GpsDateRole] = "date";
	roles[GpsWhenRole] = "when";
	roles[GpsNameRole] = "name";
	roles[GpsLatitudeRole] = "latitude";
	roles[GpsLongitudeRole] = "longitude";
	return roles;
}

GpsListModel *GpsListModel::instance()
{
	return m_instance;
}

