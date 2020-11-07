// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELOCATIONMODEL_H
#define DIVELOCATIONMODEL_H

#include <QAbstractTableModel>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include "core/units.h"

#define RECENTLY_ADDED_DIVESITE ((struct dive_site *)~0)

struct dive;
struct dive_trip;

class LocationInformationModel : public QAbstractTableModel {
	Q_OBJECT
public:
	// Common columns, roles and accessor function for all dive-site models.
	// Thus, different views can connect to different models.
	enum Columns { EDIT, REMOVE, NAME, DESCRIPTION, NUM_DIVES, LOCATION, NOTES, DIVESITE, TAXONOMY, COLUMNS };
	enum Roles { DIVESITE_ROLE = Qt::UserRole + 1 };
	static QVariant getDiveSiteData(const struct dive_site *ds, int column, int role);

	LocationInformationModel(QObject *obj = 0);
	static LocationInformationModel *instance();
	int columnCount(const QModelIndex &parent) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index = QModelIndex(), int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

public slots:
	void update();
private slots:
	void diveSiteDiveCountChanged(struct dive_site *ds);
	void diveSiteAdded(struct dive_site *ds, int idx);
	void diveSiteDeleted(struct dive_site *ds, int idx);
	void diveSiteChanged(struct dive_site *ds, int field);
	void diveSiteDivesChanged(struct dive_site *ds);
};

class DiveSiteSortedModel : public QSortFilterProxyModel {
	Q_OBJECT
private:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const override;
	bool lessThan(const QModelIndex &i1, const QModelIndex &i2) const override;
	QString fullText;
#ifndef SUBSURFACE_MOBILE
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
#endif // SUBSURFACE_MOBILE
public:
	DiveSiteSortedModel(QObject *parent = nullptr);
	QStringList allSiteNames() const;
	void setFilter(const QString &text);
	struct dive_site *getDiveSite(const QModelIndex &idx);
};

// To access only divesites at the given GPS coordinates with the exception of a given dive site
class GPSLocationInformationModel : public QSortFilterProxyModel {
	Q_OBJECT
private:
	const struct dive_site *ignoreDs;
	location_t location;
	int64_t distance;
	bool filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const override;
public:
	GPSLocationInformationModel(QObject *parent = nullptr);
	void set(const struct dive_site *ignoreDs, const location_t &);
	void setCoordinates(const location_t &);
	void setDistance(int64_t dist); // Distance from coordinates in mm
};

class GeoReferencingOptionsModel : public QStringListModel {
	Q_OBJECT
public:
	static GeoReferencingOptionsModel *instance();
private:
	GeoReferencingOptionsModel(QObject *parent = 0);
};

#endif
