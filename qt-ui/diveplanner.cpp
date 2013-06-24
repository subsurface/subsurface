#include "diveplanner.h"
#include "../dive.h"
#include <cmath>
#include <QMouseEvent>
#include <QDebug>
#include "ui_diveplanner.h"

DivePlannerGraphics::DivePlannerGraphics(QWidget* parent): QGraphicsView(parent), activeDraggedHandler(0)
{
	setMouseTracking(true);
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0,0,100,100);

	verticalLine = new QGraphicsLineItem(0,0,0, 100);
	verticalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(verticalLine);

	horizontalLine = new QGraphicsLineItem(0,0,100,0);
	horizontalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(horizontalLine);

	timeLine = new Ruler();
	timeLine->setMinimum(0);
	timeLine->setMaximum(20);
	timeLine->setTickInterval(10);
	timeLine->setLine(10, 90, 99, 90);
	timeLine->setOrientation(Qt::Horizontal);
	timeLine->updateTicks();
	scene()->addItem(timeLine);

	depthLine = new Ruler();
	depthLine->setMinimum(0);
	depthLine->setMaximum(10);
	depthLine->setTickInterval(10);
	depthLine->setLine(10, 1, 10, 90);
	depthLine->setOrientation(Qt::Vertical);
	depthLine->updateTicks();
	scene()->addItem(depthLine);

	timeString = new QGraphicsSimpleTextItem();
	timeString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	scene()->addItem(timeString);

	depthString = new QGraphicsSimpleTextItem();
	depthString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	scene()->addItem(depthString);

	plusDepth = new Button();
	plusDepth->setPos(15, 1);
	scene()->addItem(plusDepth);
	connect(plusDepth, SIGNAL(clicked()), this, SLOT(increaseDepth()));

	plusTime = new Button();
	plusTime->setPos(95, 90);
	scene()->addItem(plusTime);
	connect(plusTime, SIGNAL(clicked()), this, SLOT(increaseTime()));
}

void DivePlannerGraphics::increaseDepth()
{
	qDebug() << "Increase Depth Clicked";
}

void DivePlannerGraphics::increaseTime()
{
	qDebug() << "Increase Time Clicked";
}

void DivePlannerGraphics::mouseDoubleClickEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	if (handles.count() && handles.last()->x() > mappedPos.x())
		return;

	DiveHandler  *item = new DiveHandler ();
	item->setRect(-5,-5,10,10);
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	int minutes = rint(timeLine->valueAt(mappedPos));
	int meters = rint(depthLine->valueAt(mappedPos));
	item->sec = minutes * 60;
	item->mm =  meters * 1000;
	double xpos = timeLine->posAtValue(minutes);
	double ypos = depthLine->posAtValue(meters);
	item->setPos(QPointF(xpos, ypos));
	scene()->addItem(item);
	handles << item;

	if (lines.empty()) {
		double xpos = timeLine->posAtValue(0);
		double ypos = depthLine->posAtValue(0);
		QGraphicsLineItem *first = new QGraphicsLineItem(xpos,ypos, mappedPos.x(), mappedPos.y());
		item->from = first;
		lines.push_back(first);
		createDecoStops();
		scene()->addItem(first);
	} else {
		clearGeneratedDeco();
		DiveHandler *prevHandle = handles.at(handles.count()-2);
		QGraphicsLineItem *line = new QGraphicsLineItem(prevHandle->x(), prevHandle->y(), item->x(), item->y());
		prevHandle->to = line;
		item->from = line;
		lines.push_back(line);
		scene()->addItem(line);
		createDecoStops();
	}
	item->time = (timeLine->valueAt(mappedPos));
	item->depth = (depthLine->valueAt(mappedPos));
}

void DivePlannerGraphics::clearGeneratedDeco()
{
	for (int i = handles.count(); i <= lines.count(); i++) {
		scene()->removeItem(lines.last());
		delete lines.last();
		lines.removeLast();
	}
}

void DivePlannerGraphics::createDecoStops()
{
	// This needs to be done in the following steps:
	// Get the user-input and calculate the dive info
	// Not sure if this is the place to create the diveplan...
	// We just start with a surface node at time = 0
	struct diveplan plan;
	struct divedatapoint *dp = create_dp(0, 0, 209, 0, 0);
	dp->entered = TRUE;
	plan.dp = dp;
	DiveHandler *lastH = NULL;
	Q_FOREACH(DiveHandler *h, handles) {
		// these values need to come from the planner UI, eventually
		int o2 = 209;
		int he = 0;
		int po2 = 0;
		int deltaT = lastH ? h->sec - lastH->sec : h->sec;
		lastH = h;
		plan_add_segment(&plan, deltaT, h->mm, o2, he, po2);
		qDebug("time %d, depth %d", h->sec, h->mm);
	}
#if DEBUG_PLAN
	dump_plan(&plan);
#endif

	// create the dive info here.

	// set the new 'end time' of the dive.
	// note that this is not the user end,
	// but the real end of the dive.
	timeLine->setMaximum(60);
	timeLine->updateTicks();

	// Re-position the user generated dive handlers
	Q_FOREACH(DiveHandler *h, handles) {
	// uncomment this as soon as the posAtValue is implemented.
	// h->setPos(timeLine->posAtValue(h->time),
	// 	   depthLine->posAtValue(h->depth));
	}

	// Create all 'deco' GraphicsLineItems and put it on the canvas.This following three lines will
	// most probably need to enter on a loop.
	double xpos = timeLine->posAtValue(timeLine->maximum());
	double ypos = depthLine->posAtValue(depthLine->minimum());
	QGraphicsLineItem *item = new QGraphicsLineItem(handles.last()->x(), handles.last()->y(), xpos, ypos);
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
	depthString->setText(QString::number(rint(depthLine->valueAt(mappedPos))) + "m" );
	depthString->setPos(0, mappedPos.y());
	timeString->setText(QString::number(rint(timeLine->valueAt(mappedPos))) + "min");
	timeString->setPos(mappedPos.x()+1, 90);

	if (activeDraggedHandler)
		moveActiveHandler(mappedPos);
	if (!handles.count())
		return;

	if (handles.last()->x() > mappedPos.x()) {
		verticalLine->setPen(QPen(QBrush(Qt::red), 0, Qt::SolidLine));
		horizontalLine->setPen(QPen(QBrush(Qt::red), 0, Qt::SolidLine));
	} else {
		verticalLine->setPen(QPen(Qt::DotLine));
		horizontalLine->setPen(QPen(Qt::DotLine));
	}
}

void DivePlannerGraphics::moveActiveHandler(const QPointF& pos)
{
	int idx = handles.indexOf(activeDraggedHandler);

	double xpos = timeLine->posAtValue(rint(timeLine->valueAt(pos)));
	double ypos = depthLine->posAtValue(rint(depthLine->valueAt(pos)));
	QPointF newPos(xpos, ypos);
	int sec = rint(timeLine->valueAt(newPos)) * 60;
	int mm = rint(depthLine->valueAt(newPos)) * 1000;
	bool moveLines = false;;
	// do not allow it to move between handlers.
	if (handles.count() > 1) {
		if (idx == 0 ) { // first
			if (newPos.x() < handles[1]->x()) {
				activeDraggedHandler->setPos(newPos);
				moveLines = true;
			}
		} else if (idx == handles.count()-1) { // last
			if (newPos.x() > handles[idx-1]->x()) {
				activeDraggedHandler->setPos(newPos);
				moveLines = true;
			}
		} else { // middle
			if (newPos.x() > handles[idx-1]->x() && newPos.x() < handles[idx+1]->x()) {
				activeDraggedHandler->setPos(newPos);
				moveLines = true;
			}
		}
	} else {
		activeDraggedHandler->setPos(newPos);
		moveLines = true;
	}
	if (moveLines) {
		if (activeDraggedHandler->from) {
			QLineF f = activeDraggedHandler->from->line();
			activeDraggedHandler->from->setLine(f.x1(), f.y1(), newPos.x(), newPos.y());
		}

		if (activeDraggedHandler->to) {
			QLineF f = activeDraggedHandler->to->line();
			activeDraggedHandler->to->setLine(newPos.x(), newPos.y(), f.x2(), f.y2());
		}

		if (activeDraggedHandler == handles.last()) {
			clearGeneratedDeco();
			createDecoStops();
		}
		activeDraggedHandler->sec = sec;
		activeDraggedHandler->mm = mm;
	}
}

bool DivePlannerGraphics::isPointOutOfBoundaries(const QPointF& point)
{
	double xpos = timeLine->valueAt(point);
	double ypos = depthLine->valueAt(point);

	if (xpos > timeLine->maximum() ||
	    xpos < timeLine->minimum() ||
	    ypos > depthLine->maximum() ||
	    ypos < depthLine->minimum())
	{
		return true;
	}
	return false;
}

void DivePlannerGraphics::mousePressEvent(QMouseEvent* event)
{
    QPointF mappedPos = mapToScene(event->pos());
	Q_FOREACH(QGraphicsItem *item, scene()->items(mappedPos)) {
		if (DiveHandler *h = qgraphicsitem_cast<DiveHandler*>(item)) {
			activeDraggedHandler = h;
			activeDraggedHandler->setBrush(Qt::red);
		}
	}
	QGraphicsView::mousePressEvent(event);
}

void DivePlannerGraphics::mouseReleaseEvent(QMouseEvent* event)
{
	if (activeDraggedHandler) {
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
	ticks.clear();
	QLineF m = line();
	if (orientation == Qt::Horizontal) {
		double steps = (max - min) / interval;
		double stepSize = (m.x2() - m.x1()) / steps;
		for (qreal pos = m.x1(); pos < m.x2(); pos += stepSize) {
			ticks.push_back(new QGraphicsLineItem(pos, m.y1(), pos, m.y1() + 1, this));

		}
	} else {
		double steps = (max - min) / interval;
		double stepSize = (m.y2() - m.y1()) / steps;
		for (qreal pos = m.y1(); pos < m.y2(); pos += stepSize) {
			ticks.push_back(new QGraphicsLineItem(m.x1(), pos, m.x1() - 1, pos, this));
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
	double retValue =  orientation == Qt::Horizontal ?
				max * (p.x() - m.x1()) / (m.x2() - m.x1()) :
				max * (p.y() - m.y1()) / (m.y2() - m.y1());
	return retValue;
}

qreal Ruler::posAtValue(qreal value)
{
	QLineF m = line();
	double size = max - min;
	double percent = value / size;
	double realSize = orientation == Qt::Horizontal ?
				m.x2() - m.x1() :
				m.y2() - m.y1();
	double retValue = realSize * percent;
	retValue =  (orientation == Qt::Horizontal) ?
				retValue + m.x1() :
				retValue + m.y1();
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

double Ruler::maximum() const
{
	return max;
}

double Ruler::minimum() const
{
	return min;
}

Button::Button(QObject* parent): QObject(parent), QGraphicsPixmapItem()
{
	setPixmap(QPixmap(":plus").scaled(20,20));
	setFlag(ItemIgnoresTransformations);
}

void Button::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	emit clicked();
}
