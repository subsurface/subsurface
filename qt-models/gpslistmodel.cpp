// SPDX-License-Identifier: GPL-2.0
#include "qt-models/gpslistmodel.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/qthelper.h"
#include <QVector>

GpsListModel::GpsListModel()
{
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &GpsListModel::update);
}

void GpsListModel::update()
{
	GpsLocation *glp = GpsLocation::instance();
	if (!glp)
		return;
	QVector<gpsTracker> trackers = QVector<gpsTracker>::fromList(glp->currentGPSInfo().values());
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

int GpsListModel::rowCount(const QModelIndex&) const
{
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
		return QString::number(gt.location.lat.udeg / 1000000.0, 'f', 6);
	else if (role == GpsLongitudeRole)
		return QString::number(gt.location.lon.udeg / 1000000.0, 'f', 6);
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
	static GpsListModel self;
	return &self;
}
