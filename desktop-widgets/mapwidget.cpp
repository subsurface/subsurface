// SPDX-License-Identifier: GPL-2.0
#include <QQmlContext>
#include <QDebug>
#include <QQuickItem>
#include <QModelIndex>

#include "mapwidget.h"
#include "core/divesite.h"
#include "map-widget/qmlmapwidgethelper.h"
#include "qt-models/maplocationmodel.h"
#include "mainwindow.h"
#include "divelistview.h"

static const QUrl urlMapWidget = QUrl(QStringLiteral("qrc:/qml/MapWidget.qml"));
static const QUrl urlMapWidgetError = QUrl(QStringLiteral("qrc:/qml/MapWidgetError.qml"));
static bool isReady = false;
static bool skipReload = false;

#define CHECK_IS_READY_RETURN_VOID() \
	if (!isReady) return

MapWidget *MapWidget::m_instance = NULL;

MapWidget::MapWidget(QWidget *parent) : QQuickWidget(parent)
{
	m_rootItem = Q_NULLPTR;
	m_mapHelper = Q_NULLPTR;
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	connect(this, &QQuickWidget::statusChanged, this, &MapWidget::doneLoading);
	setSource(urlMapWidget);
}

void MapWidget::doneLoading(QQuickWidget::Status status)
{
	// the default map widget QML failed; load the error QML.
	if (source() == urlMapWidget && status != QQuickWidget::Ready) {
		qDebug() << urlMapWidget << "failed to load with status:" << status;
		setSource(urlMapWidgetError);
		return;
	} else if (source() == urlMapWidgetError) { // the error QML finished loading.
		return;
	}

	isReady = true;
	m_rootItem = qobject_cast<QQuickItem *>(rootObject());
	m_mapHelper = rootObject()->findChild<MapWidgetHelper *>();
	connect(m_mapHelper, SIGNAL(selectedDivesChanged(QList<int>)),
		this, SLOT(selectedDivesChanged(QList<int>)));
	connect(m_mapHelper, &MapWidgetHelper::coordinatesChanged, this, &MapWidget::coordinatesChangedLocal);
}

void MapWidget::centerOnSelectedDiveSite()
{
	CHECK_IS_READY_RETURN_VOID();
	if (!skipReload)
		m_mapHelper->centerOnSelectedDiveSite();
}

void MapWidget::centerOnDiveSite(struct dive_site *ds)
{
	CHECK_IS_READY_RETURN_VOID();
	if (!skipReload)
		m_mapHelper->centerOnDiveSite(ds);
}

void MapWidget::centerOnIndex(const QModelIndex& idx)
{
	CHECK_IS_READY_RETURN_VOID();
	struct dive_site *ds = get_dive_site_by_uuid(idx.model()->index(idx.row(), 0).data().toUInt());
	if (!ds || !dive_site_has_gps_location(ds))
		centerOnSelectedDiveSite();
	else
		centerOnDiveSite(ds);
}

void MapWidget::repopulateLabels()
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->reloadMapLocations();
}

void MapWidget::reload()
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->exitEditMode();
	if (!skipReload)
		m_mapHelper->reloadMapLocations();
}

void MapWidget::endGetDiveCoordinates()
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->exitEditMode();
}

void MapWidget::prepareForGetDiveCoordinates(uint32_t uuid)
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->enterEditMode(uuid);
}

void MapWidget::selectedDivesChanged(QList<int> list)
{
	CHECK_IS_READY_RETURN_VOID();
	skipReload = true;
	MainWindow::instance()->diveList->unselectDives();
	if (!list.empty())
		MainWindow::instance()->diveList->selectDives(list);
	skipReload = false;
}

void MapWidget::coordinatesChangedLocal(const location_t &location)
{
	CHECK_IS_READY_RETURN_VOID();
	emit coordinatesChanged(location);
}

void MapWidget::updateDiveSiteCoordinates(uint32_t uuid, const location_t &location)
{
	CHECK_IS_READY_RETURN_VOID();
	m_mapHelper->updateDiveSiteCoordinates(uuid, location);
}

MapWidget::~MapWidget()
{
	m_instance = NULL;
}

MapWidget *MapWidget::instance()
{
	if (m_instance == NULL)
		m_instance = new MapWidget();
	return m_instance;
}
