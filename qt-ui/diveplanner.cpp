#include "diveplanner.h"
#include <QMouseEvent>
#include <QDebug>
#include "ui_diveplanner.h"

DivePlannerGraphics::DivePlannerGraphics(QWidget* parent): QGraphicsView(parent), activeDraggedHandler(0)
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

	timeLine = new Ruler();
	timeLine->setMinimum(0);
	timeLine->setMaximum(60);
	timeLine->setTickInterval(10);
	timeLine->setLine( 10, 90, 99, 90);
	timeLine->setOrientation(Qt::Horizontal);
	timeLine->updateTicks();
	scene()->addItem(timeLine);

	depthLine = new Ruler();
	depthLine->setMinimum(0);
	depthLine->setMaximum(400);
	depthLine->setTickInterval(10);
	depthLine->setLine( 10, 1, 10, 90);
	depthLine->setOrientation(Qt::Vertical);
	depthLine->updateTicks();
	scene()->addItem(depthLine);

	timeString = new QGraphicsSimpleTextItem();
	timeString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	scene()->addItem(timeString);

	depthString = new QGraphicsSimpleTextItem();
	depthString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	scene()->addItem(depthString);
}

void DivePlannerGraphics::mouseDoubleClickEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if(isPointOutOfBoundaries(mappedPos))
		return;

	if(handles.count() && handles.last()->x() > mappedPos.x()){
		return;
	}

	DiveHandler  *item = new DiveHandler ();
	item->setRect(-5,-5,10,10);
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	item->setPos( mappedPos );
    scene()->addItem(item);
	handles << item;

	if (lines.empty()){
		QGraphicsLineItem *first = new QGraphicsLineItem(0,0, mappedPos.x(), mappedPos.y());
		item->from = first;
		lines.push_back(first);
		create_deco_stop();
		scene()->addItem(first);
	}else{
		clear_generated_deco();
		DiveHandler *prevHandle = handles.at( handles.count()-2);
		QGraphicsLineItem *line = new QGraphicsLineItem(prevHandle->x(), prevHandle->y(), item->x(), item->y());
		prevHandle->to = line;
		item->from = line;
		lines.push_back(line);
		scene()->addItem(line);
		create_deco_stop();
	}
	item->time = (timeLine->valueAt(mappedPos));
	item->depth = (depthLine->valueAt(mappedPos));
}

void DivePlannerGraphics::clear_generated_deco()
{
	for(int i = handles.count(); i <= lines.count(); i++){
		scene()->removeItem(lines.last());
		delete lines.last();
		lines.removeLast();
	}
}

void DivePlannerGraphics::create_deco_stop()
{
	// This needs to be done in the following steps:
	// Get the user-input and calculate the dive info
	Q_FOREACH(DiveHandler *h, handles){
		// use this somewhere.
		h->time;
		h->depth;
	}
	// create the dive info here.

	// set the new 'end time' of the dive.
	// note that this is not the user end,
	// but the real end of the dive.
	timeLine->setMaximum(60);
	timeLine->updateTicks();

	// Re-position the user generated dive handlers
	Q_FOREACH(DiveHandler *h, handles){
	// uncomment this as soon as the posAtValue is implemented.
	// h->setPos( timeLine->posAtValue(h->time),
	// 	   depthLine->posAtValue(h->depth));
	}

	// Create all 'deco' GraphicsLineItems and put it on the canvas.This following three lines will
	// most probably need to enter on a loop.
	QGraphicsLineItem *item = new QGraphicsLineItem(handles.last()->x(), handles.last()->y(), 100, 0);
	scene()->addItem(item);
	lines << item;
}

void DivePlannerGraphics::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void DivePlannerGraphics::showEvent(QShowEvent* event)
{
    QGraphicsView::showEvent(event);
	fitInView(sceneRect(), Qt::KeepAspectRatio);
}

void DivePlannerGraphics::mouseMoveEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	verticalLine->setLine(mappedPos.x(), 0, mappedPos.x(), 100);
	horizontalLine->setLine(0, mappedPos.y(), 100, mappedPos.y());
	depthString->setText(QString::number(depthLine->valueAt(mappedPos)));
	depthString->setPos(0, mappedPos.y());
	timeString->setText(QString::number( (int) timeLine->valueAt(mappedPos)) + "min");
	timeString->setPos(mappedPos.x()+1, 90);

	if(activeDraggedHandler)
		moveActiveHandler(mappedPos);
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

void DivePlannerGraphics::moveActiveHandler(QPointF pos)
{
	int idx = handles.indexOf(activeDraggedHandler);
	bool moveLines = false;;
	// do not allow it to move between handlers.
	if (handles.count() > 1){
		if (idx == 0 ){ // first
			if (pos.x() < handles[1]->x()){
				activeDraggedHandler->setPos(pos);
				moveLines = true;
			}
		}else if (idx == handles.count()-1){ // last
			if (pos.x() > handles[idx-1]->x()){
				activeDraggedHandler->setPos(pos);
				moveLines = true;
			}
		}else{ // middle
			if (pos.x() > handles[idx-1]->x() && pos.x() < handles[idx+1]->x()){
				activeDraggedHandler->setPos(pos);
				moveLines = true;
			}
		}
	}else{
		activeDraggedHandler->setPos(pos);
		moveLines = true;
	}
	if (moveLines){
		if (activeDraggedHandler->from){
			QLineF f = activeDraggedHandler->from->line();
			activeDraggedHandler->from->setLine(f.x1(), f.y1(), pos.x(), pos.y());
		}

		if (activeDraggedHandler->to){
			QLineF f = activeDraggedHandler->to->line();
			activeDraggedHandler->to->setLine(pos.x(), pos.y(), f.x2(), f.y2());
		}

		if(activeDraggedHandler == handles.last()){
			clear_generated_deco();
			create_deco_stop();
		}
	}
}

bool DivePlannerGraphics::isPointOutOfBoundaries(QPointF point)
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

void DivePlannerGraphics::mousePressEvent(QMouseEvent* event)
{
    QPointF mappedPos = mapToScene(event->pos());
	Q_FOREACH(QGraphicsItem *item, scene()->items(mappedPos)){
		if (DiveHandler *h = qgraphicsitem_cast<DiveHandler*>(item)){
			activeDraggedHandler = h;
			activeDraggedHandler->setBrush(Qt::red);
		}
	}
}

void DivePlannerGraphics::mouseReleaseEvent(QMouseEvent* event)
{
	if (activeDraggedHandler){
		QPointF mappedPos = mapToScene(event->pos());
		activeDraggedHandler->time = (timeLine->valueAt(mappedPos));
		activeDraggedHandler->depth = (depthLine->valueAt(mappedPos));
		activeDraggedHandler->setBrush(QBrush());
		activeDraggedHandler = 0;
	}
}

DiveHandler::DiveHandler(): QGraphicsEllipseItem(), from(0), to(0)
{
}

void Ruler::setMaximum(double maximum)
{
	max = maximum;
}

void Ruler::setMinimum(double minimum)
{
	min = minimum;
}

Ruler::Ruler() : orientation(Qt::Horizontal)
{
}

void Ruler::setOrientation(Qt::Orientation o)
{
	orientation = o;
}

void Ruler::updateTicks()
{
	qDeleteAll(ticks);
	QLineF m = line();
	if(orientation == Qt::Horizontal){
		double steps = (max - min) / interval;
		double stepSize = (m.x2() - m.x1()) / steps;
		for(qreal pos = m.x1(); pos < m.x2(); pos += stepSize){
			QGraphicsLineItem *l = new QGraphicsLineItem(pos, m.y1(), pos, m.y1() + 1, this);

		}
	}else{
		double steps = (max - min) / interval;
		double stepSize = (m.y2() - m.y1()) / steps;
		for(qreal pos = m.y1(); pos < m.y2(); pos += stepSize){
			QGraphicsLineItem *l = new QGraphicsLineItem(m.x1(), pos, m.x1() - 1, pos, this);
		}
	}
}

void Ruler::setTickInterval(double i)
{
	interval = i;
}

qreal Ruler::valueAt(const QPointF& p)
{
	QLineF m = line();
	double retValue =  orientation == Qt::Horizontal
		? max * (p.x() - m.x1()) / (m.x2() - m.x1())
		: max * (p.y() - m.y1()) / (m.y2() - m.y1());
	return retValue;
}

qreal Ruler::posAtValue(qreal value)
{
	QLineF m = line();
	double size = max - min;
	double percent = value / size;
	double realSize = orientation == Qt::Horizontal
		? m.x2() - m.x1()
		: m.y2() - m.y1();
	double retValue = realSize * percent;
	retValue =  (orientation == Qt::Horizontal)
		? retValue + m.x1()
		: retValue + m.y1();
	return retValue;
}

DivePlanner::DivePlanner() : ui(new Ui::DivePlanner())
{
	ui->setupUi(this);
}

struct dive* DivePlanner::getDive()
{
	return 0;
}

DivePlanner* DivePlanner::instance()
{
	static DivePlanner *self = new DivePlanner();
	return self;
}
