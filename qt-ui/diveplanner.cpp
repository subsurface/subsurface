#include "diveplanner.h"
#include <QMouseEvent>
#include <QDebug>

DivePlanner* DivePlanner::instance()
{
	static DivePlanner *self = new DivePlanner();
	return self;
}

DivePlanner::DivePlanner(QWidget* parent): QGraphicsView(parent)
{
	setMouseTracking(true);
	setScene( new QGraphicsScene());
	scene()->setSceneRect(0,0,100,100);

	verticalLine = new QGraphicsLineItem(0,0,0, 100);
	verticalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(verticalLine);

	horizontalLine = new QGraphicsLineItem(0,0,100,0);
	horizontalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(horizontalLine);
}

void DivePlanner::mouseDoubleClickEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if(isPointOutOfBoundaries(mappedPos))
		return;

	if(handles.count() && handles.last()->x() > mappedPos.x()){
		return;
	}

	QGraphicsEllipseItem *item = new QGraphicsEllipseItem(-5,-5,10,10);
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);


	item->setPos( mappedPos );
    scene()->addItem(item);
	handles << item;

	if (lines.empty()){
		QGraphicsLineItem *first = new QGraphicsLineItem(0,0, mappedPos.x(), mappedPos.y());
		lines.push_back(first);
		create_deco_stop();
		scene()->addItem(first);
	}else{
		clear_generated_deco();
		QGraphicsEllipseItem *prevHandle = handles.at( handles.count()-2);
		QGraphicsLineItem *line = new QGraphicsLineItem(prevHandle->x(), prevHandle->y(), item->x(), item->y());
		lines.push_back(line);
		scene()->addItem(line);
		create_deco_stop();
	}
}

void DivePlanner::clear_generated_deco()
{
	for(int i = handles.count(); i <= lines.count(); i++){
		scene()->removeItem(lines.last());
		delete lines.last();
		lines.removeLast();
	}
}

void DivePlanner::create_deco_stop()
{
	// this needs to create everything
	// for the calculated deco.
	QGraphicsLineItem *item = new QGraphicsLineItem(handles.last()->x(), handles.last()->y(), 100, 0);
	scene()->addItem(item);
	lines << item;
}

void DivePlanner::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void DivePlanner::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);
	fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void DivePlanner::mouseMoveEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	verticalLine->setLine(mappedPos.x(), 0, mappedPos.x(), 100);
	horizontalLine->setLine(0, mappedPos.y(), 100, mappedPos.y());

	if (!handles.count())
		return;

	if (handles.last()->x() > mappedPos.x()){
		verticalLine->setPen( QPen(QBrush(Qt::red), 0, Qt::SolidLine));
		horizontalLine->setPen( QPen(QBrush(Qt::red), 0, Qt::SolidLine));
	}else{
		verticalLine->setPen(QPen(Qt::DotLine));
		horizontalLine->setPen(QPen(Qt::DotLine));
	}
}

bool DivePlanner::isPointOutOfBoundaries(QPointF point)
{
	if (point.x() > sceneRect().width()
	||  point.x() < 0
	||  point.y() < 0
	|| point.y() > sceneRect().height())
	{
		return true;
	}
	return false;
}
