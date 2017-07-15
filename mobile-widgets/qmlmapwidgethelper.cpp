#include <QDebug>
#include "qmlmapwidgethelper.h"

MapWidgetHelper::MapWidgetHelper(QObject *parent) : QObject(parent)
{
}

void MapWidgetHelper::test()
{
	qDebug() << "test";
}
