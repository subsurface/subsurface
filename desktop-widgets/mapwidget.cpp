// SPDX-License-Identifier: GPL-2.0
#include <QQmlContext>
#include <QDebug>
#include <QQuickItem>
#include <QModelIndex>

#include "mapwidget.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "mobile-widgets/qmlmapwidgethelper.h"
#include "qt-models/maplocationmodel.h"
#include "mainwindow.h"
#include "divelistview.h"

static bool skipReload = false;

MapWidget *MapWidget::m_instance = NULL;

MapWidget::MapWidget(QWidget *parent) : QQuickWidget(parent)
{
	qmlRegisterType<MapWidgetHelper>("org.subsurfacedivelog.mobile", 1, 0, "MapWidgetHelper");
	qmlRegisterType<MapLocationModel>("org.subsurfacedivelog.mobile", 1, 0, "MapLocationModel");
	qmlRegisterType<MapLocation>("org.subsurfacedivelog.mobile", 1, 0, "MapLocation");

	connect(this, &QQuickWidget::statusChanged, this, &MapWidget::doneLoading);
	setSource(QUrl(QStringLiteral("qrc:/MapWidget.qml")));
}

void MapWidget::doneLoading(QQuickWidget::Status status)
{
	if (status != QQuickWidget::Ready) {
		qDebug() << "MapWidget status" << status;
		return;
	}
	qDebug() << "MapWidget ready";
	setResizeMode(QQuickWidget::SizeRootObjectToView);

	m_rootItem = qobject_cast<QQuickItem *>(rootObject());
	m_mapHelper = rootObject()->findChild<MapWidgetHelper *>();
	connect(m_mapHelper, SIGNAL(selectedDivesChanged(QList<int>)),
		this, SLOT(selectedDivesChanged(QList<int>)));
	connect(m_mapHelper, SIGNAL(coordinatesChanged()),
		this, SLOT(coordinatesChangedLocal()));
}

void MapWidget::centerOnDiveSite(struct dive_site *ds)
{
	if (!skipReload)
		m_mapHelper->centerOnDiveSite(ds);
}

void MapWidget::centerOnIndex(const QModelIndex& idx)
{
	struct dive_site *ds = get_dive_site_by_uuid(idx.model()->index(idx.row(), 0).data().toInt());
	if (!ds || !dive_site_has_gps_location(ds))
		centerOnDiveSite(&displayed_dive_site);
	else
		centerOnDiveSite(ds);
}

void MapWidget::repopulateLabels()
{
	m_mapHelper->reloadMapLocations();
}

void MapWidget::reload()
{
	setEditMode(false);
	if (!skipReload)
		m_mapHelper->reloadMapLocations();
}

void MapWidget::setEditMode(bool editMode)
{
	m_mapHelper->setEditMode(editMode);
}

void MapWidget::endGetDiveCoordinates()
{
	setEditMode(false);
}

void MapWidget::prepareForGetDiveCoordinates()
{
	setEditMode(true);
}

void MapWidget::selectedDivesChanged(QList<int> list)
{
	skipReload = true;
	MainWindow::instance()->dive_list()->unselectDives();
	if (!list.empty())
		MainWindow::instance()->dive_list()->selectDives(list);
	skipReload = false;
}

void MapWidget::coordinatesChangedLocal()
{
	emit coordinatesChanged();
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
