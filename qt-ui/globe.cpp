#include "globe.h"
#include "kmessagewidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../dive.h"
#include "../divelist.h"
#include "../helpers.h"

#include <QDebug>
#include <QTimer>

#include <marble/AbstractFloatItem.h>
#include <marble/GeoDataPlacemark.h>
#include <marble/GeoDataDocument.h>
#include <marble/MarbleModel.h>
#include <marble/MarbleDirs.h>
#include <marble/MapThemeManager.h>
#include <marble/GeoDataLineString.h>
#if INCOMPLETE_MARBLE
#include "marble/GeoDataTreeModel.h"
#else
#include <marble/GeoDataTreeModel.h>
#endif
#include <QMouseEvent>
#include <QMessageBox>

GlobeGPS::GlobeGPS(QWidget* parent) : MarbleWidget(parent), loadedDives(0), editingDiveLocation(false)
{
	// check if Google Sat Maps are installed
	// if not, check if they are in a known location
	MapThemeManager mtm;
	QStringList list = mtm.mapThemeIds();
	QString  subsurfaceDataPath;
	QDir marble;
	if (!list.contains("earth/googlesat/googlesat.dgml")) {
		subsurfaceDataPath = getSubsurfaceDataPath("marbledata");
		if (subsurfaceDataPath != "") {
			MarbleDirs::setMarbleDataPath(subsurfaceDataPath);
		} else {
			subsurfaceDataPath = getSubsurfaceDataPath("data");
			if (subsurfaceDataPath != "")
				MarbleDirs::setMarbleDataPath(subsurfaceDataPath);
		}
	}
	messageWidget = new KMessageWidget(this);
	messageWidget->setCloseButtonVisible(false);
	messageWidget->setHidden(true);

	setMapThemeId("earth/googlesat/googlesat.dgml");
	//setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
	setProjection(Marble::Spherical);

	setAnimationsEnabled(true);
	Q_FOREACH(AbstractFloatItem *i, floatItems()) {
		i->setVisible(false);
	}

	setShowClouds(false);
	setShowBorders(false);
	setShowPlaces(true);
	setShowCrosshairs(false);
	setShowGrid(false);
	setShowOverviewMap(false);
	setShowScaleBar(true);
	setShowCompass(false);
	connect(this, SIGNAL(mouseClickGeoPosition(qreal, qreal, GeoDataCoordinates::Unit)),
			this, SLOT(mouseClicked(qreal, qreal, GeoDataCoordinates::Unit)));

	setMinimumHeight(0);
	setMinimumWidth(0);
	fixZoomTimer = new QTimer();
	connect(fixZoomTimer, SIGNAL(timeout()), this, SLOT(fixZoom()));
	fixZoomTimer->setSingleShot(true);
	installEventFilter(this);
}

bool GlobeGPS::eventFilter(QObject *obj, QEvent *ev)
{
	// This disables Zooming when a double click occours on the scene.
	if (ev->type() == QEvent::MouseButtonDblClick && !editingDiveLocation)
		return true;
	// This disables the Marble's Context Menu
	// we need to move this to our 'contextMenuEvent'
	// if we plan to do a different one in the future.
	if (ev->type() == QEvent::ContextMenu) {
		contextMenuEvent(static_cast<QContextMenuEvent*>(ev));
		return true;
	}
	if (ev->type() == QEvent::MouseButtonPress) {
		QMouseEvent *e = static_cast<QMouseEvent*>(ev);
		 if (e->button() == Qt::RightButton)
			return true;
	}
	return QObject::eventFilter(obj,ev );
}

void GlobeGPS::contextMenuEvent(QContextMenuEvent* ev)
{
	QMenu m;
	QAction *a = m.addAction(tr("Edit Selected Dive Locations"), this, SLOT(prepareForGetDiveCoordinates()));
	a->setData(QVariant::fromValue<void*>(&m));
	m.exec(ev->globalPos());
}

void GlobeGPS::mouseClicked(qreal lon, qreal lat, GeoDataCoordinates::Unit unit)
{
	// don't mess with the selection while the user is editing a dive
	if (mainWindow()->information()->isEditing())
		return;

	GeoDataCoordinates here(lon, lat, unit);
	long lon_udeg = rint(1000000 * here.longitude(GeoDataCoordinates::Degree));
	long lat_udeg = rint(1000000 * here.latitude(GeoDataCoordinates::Degree));

	// distance() is in km above the map.
	// We're going to use that to decide how
	// approximate the dives have to be.
	//
	// Totally arbitrarily I say that 1km
	// distance means that we can resolve
	// to about 100m. Which in turn is about
	// 1000 udeg.
	//
	// Trigonometry is hard, but sin x == x
	// for small x, so let's just do this as
	// a linear thing.
	long resolve = rint(distance() * 1000);

	int idx;
	struct dive *dive;
	bool clear = !(QApplication::keyboardModifiers() & Qt::ControlModifier);
	QList<int> selectedDiveIds;
	for_each_dive(idx, dive) {
		long lat_diff, lon_diff;
		if (!dive_has_gps_location(dive))
			continue;
		lat_diff = labs(dive->latitude.udeg - lat_udeg);
		lon_diff = labs(dive->longitude.udeg - lon_udeg);
		if (lat_diff > 180000000)
			lat_diff = 360000000 - lat_diff;
		if (lon_diff > 180000000)
			lon_diff = 180000000 - lon_diff;
		if (lat_diff > resolve || lon_diff > resolve)
			continue;

		selectedDiveIds.push_back(idx);
	}
	if (selectedDiveIds.empty())
		return;
	if (clear) {
		mainWindow()->dive_list()->unselectDives();
		clear = false;
	}
	mainWindow()->dive_list()->selectDives(selectedDiveIds);
}

void GlobeGPS::repopulateLabels()
{
	if (loadedDives) {
		model()->treeModel()->removeDocument(loadedDives);
		delete loadedDives;
	}
	loadedDives = new GeoDataDocument;
	QMap<QString, GeoDataPlacemark *> locationMap;

	int idx = 0;
	struct dive *dive;
	for_each_dive(idx, dive) {
		if (dive_has_gps_location(dive)) {
			GeoDataPlacemark *place = new GeoDataPlacemark(dive->location);
			place->setCoordinate(dive->longitude.udeg / 1000000.0,dive->latitude.udeg / 1000000.0 , 0, GeoDataCoordinates::Degree);
			// don't add dive locations twice, unless they are at least 50m apart
			if (locationMap[QString(dive->location)]) {
				GeoDataCoordinates existingLocation = locationMap[QString(dive->location)]->coordinate();
				GeoDataLineString segment = GeoDataLineString();
				segment.append(existingLocation);
				GeoDataCoordinates newLocation = place->coordinate();
				segment.append(newLocation);
				double dist = segment.length(6371);
				// the dist is scaled to the radius given - so with 6371km as radius
				// 50m turns into 0.05 as threashold
				if (dist < 0.05)
					continue;
			}
			locationMap[QString(dive->location)] = place;
			loadedDives->append(place);
		}
	}
	model()->treeModel()->addDocument(loadedDives);
}

void GlobeGPS::reload()
{
	editingDiveLocation = false;
	if (messageWidget->isVisible())
		messageWidget->hide();
	repopulateLabels();
}

void GlobeGPS::centerOn(dive* dive)
{
	// dive has changed, if we had the 'editingDive', hide it.
	if (messageWidget->isVisible() && (!dive || dive_has_gps_location(dive)))
		messageWidget->hide();
	if (!dive)
		return;
	qreal longitude = dive->longitude.udeg / 1000000.0;
	qreal latitude = dive->latitude.udeg / 1000000.0;

	if (!longitude || !latitude || mainWindow()->information()->isEditing()) {
		prepareForGetDiveCoordinates();
		return;
	}

	// set the zoom as seen from n kilometer above. 3km / 10,000ft seems pleasant
	// do not change it it was already modified by user
	if (!zoom())
		zoomView(zoomFromDistance(3));

	if (!fixZoomTimer->isActive())
	    currentZoomLevel = zoom();
	// From the marble source code, the maximum time of
	// 'spin and fit' is 2 seconds, so wait a bit them zoom again.
	fixZoomTimer->start(2100);

	centerOn(longitude,latitude, true);
}

void GlobeGPS::fixZoom()
{
	zoomView(currentZoomLevel, Marble::Linear);
}

void GlobeGPS::prepareForGetDiveCoordinates()
{
	if (!messageWidget->isVisible()) {
		messageWidget->setMessageType(KMessageWidget::Warning);
		messageWidget->setText(QObject::tr("Move the map and double-click to set the dive location"));
		messageWidget->setWordWrap(true);
		messageWidget->animatedShow();
		editingDiveLocation = true;
	}
}

void GlobeGPS::changeDiveGeoPosition(qreal lon, qreal lat, GeoDataCoordinates::Unit unit)
{
	messageWidget->hide();

	if (mainWindow()->dive_list()->selectionModel()->selection().isEmpty())
		return;

	// convert to degrees if in radian.
	if (unit == GeoDataCoordinates::Radian) {
		lon = lon * 180 / M_PI;
		lat = lat * 180 / M_PI;
	}

	/* change everything on the selection. */
	int i;
	struct dive* dive;
	for_each_dive(i, dive) {
		if (!dive->selected)
			continue;
		dive->latitude.udeg = lrint(lat * 1000000.0);
		dive->longitude.udeg = lrint(lon * 1000000.0);
	}
	centerOn(lon, lat, true);
	editingDiveLocation = false;
	mark_divelist_changed(true);
	mainWindow()->refreshDisplay();
}

void GlobeGPS::mousePressEvent(QMouseEvent* event)
{
	qreal lat, lon;
	bool clickOnGlobe = geoCoordinates(event->pos().x(), event->pos().y(), lon, lat, GeoDataCoordinates::Degree);

	// there could be two scenarios that got us here; let's check if we are editing a dive
	if (mainWindow()->information()->isEditing() && clickOnGlobe) {
		mainWindow()->information()->updateCoordinatesText(lat, lon);
		repopulateLabels();
	} else if (clickOnGlobe) {
		changeDiveGeoPosition(lon, lat, GeoDataCoordinates::Degree);
	}
}

void GlobeGPS::resizeEvent(QResizeEvent* event)
{
	int size  = event->size().width();
	MarbleWidget::resizeEvent(event);
	if (size > 600)
		messageWidget->setGeometry((size - 600) / 2, 5, 600, 0);
	else
		messageWidget->setGeometry(5, 5, size - 10, 0);
	messageWidget->setMaximumHeight(500);
}
