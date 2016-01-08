#ifndef GPSLISTMODEL_H
#define GPSLISTMODEL_H

#include "gpslocation.h"
#include <QObject>
#include <QAbstractListModel>

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
	void addGpsFix(gpsTracker g);
	void clear();
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QHash<int, QByteArray> roleNames() const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	void update();
private:
	QVector<gpsTracker> m_gpsFixes;
	static GpsListModel *m_instance;
};

#endif // GPSLISTMODEL_H
