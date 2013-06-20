#include "diveplanner.h"
#include <QMouseEvent>

DivePlanner* DivePlanner::instance()
{
	static DivePlanner *self = new DivePlanner();
	return self;
}

DivePlanner::DivePlanner(QWidget* parent): QGraphicsView(parent)
{
	setScene( new QGraphicsScene());
	scene()->setSceneRect(0,0,100,100);
}

void DivePlanner::mouseDoubleClickEvent(QMouseEvent* event)
{
	QGraphicsEllipseItem *item = new QGraphicsEllipseItem(-10,-10,20,20);
	item->setPos( mapToScene(event->pos()));
    scene()->addItem(item);
}

