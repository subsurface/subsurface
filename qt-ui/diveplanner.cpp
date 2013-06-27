#include "diveplanner.h"
#include "../dive.h"
#include <cmath>
#include <QMouseEvent>
#include <QDebug>
#include "ui_diveplanner.h"

DivePlannerGraphics::DivePlannerGraphics(QWidget* parent): QGraphicsView(parent), activeDraggedHandler(0),
	lastValidPos(0.0, 0.0)
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
	depthLine->setMaximum(100);
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
	createDecoStops();
}

void DivePlannerGraphics::clearGeneratedDeco()
{
}

void DivePlannerGraphics::createDecoStops()
{
	qDeleteAll(lines);
	lines.clear();

	// This needs to be done in the following steps:
	// Get the user-input and calculate the dive info
	// Not sure if this is the place to create the diveplan...
	// We just start with a surface node at time = 0
	struct diveplan diveplan;
	struct divedatapoint *dp = create_dp(0, 0, 209, 0, 0);
	dp->entered = TRUE;
	diveplan.dp = dp;
	diveplan.gflow = 30;
	diveplan.gfhigh = 70;
	diveplan.surface_pressure = 1013;
	DiveHandler *lastH = NULL;
	Q_FOREACH(DiveHandler *h, handles) {
		// these values need to come from the planner UI, eventually
		int o2 = 209;
		int he = 0;
		int po2 = 0;
		int deltaT = lastH ? h->sec - lastH->sec : h->sec;
		lastH = h;
		dp = plan_add_segment(&diveplan, deltaT, h->mm, o2, he, po2);
		dp->entered = TRUE;
		qDebug("time %d, depth %d", h->sec, h->mm);
	}
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif
	char *cache = NULL;
	struct dive *dive = NULL;
	char *errorString = NULL;
	plan(&diveplan, &cache, &dive, &errorString);
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif

	while(dp->next)
		dp = dp->next;

	//if (timeLine->maximum() < dp->time / 60.0 + 5) {
		timeLine->setMaximum(dp->time / 60.0 + 5);
		timeLine->updateTicks();
	//}

	// Re-position the user generated dive handlers
	Q_FOREACH(DiveHandler *h, handles){
		h->setPos(timeLine->posAtValue(h->sec / 60), depthLine->posAtValue(h->mm) / 1000);
	}

	// (re-) create the profile with different colors for segments that were
	// entered vs. segments that were calculated
	double lastx = timeLine->posAtValue(0);
	double lasty = depthLine->posAtValue(0);
	for (dp = diveplan.dp; dp != NULL; dp = dp->next) {
		double xpos = timeLine->posAtValue(dp->time / 60.0);
		double ypos = depthLine->posAtValue(dp->depth / 1000.0);
		QGraphicsLineItem *item = new QGraphicsLineItem(lastx, lasty, xpos, ypos);
		item->setPen(QPen(QBrush(dp->entered ? Qt::black : Qt::red),0));
		lastx = xpos;
		lasty = ypos;
		scene()->addItem(item);
		lines << item;
	}
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
	// do not allow it to move between handlers.
	if (handles.count() > 1) {
		if (idx == 0 ) { // first
			if (newPos.x() < handles[1]->x()) {
				activeDraggedHandler->setPos(newPos);
				lastValidPos = newPos;
			}
		} else if (idx == handles.count()-1) { // last
			if (newPos.x() > handles[idx-1]->x()) {
				activeDraggedHandler->setPos(newPos);
				lastValidPos = newPos;
			}
		} else { // middle
			if (newPos.x() > handles[idx-1]->x() && newPos.x() < handles[idx+1]->x()) {
				activeDraggedHandler->setPos(newPos);
				lastValidPos = newPos;
			}
		}
	} else {
		activeDraggedHandler->setPos(newPos);
		lastValidPos = newPos;
	}
	qDeleteAll(lines);
	lines.clear();
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
		activeDraggedHandler->sec = rint(timeLine->valueAt(lastValidPos)) * 60;
		activeDraggedHandler->mm = rint(depthLine->valueAt(lastValidPos)) * 1000;
		activeDraggedHandler->setBrush(QBrush());
		createDecoStops();
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
