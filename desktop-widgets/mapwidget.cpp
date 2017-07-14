#include <QQmlContext>
#include <QDebug>
#include <QQuickItem>

#include "mapwidget.h"

MapWidget *MapWidget::m_instance = NULL;

MapWidget::MapWidget(QWidget *parent) : QQuickWidget(parent)
{
	setSource(QUrl(QStringLiteral("qrc:/mapwidget-qml")));
	setResizeMode(QQuickWidget::SizeRootObjectToView);

	m_rootItem = qobject_cast<QQuickItem *>(rootObject());
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
