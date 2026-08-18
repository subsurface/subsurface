// SPDX-License-Identifier: GPL-2.0
#include "maplocationmodel.h"
#include "divelocationmodel.h"
#include "core/divesite.h"
#include "core/divefilter.h"
#include "core/divelog.h"
#include "core/range.h"
#include "core/settings/qPrefDisplay.h"
#if !defined(SUBSURFACE_MOBILE) && !defined(SUBSURFACE_DOWNLOADER)
#include "qt-models/filtermodels.h"
#include "desktop-widgets/mapwidget.h"
#endif
#include <map>
#include <QRegularExpression>

// AI-generated (Claude)
// Some dive computer downloads (see core/libdivecomputer.cpp) name a dive
// site after its own GPS coordinates (e.g. "12.345678, -6.789012") when the
// device provides a GPS fix but no site name. Such a name carries no real
// identity -- it is effectively unnamed -- so for map dedup purposes treat
// it the same as an empty name.
static bool looksLikeCoordinates(const QString &name)
{
	static const QRegularExpression re(QStringLiteral("^-?\\d{1,3}\\.\\d+,\\s*-?\\d{1,3}\\.\\d+$"));
	return re.match(name).hasMatch();
}

// MKW If "Map Short Names" preference is set, only return the last component
// of the full dive site name.
// Example:
// Japan/Izu Peninsula/Atami/Chinsen-Aft
//    Short name: Chinsen-Aft
static QString siteMapDisplayName(const std::string &sitename)
{
	const char Separator = '/';
	QString fullname = QString::fromStdString(sitename);

	if (!qPrefDisplay::map_short_names() )
		return fullname;

	QString name = fullname.section(Separator, -1).trimmed();
	if (name.isEmpty())
		name = std::move(fullname);
	return name;
}


MapLocation::MapLocation(struct dive_site *dsIn, QGeoCoordinate coordIn, QString nameIn, bool selectedIn) :
    divesite(dsIn), coordinate(coordIn), name(nameIn), selected(selectedIn)
{
}

// Check whether we are in divesite-edit mode. This doesn't
// exist on mobile. And on desktop we have to access the MapWidget.
// Simplify this!
static bool inEditMode()
{
#if defined(SUBSURFACE_MOBILE) || defined(SUBSURFACE_DOWNLOADER)
	return false;
#else
	return MapWidget::instance()->editMode();
#endif
}

QVariant MapLocation::getRole(int role) const
{
	switch (role) {
	case RoleDivesite:
		return QVariant::fromValue(divesite);
	case RoleCoordinate:
		return QVariant::fromValue(coordinate);
	case RoleName:
		return QVariant::fromValue(name);
	case RolePixmap:
		return selected ? QString("qrc:///dive-location-marker-selected-icon") :
		       inEditMode() ? QString("qrc:///dive-location-marker-inactive-icon") :
				    QString("qrc:///dive-location-marker-icon");
	case RoleZ:
		return selected ? 1 : 0;
	case RoleIsSelected:
		return QVariant::fromValue(selected);
	default:
		return QVariant();
	}
}

MapLocationModel::MapLocationModel(QObject *parent) : QAbstractListModel(parent)
{
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &MapLocationModel::diveSiteChanged);
}

MapLocationModel::~MapLocationModel()
{
}

QVariant MapLocationModel::data(const QModelIndex & index, int role) const
{
	if (index.row() < 0 || index.row() >= (int)m_mapLocations.size())
		return QVariant();

	return m_mapLocations[index.row()].getRole(role);
}

QHash<int, QByteArray> MapLocationModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[MapLocation::RoleDivesite] = "divesite";
	roles[MapLocation::RoleCoordinate] = "coordinate";
	roles[MapLocation::RoleName] = "name";
	roles[MapLocation::RolePixmap] = "pixmap";
	roles[MapLocation::RoleZ] = "z";
	roles[MapLocation::RoleIsSelected] = "isSelected";
	return roles;
}

int MapLocationModel::rowCount(const QModelIndex&) const
{
	return (int)m_mapLocations.size();
}

/* UNUSED?
void MapLocationModel::add(MapLocation *location)
{
	beginInsertRows(QModelIndex(), (int)m_mapLocations.size(), (int)m_mapLocations.size());
	m_mapLocations.append(location);
	endInsertRows();
}
*/

const std::vector<dive_site *> &MapLocationModel::selectedDs() const
{
	return m_selectedDs;
}

static bool hasVisibleDive(const dive_site &ds)
{
	return std::any_of(ds.dives.begin(), ds.dives.end(),
			   [] (const dive *d) { return !d->hidden_by_filter; });
}

static bool hasSelectedDive(const dive_site &ds)
{
	return std::any_of(ds.dives.begin(), ds.dives.end(),
			   [] (const dive *d) { return d->selected; });
}

void MapLocationModel::selectionChanged()
{
	if (m_mapLocations.empty())
		return;
	for(MapLocation &m: m_mapLocations)
		m.selected = range_contains(m_selectedDs, m.divesite);
	emit dataChanged(createIndex(0, 0), createIndex((int)m_mapLocations.size() - 1, 0));
}

void MapLocationModel::reload(QObject *map)
{
	beginResetModel();

	m_mapLocations.clear();
	m_selectedDs.clear();

	std::map<QString, size_t> locationNameMap;

#if defined(SUBSURFACE_MOBILE) || defined(SUBSURFACE_DOWNLOADER)
	bool diveSiteMode = false;
#else
	// In dive site mode (that is when either editing a dive site or on
	// the dive site tab), we want to show all dive sites, not only those
	// of the non-hidden dives. Moreover, the selected dive sites are those
	// that we filter for.
	bool diveSiteMode = DiveFilter::instance()->diveSiteMode();
#endif
	if (diveSiteMode) {
#if !defined(SUBSURFACE_MOBILE) && !defined(SUBSURFACE_DOWNLOADER)
		m_selectedDs = DiveFilter::instance()->filteredDiveSites();
#endif
	} else {
		// Determine the full set of selected dive sites up front, so that
		// "only show selected dive site" (below) knows about all of them,
		// not just the ones encountered so far in the loop.
		for (const auto &ds: divelog.sites) {
			if (hasVisibleDive(*ds) && hasSelectedDive(*ds))
				m_selectedDs.push_back(ds.get());
		}
	}

	// AI-generated (Claude)
	// Optionally hide every dive site except the selected one(s), or restrict
	// to sites within a given radius of a selected site.
	bool onlySelected = !diveSiteMode && qPrefDisplay::map_show_only_selected_dive_site();
	double radius_m = qPrefDisplay::map_selected_dive_site_radius() * 1000.0;
	std::vector<QGeoCoordinate> selectedCoords;
	if (onlySelected) {
		for (const dive_site *sel: m_selectedDs) {
			if (sel->has_gps_location())
				selectedCoords.emplace_back(sel->location.lat.udeg * 0.000001, sel->location.lon.udeg * 0.000001);
		}
	}

	for (const auto &ds: divelog.sites) {
		QGeoCoordinate dsCoord;

		// Don't show dive sites of hidden dives, unless we're in dive site edit mode.
		if (!diveSiteMode && !hasVisibleDive(*ds))
			continue;
		if (!ds->has_gps_location()) {
			// Dive sites that do not have a gps location are not shown in normal mode.
			// In dive-edit mode, selected sites are placed at the center of the map,
			// so that the user can drag them somewhere without having to enter coordinates.
			if (!diveSiteMode || !range_contains(m_selectedDs, ds.get()) || !map)
				continue;
			dsCoord = map->property("center").value<QGeoCoordinate>();
		} else {
			qreal latitude = ds->location.lat.udeg * 0.000001;
			qreal longitude = ds->location.lon.udeg * 0.000001;
			dsCoord = QGeoCoordinate(latitude, longitude);
		}
		QString name = siteMapDisplayName(ds->name);
		bool isSelected = range_contains(m_selectedDs, ds.get());
		if (onlySelected && !isSelected) {
			bool withinRadius = radius_m > 0 &&
				std::any_of(selectedCoords.begin(), selectedCoords.end(),
					    [&dsCoord, radius_m](const QGeoCoordinate &c) { return dsCoord.distanceTo(c) <= radius_m; });
			if (!withinRadius)
				continue;
		}
		if (!diveSiteMode && !isSelected && qPrefDisplay::map_dedup_nearby_sites()) {
			// Named sites only merge with another site of the exact same
			// name within map_dedup_distance() meters, so two distinct,
			// deliberately named sites are never accidentally combined.
			// Unnamed sites (no name given by the user, or a name that is
			// just the site's own GPS coordinates) have no identity worth
			// preserving, so they merge into any nearby marker, named or
			// not, within the same distance. Never skip the currently
			// selected dive site, or its marker would be missing from the
			// map. The toggle and the distance are user preferences.
			// (AI-generated (Claude))
			if (name.isEmpty() || looksLikeCoordinates(name)) {
				bool mergedIntoExisting = std::any_of(m_mapLocations.begin(), m_mapLocations.end(),
					[&dsCoord](const MapLocation &existingLocation) {
						return dsCoord.distanceTo(existingLocation.coordinate) < qPrefDisplay::map_dedup_distance();
					});
				if (mergedIntoExisting)
					continue;
			} else {
				auto it = locationNameMap.find(name);
				if (it != locationNameMap.end()) {
					const MapLocation &existingLocation = m_mapLocations[it->second];
					QGeoCoordinate coord = existingLocation.coordinate;
					if (dsCoord.distanceTo(coord) < qPrefDisplay::map_dedup_distance())
						continue;
				}
			}
		}
		m_mapLocations.emplace_back(ds.get(), dsCoord, name, isSelected);
		if (!diveSiteMode)
			locationNameMap[name] = m_mapLocations.size() - 1;
	}

	endResetModel();
}

void MapLocationModel::setSelected(struct dive_site *ds)
{
	m_selectedDs.clear();
	if (ds)
		m_selectedDs.push_back(ds);
}

void MapLocationModel::setSelected(const std::vector<dive_site *> &divesites)
{
	m_selectedDs = divesites;
}

MapLocation *MapLocationModel::getMapLocation(const struct dive_site *ds)
{
	for (MapLocation &location: m_mapLocations) {
		if (location.divesite == ds)
			return &location;
	}
	return nullptr;
}

void MapLocationModel::diveSiteChanged(struct dive_site *ds, int field)
{
	// Find dive site
	auto it = std::find_if(m_mapLocations.begin(), m_mapLocations.end(),
			       [ds](auto &entry) { return entry.divesite == ds; });
	if (it == m_mapLocations.end())
		return;

	switch (field) {
	case LocationInformationModel::LOCATION:
		if (has_location(&ds->location)) {
			const qreal latitude_r = ds->location.lat.udeg * 0.000001;
			const qreal longitude_r = ds->location.lon.udeg * 0.000001;
			QGeoCoordinate coord(latitude_r, longitude_r);
			it->coordinate = coord;
		}
		break;
	case LocationInformationModel::NAME:
		it->name = siteMapDisplayName(ds->name);
		break;
	default:
		break;
	}

	int row = static_cast<int>(it - m_mapLocations.begin());
	emit dataChanged(createIndex(row, 0), createIndex(row, 0));
}
