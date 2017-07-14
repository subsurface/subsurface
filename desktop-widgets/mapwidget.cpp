#include <QQmlContext>
#include <QDebug>
#include <QQuickItem>

#include "mapwidget.h"
#include "core/dive.h"
#include "core/divesite.h"

MapWidget *MapWidget::m_instance = NULL;

MapWidget::MapWidget(QWidget *parent) : QQuickWidget(parent)
{
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
