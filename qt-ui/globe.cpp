#include "globe.h"
#ifndef NO_MARBLE
#include "kmessagewidget.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dive.h"
#include "divelist.h"
#include "helpers.h"
#include "display.h"

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

// as of Marble 4.10 (which has MARBLE_VERSION 0x001000) zoomView is
// deprecated and has been replaced by setZoom with the same function signature
#if MARBLE_VERSION < 0x001000
#define setZoom zoomView
#endif

GlobeGPS::GlobeGPS(QWidget *parent) : MarbleWidget(parent),
	loadedDives(0),
	messageWidget(new KMessageWidget(this)),
	fixZoomTimer(new QTimer(this)),
	currentZoomLevel(0),
	needResetZoom(false),
	editingDiveLocation(false)
{
	// check if Google Sat Maps are installed
	// if not, check if they are in a known location
	MapThemeManager mtm;
	QStringList list = mtm.mapThemeIds();
	QString subsurfaceDataPath;
	QDir marble;
	if (!list.contains("earth/googlesat/googlesat.dgml")) {
		subsurfaceDataPath = getSubsurfaceDataPath("marbledata");
		if (subsurfaceDataPath.size()) {
			MarbleDirs::setMarbleDataPath(subsurfaceDataPath);
		} else {
			subsurfaceDataPath = getSubsurfaceDataPath("data");
			if (subsurfaceDataPath.size())
				MarbleDirs::setMarbleDataPath(subsurfaceDataPath);
		}
	}
	messageWidget->setCloseButtonVisible(false);
	messageWidget->setHidden(true);

	setMapThemeId("earth/googlesat/googlesat.dgml");
	//setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
	setProjection(Marble::Spherical);

	setAnimationsEnabled(true);
	Q_FOREACH (AbstractFloatItem *i, floatItems()) {
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
	connect(fixZoomTimer, SIGNAL(timeout()), this, SLOT(fixZoom()));
	fixZoomTimer->setSingleShot(true);
	installEventFilter(this);
}

bool GlobeGPS::eventFilter(QObject *obj, QEvent *ev)
{
	// sometimes Marble seems not to notice double clicks and consequently not call
	// the right callback - so let's remember here if the last 'click' is a 'double' or not
	enum QEvent::Type type = ev->type();
	if (type == QEvent::MouseButtonDblClick)
		doubleClick = true;
	else if (type == QEvent::MouseButtonPress)
		doubleClick = false;

	// This disables Zooming when a double click occours on the scene.
	if (type == QEvent::MouseButtonDblClick && !editingDiveLocation)
		return true;
	// This disables the Marble's Context Menu
	// we need to move this to our 'contextMenuEvent'
	// if we plan to do a different one in the future.
	if (type == QEvent::ContextMenu) {
		contextMenuEvent(static_cast<QContextMenuEvent *>(ev));
		return true;
	}
	if (type == QEvent::MouseButtonPress) {
		QMouseEvent *e = static_cast<QMouseEvent *>(ev);
		if (e->button() == Qt::RightButton)
			return true;
	}
	return QObject::eventFilter(obj, ev);
}

void GlobeGPS::contextMenuEvent(QContextMenuEvent *ev)
{
	QMenu m;
	QAction *a = m.addAction(tr("Edit selected dive locations"), this, SLOT(prepareForGetDiveCoordinates()));
	a->setData(QVariant::fromValue<void *>(&m));
	a->setEnabled(current_dive);
	m.exec(ev->globalPos());
}

void GlobeGPS::mouseClicked(qreal lon, qreal lat, GeoDataCoordinates::Unit unit)
{
	if (doubleClick) {
		// strangely sometimes we don't get the changeDiveGeoPosition callback
		// and end up here instead
		changeDiveGeoPosition(lon, lat, unit);
		return;
	}
	// don't mess with the selection while the user is editing a dive
	if (MainWindow::instance()->information()->isEditing() || messageWidget->isVisible())
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
	for_each_dive (idx, dive) {
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
	if (clear)
		MainWindow::instance()->dive_list()->unselectDives();
	MainWindow::instance()->dive_list()->selectDives(selectedDiveIds);
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
	for_each_dive (idx, dive) {
		if (dive_has_gps_location(dive)) {
			GeoDataPlacemark *place = new GeoDataPlacemark(dive->location);
			place->setCoordinate(dive->longitude.udeg / 1000000.0, dive->latitude.udeg / 1000000.0, 0, GeoDataCoordinates::Degree);
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
	messageWidget->hide();
	repopulateLabels();
}

void GlobeGPS::centerOnCurrentDive()
{
	struct dive *dive = current_dive;
	// dive has changed, if we had the 'editingDive', hide it.
	if (messageWidget->isVisible() && (!dive || dive_has_gps_location(dive) || amount_selected != 1))
		messageWidget->hide();

	editingDiveLocation = false;
	if (!dive)
		return;

	qreal longitude = dive->longitude.udeg / 1000000.0;
	qreal latitude = dive->latitude.udeg / 1000000.0;

	if ((!dive_has_gps_location(dive) || MainWindow::instance()->information()->isEditing()) && amount_selected == 1) {
		prepareForGetDiveCoordinates();
		return;
	}
	if (!dive_has_gps_location(dive)) {
		zoomOutForNoGPS();
		return;
	}

	// if no zoom is set up, set the zoom as seen from 3km above
	// if we come back from a dive without GPS data, reset to the last zoom value
	// otherwise check to make sure we aren't still running an animation and then remember
	// the current zoom level
	if (!zoom()) {
		currentZoomLevel = zoomFromDistance(3);
		fixZoom();
	} else if (needResetZoom) {
		needResetZoom = false;
		fixZoom();
	} else if (!fixZoomTimer->isActive())
		currentZoomLevel = zoom();
	// From the marble source code, the maximum time of
	// 'spin and fit' is 2 seconds, so wait a bit them zoom again.
	fixZoomTimer->start(2100);

	centerOn(longitude, latitude, true);
}

void GlobeGPS::fixZoom()
{
	setZoom(currentZoomLevel, Marble::Linear);
}

void GlobeGPS::zoomOutForNoGPS()
{
	// this is called if the dive has no GPS location.
	// zoom out quite a bit to show the globe and remember that the next time
	// we show a dive with GPS location we need to zoom in again
	if (fixZoomTimer->isActive())
		fixZoomTimer->stop();
	setZoom(1200, Marble::Automatic);
	if (!needResetZoom) {
		needResetZoom = true;
		currentZoomLevel = zoom();
	}
}

void GlobeGPS::prepareForGetDiveCoordinates()
{
	if (!messageWidget->isVisible()) {
		messageWidget->setMessageType(KMessageWidget::Warning);
		messageWidget->setText(QObject::tr("Move the map and double-click to set the dive location"));
		messageWidget->setWordWrap(true);
		messageWidget->setCloseButtonVisible(false);
		messageWidget->animatedShow();
		editingDiveLocation = true;
		if (!dive_has_gps_location(current_dive))
			zoomOutForNoGPS();
	}
}

void GlobeGPS::changeDiveGeoPosition(qreal lon, qreal lat, GeoDataCoordinates::Unit unit)
{
	messageWidget->hide();

	if (MainWindow::instance()->dive_list()->selectionModel()->selection().isEmpty())
		return;

	// convert to degrees if in radian.
	if (unit == GeoDataCoordinates::Radian) {
		lon = lon * 180 / M_PI;
		lat = lat * 180 / M_PI;
	}

	// right now we try to only ever do this with one dive selected,
	// but we keep the code here that changes the coordinates for each selected dive
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (!dive->selected)
			continue;
		dive->latitude.udeg = lrint(lat * 1000000.0);
		dive->longitude.udeg = lrint(lon * 1000000.0);
	}
	centerOn(lon, lat, true);
	editingDiveLocation = false;
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}

void GlobeGPS::mousePressEvent(QMouseEvent *event)
{
	if (event->type() != QEvent::MouseButtonDblClick)
		return;

	qreal lat, lon;
	bool clickOnGlobe = geoCoordinates(event->pos().x(), event->pos().y(), lon, lat, GeoDataCoordinates::Degree);

	// there could be two scenarios that got us here; let's check if we are editing a dive
	if (MainWindow::instance()->information()->isEditing() && clickOnGlobe) {
		MainWindow::instance()->information()->updateCoordinatesText(lat, lon);
		repopulateLabels();
	} else if (clickOnGlobe) {
		changeDiveGeoPosition(lon, lat, GeoDataCoordinates::Degree);
	}
}

void GlobeGPS::resizeEvent(QResizeEvent *event)
{
	int size = event->size().width();
	MarbleWidget::resizeEvent(event);
	if (size > 600)
		messageWidget->setGeometry((size - 600) / 2, 5, 600, 0);
	else
		messageWidget->setGeometry(5, 5, size - 10, 0);
	messageWidget->setMaximumHeight(500);
}
#else

GlobeGPS::GlobeGPS(QWidget *parent)
{
	setText("MARBLE DISABLED AT BUILD TIME");
}
void GlobeGPS::repopulateLabels()
{
}
void GlobeGPS::centerOnCurrentDive()
{
}
bool GlobeGPS::eventFilter(QObject *obj, QEvent *ev)
{
	return QObject::eventFilter(obj, ev);
}
void GlobeGPS::prepareForGetDiveCoordinates()
{
}
void GlobeGPS::reload()
{
}
#endif
