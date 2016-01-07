#ifndef GPSLISTMODEL_H
#define GPSLISTMODEL_H

#include "gpslocation.h"
#include <QObject>
#include <QAbstractListModel>

class GpsTracker
{
private:
	quint64 m_when;
	qint32 m_latitude;
	qint32 m_longitude;
	QString m_name;

public:
	GpsTracker(struct gpsTracker *gt)
	{
		m_when = gt->when;
		m_latitude = gt->latitude.udeg;
		m_longitude = gt->longitude.udeg;
		m_name = gt->name;
	}
	GpsTracker();
	~GpsTracker();
	uint64_t when() const;
	int32_t latitude() const;
	int32_t longitude() const;
	QString name() const;
};

class GpsListModel : public QAbstractListModel
{
	Q_OBJECT
public:

	enum GpsListRoles {
		GpsDateRole = Qt::UserRole + 1,
		GpsNameRole,
		GpsLatitudeRole,
		GpsLongitudeRole
	};

	static GpsListModel *instance();
	GpsListModel(QObject *parent = 0);
	void addGpsFix(struct gpsTracker *g);
	void clear();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QHash<int, QByteArray> roleNames() const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
	QList<GpsTracker> m_gpsFixes;
	static GpsListModel *m_instance;
};

#endif // GPSLISTMODEL_H
