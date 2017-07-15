#include <QQmlContext>
#include <QDebug>
#include <QQuickItem>
#include <QModelIndex>

#include "mapwidget.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "mobile-widgets/qmlmapwidgethelper.h"

MapWidget *MapWidget::m_instance = NULL;

MapWidget::MapWidget(QWidget *parent) : QQuickWidget(parent)
{
	qmlRegisterType<MapWidgetHelper>("org.subsurfacedivelog.mobile", 1, 0, "MapWidgetHelper");

	setSource(QUrl(QStringLiteral("qrc:/mapwidget-qml")));
	setResizeMode(QQuickWidget::SizeRootObjectToView);

	m_rootItem = qobject_cast<QQuickItem *>(rootObject());
}

void MapWidget::centerOnDiveSite(struct dive_site *ds)
{
	if (!dive_site_has_gps_location(ds))
		return;

	qreal longitude = ds->longitude.udeg / 1000000.0;
	qreal latitude = ds->latitude.udeg / 1000000.0;

	qDebug() << longitude << latitude;
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
	// TODO;
}

void MapWidget::reload()
{
	// TODO;
}

void MapWidget::endGetDiveCoordinates()
{
	// TODO;
}

void MapWidget::prepareForGetDiveCoordinates()
{
	// TODO;
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
