#include "diveplanner.h"
#include "graphicsview-common.h"

#include "../dive.h"
#include <cmath>
#include <QMouseEvent>
#include <QDebug>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>
#include <QPushButton>

#include "ui_diveplanner.h"
#include "mainwindow.h"

#define TIME_INITIAL_MAX 30

DivePlannerGraphics::DivePlannerGraphics(QWidget* parent): QGraphicsView(parent), activeDraggedHandler(0),
	lastValidPos(0.0, 0.0)
{
	fill_profile_color();
	setBackgroundBrush(profile_color[BACKGROUND].at(0));
	setMouseTracking(true);
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0,0,1920,1080);

	QRectF r = scene()->sceneRect();

	verticalLine = new QGraphicsLineItem(
		fromPercent(0, Qt::Horizontal),
		fromPercent(0, Qt::Vertical),
		fromPercent(0, Qt::Horizontal),
		fromPercent(100, Qt::Vertical)
	);

	verticalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(verticalLine);

	horizontalLine = new QGraphicsLineItem(
		fromPercent(0, Qt::Horizontal),
		fromPercent(0, Qt::Vertical),
		fromPercent(100, Qt::Horizontal),
		fromPercent(0, Qt::Vertical)
	);
	horizontalLine->setPen(QPen(Qt::DotLine));
	scene()->addItem(horizontalLine);

	timeLine = new Ruler();
	timeLine->setMinimum(0);
	timeLine->setMaximum(TIME_INITIAL_MAX);
	timeLine->setTickInterval(10);
	timeLine->setLine(
		fromPercent(10, Qt::Horizontal),
		fromPercent(90, Qt::Vertical),
		fromPercent(90, Qt::Horizontal),
		fromPercent(90, Qt::Vertical)
	);
	timeLine->setOrientation(Qt::Horizontal);
	timeLine->setTickSize(fromPercent(1, Qt::Vertical));
	timeLine->setColor(profile_color[TIME_GRID].at(0));
	timeLine->updateTicks();
	scene()->addItem(timeLine);

	depthLine = new Ruler();
	depthLine->setMinimum(0);
	depthLine->setMaximum(40);
	depthLine->setTickInterval(10);
	depthLine->setLine(
		fromPercent(10, Qt::Horizontal),
		fromPercent(10, Qt::Vertical),
		fromPercent(10, Qt::Horizontal),
		fromPercent(90, Qt::Vertical)
	);
	depthLine->setOrientation(Qt::Vertical);
	depthLine->setTickSize(fromPercent(1, Qt::Horizontal));
	depthLine->setColor(profile_color[DEPTH_GRID].at(0));
	depthLine->updateTicks();
	scene()->addItem(depthLine);

	timeString = new QGraphicsSimpleTextItem();
	timeString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	scene()->addItem(timeString);

	depthString = new QGraphicsSimpleTextItem();
	depthString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	scene()->addItem(depthString);

	diveBg = new QGraphicsPolygonItem();
	diveBg->setBrush(QBrush(Qt::green));
	scene()->addItem(diveBg);

	plusDepth = new Button();
	plusDepth->setPixmap(QPixmap(":plus"));
	plusDepth->setPos(0, 1);
	scene()->addItem(plusDepth);
	connect(plusDepth, SIGNAL(clicked()), this, SLOT(increaseDepth()));

	plusTime = new Button();
	plusTime->setPixmap(QPixmap(":plus"));
	plusTime->setPos(180, 190);
	scene()->addItem(plusTime);
	connect(plusTime, SIGNAL(clicked()), this, SLOT(increaseTime()));

	okBtn = new Button();
	okBtn->setText(tr("Ok"));
	okBtn->setPos(1, 190);
	scene()->addItem(okBtn);
	connect(okBtn, SIGNAL(clicked()), this, SLOT(okClicked()));

	cancelBtn = new Button();
	cancelBtn->setText(tr("Cancel"));
	cancelBtn->setPos(10,190);
	scene()->addItem(cancelBtn);
	connect(cancelBtn, SIGNAL(clicked()), this, SLOT(cancelClicked()));

	setRenderHint(QPainter::Antialiasing);
}

qreal DivePlannerGraphics::fromPercent(qreal percent, Qt::Orientation orientation)
{
	qreal total = orientation == Qt::Horizontal ? sceneRect().width() : sceneRect().height();
	qreal result = (total * percent) / 100;
	return result;
}

void DivePlannerGraphics::cancelClicked()
{
	qDebug() << "clicked";
	mainWindow()->showProfile();
}

void DivePlannerGraphics::okClicked()
{
	// todo.
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

	if (timeLine->maximum() < dp->time / 60.0 + 5 ||
	    dp->time / 60.0 + 15 < timeLine->maximum()) {
		double newMax = fmax(dp->time / 60.0 + 5, TIME_INITIAL_MAX);
		timeLine->setMaximum(newMax);
		timeLine->updateTicks();
	}

	// Re-position the user generated dive handlers
	Q_FOREACH(DiveHandler *h, handles){
		h->setPos(timeLine->posAtValue(h->sec / 60), depthLine->posAtValue(h->mm / 1000));
	}

	// (re-) create the profile with different colors for segments that were
	// entered vs. segments that were calculated
	double lastx = timeLine->posAtValue(0);
	double lasty = depthLine->posAtValue(0);

	QPolygonF poly;
	poly.append(QPointF(lastx, lasty));
	for (dp = diveplan.dp; dp != NULL; dp = dp->next) {
		double xpos = timeLine->posAtValue(dp->time / 60.0);
		double ypos = depthLine->posAtValue(dp->depth / 1000.0);
		QGraphicsLineItem *item = new QGraphicsLineItem(lastx, lasty, xpos, ypos);
		item->setPen(QPen(QBrush(dp->entered ? Qt::black : Qt::red),0));
		lastx = xpos;
		lasty = ypos;
		scene()->addItem(item);
		lines << item;
		poly.append(QPointF(lastx, lasty));
	}

	diveBg->setPolygon(poly);
	QRectF b = poly.boundingRect();
	QLinearGradient pat(
		b.x(),
		b.y(),
		b.x(),
		b.height() + b.y()
	);

	pat.setColorAt(1, profile_color[DEPTH_BOTTOM].first());
	pat.setColorAt(0, profile_color[DEPTH_TOP].first());
	diveBg->setBrush(pat);

	deleteTemporaryDivePlan(diveplan.dp);
}

void DivePlannerGraphics::deleteTemporaryDivePlan(divedatapoint* dp)
{
	if (!dp)
		return;
	deleteTemporaryDivePlan(dp->next);
	free(dp);
}

void DivePlannerGraphics::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
}

void DivePlannerGraphics::showEvent(QShowEvent* event)
{
	QGraphicsView::showEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
}

void DivePlannerGraphics::mouseMoveEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	verticalLine->setPos(mappedPos.x(), fromPercent(0, Qt::Vertical));
	horizontalLine->setPos(fromPercent(0, Qt::Horizontal), mappedPos.y());

	depthString->setText(QString::number(rint(depthLine->valueAt(mappedPos))) + "m" );
	depthString->setPos(fromPercent(5, Qt::Horizontal), mappedPos.y());
	timeString->setText(QString::number(rint(timeLine->valueAt(mappedPos))) + "min");
	timeString->setPos(mappedPos.x()+1, fromPercent(95, Qt::Vertical));

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
		activeDraggedHandler->setBrush(QBrush(Qt::white));
		createDecoStops();
		activeDraggedHandler = 0;
	}
}

DiveHandler::DiveHandler(): QGraphicsEllipseItem(), from(0), to(0)
{
	setBrush(Qt::white);
	setZValue(2);
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
	QGraphicsLineItem *item = NULL;

	if (orientation == Qt::Horizontal) {
		double steps = (max - min) / interval;
		double stepSize = (m.x2() - m.x1()) / steps;
		qreal pos;
		for (qreal pos = m.x1(); pos < m.x2(); pos += stepSize) {
			item = new QGraphicsLineItem(pos, m.y1(), pos, m.y1() + tickSize, this);
			item->setPen(pen());
			ticks.push_back(item);
		}
		item = new QGraphicsLineItem(pos, m.y1(), pos, m.y1() + tickSize, this);
		item->setPen(pen());
		ticks.push_back(item);
	} else {
		double steps = (max - min) / interval;
		double stepSize = (m.y2() - m.y1()) / steps;
		qreal pos;
		for (pos = m.y1(); pos < m.y2(); pos += stepSize) {
			item = new QGraphicsLineItem(m.x1(), pos, m.x1() - tickSize, pos, this);
			item->setPen(pen());
			ticks.push_back(item);
		}
		item = new QGraphicsLineItem(m.x1(), pos, m.x1() - tickSize, pos, this);
		item->setPen(pen());
		ticks.push_back(item);
	}
}

void Ruler::setTickSize(qreal size)
{
	tickSize = size;
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

double Ruler::maximum() const
{
	return max;
}

double Ruler::minimum() const
{
	return min;
}

void Ruler::setColor(const QColor& color)
{
	setPen(QPen(color));
}

Button::Button(QObject* parent): QObject(parent), QGraphicsRectItem()
{
	icon  = new QGraphicsPixmapItem(this);
	text = new QGraphicsSimpleTextItem(this);
	icon->setPos(0,0);
	text->setPos(0,0);
	setFlag(ItemIgnoresTransformations);
	setPen(QPen(QBrush(Qt::white), 0));
}

void Button::setPixmap(const QPixmap& pixmap)
{
	icon->setPixmap(pixmap.scaled(20,20));
	if(pixmap.isNull()){
		icon->hide();
	}else{
		icon->show();
	}
	setRect(childrenBoundingRect());
}

void Button::setText(const QString& t)
{
	text->setText(t);
	if(icon->pixmap().isNull()){
		icon->hide();
		text->setPos(0,0);
	}else{
		icon->show();
		text->setPos(22,0);
	}
	setRect(childrenBoundingRect());
}

void Button::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	emit clicked();
}
