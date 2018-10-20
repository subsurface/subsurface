// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELOCATIONMODEL_H
#define DIVELOCATIONMODEL_H

#include <QAbstractTableModel>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <stdint.h>
#include "core/units.h"

#define RECENTLY_ADDED_DIVESITE 1

class LocationInformationModel : public QAbstractTableModel {
Q_OBJECT
public:
	// Common columns, roles and accessor function for all dive-site models.
	// Thus, different views can connect to different models.
	enum Columns { UUID, NAME, LATITUDE, LONGITUDE, COORDS, DESCRIPTION, NOTES, TAXONOMY_1, TAXONOMY_2, TAXONOMY_3, COLUMNS};
	enum Roles { UUID_ROLE = Qt::UserRole + 1 };
	static QVariant getDiveSiteData(const struct dive_site *ds, int column, int role);

	LocationInformationModel(QObject *obj = 0);
	static LocationInformationModel *instance();
	int columnCount(const QModelIndex &parent) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const;
	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());

public slots:
	void update();
	QStringList allSiteNames() const;
private:
	QStringList locationNames;
};

// To access only divesites at the given GPS coordinates with the exception of a given dive site
class GPSLocationInformationModel : public QSortFilterProxyModel {
Q_OBJECT
private:
	uint32_t ignoreUuid;
	location_t location;
	bool filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const override;
public:
	GPSLocationInformationModel(QObject *parent = nullptr);
	void set(uint32_t ignoreUuid, const location_t &);
	void setCoordinates(const location_t &);
};

class GeoReferencingOptionsModel : public QStringListModel {
Q_OBJECT
public:
	static GeoReferencingOptionsModel *instance();
private:
	GeoReferencingOptionsModel(QObject *parent = 0);
};

#endif
