#include "diveplanner.h"
#include "graphicsview-common.h"

#include "../dive.h"
#include <cmath>
#include <QMouseEvent>
#include <QDebug>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>
#include <QPushButton>
#include <QGraphicsSceneMouseEvent>

#include "ui_diveplanner.h"
#include "mainwindow.h"

#define TIME_INITIAL_MAX 30

#define MAX_DEEPNESS 150
#define MIN_DEEPNESS 40

bool handlerLessThenMinutes(DiveHandler *d1, DiveHandler *d2){
	return d1->sec < d2->sec;
}

DivePlannerGraphics::DivePlannerGraphics(QWidget* parent): QGraphicsView(parent), activeDraggedHandler(0)
{
	fill_profile_color();
	setBackgroundBrush(profile_color[BACKGROUND].at(0));
	setMouseTracking(true);
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0,0,1920,1080);

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
	timeString->setBrush(profile_color[TIME_TEXT].at(0));
	scene()->addItem(timeString);

	depthString = new QGraphicsSimpleTextItem();
	depthString->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	depthString->setBrush(profile_color[SAMPLE_DEEP].at(0));
	scene()->addItem(depthString);

	diveBg = new QGraphicsPolygonItem();
	diveBg->setPen(QPen(QBrush(),0));
	scene()->addItem(diveBg);

	plusDepth = new Button();
	plusDepth->setPixmap(QPixmap(":plus"));
	plusDepth->setPos(fromPercent(5, Qt::Horizontal), fromPercent(5, Qt::Vertical));
	plusDepth->setToolTip("Increase maximum depth by 10m");
	scene()->addItem(plusDepth);
	connect(plusDepth, SIGNAL(clicked()), this, SLOT(increaseDepth()));

	plusTime = new Button();
	plusTime->setPixmap(QPixmap(":plus"));
	plusTime->setPos(fromPercent(95, Qt::Horizontal), fromPercent(95, Qt::Vertical));
	plusTime->setToolTip("Increase minimum dive time by 10m");
	scene()->addItem(plusTime);
	connect(plusTime, SIGNAL(clicked()), this, SLOT(increaseTime()));

	okBtn = new Button();
	okBtn->setText(tr("Ok"));
	okBtn->setPos(fromPercent(1, Qt::Horizontal), fromPercent(95, Qt::Vertical));
	scene()->addItem(okBtn);
	connect(okBtn, SIGNAL(clicked()), this, SLOT(okClicked()));

	cancelBtn = new Button();
	cancelBtn->setText(tr("Cancel"));
	cancelBtn->setPos(okBtn->pos().x() + okBtn->boundingRect().width() + fromPercent(2, Qt::Horizontal),
					  fromPercent(95, Qt::Vertical));
	scene()->addItem(cancelBtn);
	connect(cancelBtn, SIGNAL(clicked()), this, SLOT(cancelClicked()));

	minMinutes = TIME_INITIAL_MAX;

	QAction *action = NULL;

#define ADD_ACTION( SHORTCUT, Slot ) \
	action = new QAction(this); \
	action->setShortcut( SHORTCUT ); \
	action->setShortcutContext(Qt::ApplicationShortcut); \
	addAction(action); \
	connect(action, SIGNAL(triggered(bool)), this, SLOT( Slot ))

	ADD_ACTION(Qt::Key_Escape, keyEscAction());
	ADD_ACTION(Qt::Key_Delete, keyDeleteAction());
#undef ADD_ACTION

	setRenderHint(QPainter::Antialiasing);
}

void DivePlannerGraphics::keyDeleteAction()
{
	if(scene()->selectedItems().count()){
		Q_FOREACH(QGraphicsItem *i, scene()->selectedItems()){
			if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler*>(i)){
				handles.removeAll(handler);
				scene()->removeItem(handler);
				delete i;
			}
		}
		createDecoStops();
	}
}

void DivePlannerGraphics::keyEscAction()
{
	if (scene()->selectedItems().count()){
		scene()->clearSelection();
		return;
	}

	cancelClicked();
}

qreal DivePlannerGraphics::fromPercent(qreal percent, Qt::Orientation orientation)
{
	qreal total = orientation == Qt::Horizontal ? sceneRect().width() : sceneRect().height();
	qreal result = (total * percent) / 100;
	return result;
}

void DivePlannerGraphics::cancelClicked()
{
	mainWindow()->showProfile();
}

void DivePlannerGraphics::okClicked()
{
	// todo.
}

void DivePlannerGraphics::increaseDepth()
{
	if (depthLine->maximum() + 10 > MAX_DEEPNESS)
		return;
	depthLine->setMaximum( depthLine->maximum() + 10);
	depthLine->updateTicks();
	createDecoStops();
}

void DivePlannerGraphics::increaseTime()
{
	minMinutes += 10;
	timeLine->setMaximum( minMinutes );
	timeLine->updateTicks();
	createDecoStops();
}

void DivePlannerGraphics::mouseDoubleClickEvent(QMouseEvent* event)
{
	QPointF mappedPos = mapToScene(event->pos());
	if (isPointOutOfBoundaries(mappedPos))
		return;

	int minutes = rint(timeLine->valueAt(mappedPos));
	int meters = rint(depthLine->valueAt(mappedPos));
	double xpos = timeLine->posAtValue(minutes);
	double ypos = depthLine->posAtValue(meters);
	Q_FOREACH(DiveHandler* handler, handles){
		if (xpos == handler->pos().x()){
			qDebug() << "There's already an point at that place.";
			//TODO: Move this later to a KMessageWidget.
			return;
		}
	}

	DiveHandler  *item = new DiveHandler ();
	item->sec = minutes * 60;
	item->mm =  meters * 1000;
	item->setPos(QPointF(xpos, ypos));
	scene()->addItem(item);
	handles << item;
	createDecoStops();
}

void DivePlannerGraphics::createDecoStops()
{
	qDeleteAll(lines);
	lines.clear();
	qSort(handles.begin(), handles.end(), handlerLessThenMinutes);

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
		double newMax = fmax(dp->time / 60.0 + 5, minMinutes);
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
		if(!dp->entered){
			QGraphicsLineItem *item = new QGraphicsLineItem(lastx, lasty, xpos, ypos);
			item->setPen(QPen(QBrush(Qt::red),0));
			scene()->addItem(item);
			lines << item;
		}
		lastx = xpos;
		lasty = ypos;
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

	// calculate the correct color for the depthString.
	// QGradient doesn't returns it's interpolation, meh.
	double percent = depthLine->percentAt(mappedPos);
	QColor& startColor = profile_color[SAMPLE_SHALLOW].first();
	QColor& endColor = profile_color[SAMPLE_DEEP].first();
	short redDelta = (endColor.red() - startColor.red()) * percent + startColor.red();
	short greenDelta = (endColor.green() - startColor.green()) * percent + startColor.green();
	short blueDelta = (endColor.blue() - startColor.blue()) * percent + startColor.blue();
	depthString->setBrush( QColor(redDelta, greenDelta, blueDelta));

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
	double xpos = timeLine->posAtValue(rint(timeLine->valueAt(pos)));
	double ypos = depthLine->posAtValue(rint(depthLine->valueAt(pos)));
	activeDraggedHandler->setPos(QPointF(xpos, ypos));
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
	if (event->modifiers()){
		QGraphicsView::mousePressEvent(event);
	}

    QPointF mappedPos = mapToScene(event->pos());
	Q_FOREACH(QGraphicsItem *item, scene()->items(mappedPos, Qt::IntersectsItemBoundingRect, Qt::AscendingOrder, transform())){
		if (DiveHandler *h = qgraphicsitem_cast<DiveHandler*>(item)) {
			activeDraggedHandler = h;
			activeDraggedHandler->setBrush(Qt::red);
			originalHandlerPos = activeDraggedHandler->pos();
		}
	}
}

void DivePlannerGraphics::mouseReleaseEvent(QMouseEvent* event)
{
	if (activeDraggedHandler) {
		QPointF mappedPos = mapToScene(event->pos());
		int minutes = rint(timeLine->valueAt(mappedPos));
		int meters = rint(depthLine->valueAt(mappedPos));
		double xpos = timeLine->posAtValue(minutes);
		double ypos = depthLine->posAtValue(meters);
		Q_FOREACH(DiveHandler* handler, handles){
			if (xpos == handler->pos().x() && handler != activeDraggedHandler){
				qDebug() << "There's already an point at that place.";
				//TODO: Move this later to a KMessageWidget.
				activeDraggedHandler->setPos(originalHandlerPos);
				activeDraggedHandler = NULL;
				return;
			}
		}

		activeDraggedHandler->sec = rint(timeLine->valueAt(mappedPos)) * 60;
		activeDraggedHandler->mm = rint(depthLine->valueAt(mappedPos)) * 1000;
		activeDraggedHandler->setBrush(QBrush(Qt::white));
		activeDraggedHandler->setPos(QPointF(xpos, ypos));

		createDecoStops();
		activeDraggedHandler = 0;
	}
}

DiveHandler::DiveHandler(): QGraphicsEllipseItem(), from(0), to(0)
{
	setRect(-5,-5,10,10);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setBrush(Qt::white);
	setZValue(2);
}

void DiveHandler::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->modifiers().testFlag(Qt::ControlModifier)){
		setSelected(true);
	}
	// mousePressEvent 'grabs' the mouse and keyboard, annoying.
	ungrabMouse();
	ungrabKeyboard();
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
		for (pos = m.x1(); pos < m.x2(); pos += stepSize) {
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

qreal Ruler::percentAt(const QPointF& p)
{
	qreal value = valueAt(p);
	double size = max - min;
	double percent = value / size;
	return percent;
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
	setPen(QPen(QBrush(), 0));
}

void Button::setPixmap(const QPixmap& pixmap)
{
	icon->setPixmap(pixmap.scaled(20,20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
	event->ignore();
	emit clicked();
}
