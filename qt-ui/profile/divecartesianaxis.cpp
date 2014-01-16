#include "divecartesianaxis.h"
#include "divelineitem.h"
#include "divetextitem.h"
#include "helpers.h"

#include <QPen>
#include <QGraphicsScene>
#include <QDebug>
#include <QPropertyAnimation>
#include <QGraphicsView>
#include <QStyleOption>

void DiveCartesianAxis::setMaximum(double maximum)
{
	max = maximum;
	emit sizeChanged();
}

void DiveCartesianAxis::setMinimum(double minimum)
{
	min = minimum;
	emit sizeChanged();
}

void DiveCartesianAxis::setTextColor(const QColor& color)
{
	textColor = color;
}

DiveCartesianAxis::DiveCartesianAxis() : orientation(LeftToRight)
{

}

DiveCartesianAxis::~DiveCartesianAxis()
{

}

void DiveCartesianAxis::setOrientation(Orientation o)
{
	orientation = o;
}

void DiveCartesianAxis::updateTicks()
{
	QLineF m = line();
	// unused so far:
	// QGraphicsView *view = scene()->views().first();
	double steps = (max - min) / interval;
	double currValue = min;

	// Remove the uneeded Ticks / Texts.
	if (ticks.size() > steps) {
		while (ticks.size() > steps) {
			DiveLineItem *removedLine = ticks.takeLast();
			removedLine->animatedHide();
			DiveTextItem *removedText = labels.takeLast();
			removedText->animatedHide();
		}
	}

	// Move the remaining Ticks / Text to it's corerct position
	// Regartind the possibly new values for the Axis
	qreal begin, stepSize;
	if (orientation == TopToBottom) {
		begin = m.y1();
		stepSize = (m.y2() - m.y1());
	} else if (orientation == BottomToTop) {
		begin = m.y2();
		stepSize = (m.y2() - m.y1());
	} else if (orientation == LeftToRight ) {
		begin = m.x1();
		stepSize = (m.x2() - m.x1());
	} else if (orientation == RightToLeft) {
		begin = m.x2();
		stepSize = (m.x2() - m.x1());
	}
	stepSize = stepSize / steps;


	for (int i = 0, count = ticks.size(); i < count; i++, currValue += interval) {
		qreal childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}
		labels[i]->setText(textForValue(currValue));
		if ( orientation == LeftToRight || orientation == RightToLeft) {
			ticks[i]->animateMoveTo(childPos, m.y1() + tickSize);
			labels[i]->animateMoveTo(childPos, m.y1() + tickSize);
		} else {
			ticks[i]->animateMoveTo(m.x1() - tickSize, childPos);
			labels[i]->animateMoveTo(m.x1() - tickSize, childPos);
		}
	}

	// Add's the rest of the needed Ticks / Text.
	for (int i = ticks.size(); i < steps; i++,  currValue += interval) {
		qreal childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
		childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}
		DiveLineItem *item = new DiveLineItem(this);
		item->setPen(pen());
		ticks.push_back(item);

		DiveTextItem *label = new DiveTextItem(this);
		label->setText(textForValue(currValue));
		label->setBrush(QBrush(textColor));

		labels.push_back(label);
		if (orientation == RightToLeft || orientation == LeftToRight) {
			item->setLine(0, 0, 0, tickSize);
			item->setPos(scene()->sceneRect().width() + 10, m.y1() + tickSize); // position it outside of the scene
			item->animateMoveTo(childPos, m.y1() + tickSize); // anim it to scene.
			label->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
			label->setPos(scene()->sceneRect().width() + 10, m.y1() + tickSize); // position it outside of the scene);
			label->animateMoveTo(childPos, m.y1() + tickSize);
		} else {
			item->setLine(0, 0, tickSize, 0);
			item->setPos(m.x1() - tickSize, scene()->sceneRect().height() + 10);
			item->animateMoveTo(m.x1() - tickSize, childPos);
			label->setAlignment(Qt::AlignVCenter| Qt::AlignRight);
			label->setPos(m.x1() - tickSize, scene()->sceneRect().height() + 10);
			label->animateMoveTo(m.x1() - tickSize, childPos);
		}
	}
}

QString DiveCartesianAxis::textForValue(double value)
{
	return QString::number(value);
}

void DiveCartesianAxis::setTickSize(qreal size)
{
	tickSize = size;
}

void DiveCartesianAxis::setTickInterval(double i)
{
	interval = i;
}

qreal DiveCartesianAxis::valueAt(const QPointF& p)
{
	QLineF m = line();
	double retValue =  orientation == LeftToRight || RightToLeft?
				max * (p.x() - m.x1()) / (m.x2() - m.x1()) :
				max * (p.y() - m.y1()) / (m.y2() - m.y1());
	return retValue;
}

qreal DiveCartesianAxis::posAtValue(qreal value)
{
	QLineF m = line();
	QPointF p = pos();

	double size = max - min;
	// unused for now:
	// double distanceFromOrigin = value - min;
	double percent = (value - min) / size;

	double realSize = orientation == LeftToRight || orientation == RightToLeft?
				m.x2() - m.x1() :
				m.y2() - m.y1();

	// Inverted axis, just invert the percentage.
	if (orientation == RightToLeft || orientation == BottomToTop)
		percent = 1 - percent;

	double retValue = realSize * percent;
	double adjusted =
		orientation == LeftToRight ?  retValue + m.x1() + p.x() :
		orientation == RightToLeft ?  retValue + m.x1() + p.x() :
		orientation == TopToBottom ?  retValue + m.y1() + p.y() :
		/* entation == BottomToTop */ retValue + m.y1() + p.y() ;
	return adjusted;
}

qreal DiveCartesianAxis::percentAt(const QPointF& p)
{
	qreal value = valueAt(p);
	double size = max - min;
	double percent = value / size;
	return percent;
}

double DiveCartesianAxis::maximum() const
{
	return max;
}

double DiveCartesianAxis::minimum() const
{
	return min;
}

void DiveCartesianAxis::setColor(const QColor& color)
{
	QPen defaultPen(color);
	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	defaultPen.setWidth(2);
	defaultPen.setCosmetic(true);
	setPen(defaultPen);
}

QString DepthAxis::textForValue(double value)
{
	return get_depth_string(value, false, false);
}

QString TimeAxis::textForValue(double value)
{
	return QString::number(value / 60);
}

QString TemperatureAxis::textForValue(double value)
{
	return QString::number(mkelvin_to_C( (int) value));
}


void DiveCartesianPlane::setLeftAxis(DiveCartesianAxis* axis)
{
	leftAxis = axis;
	connect(leftAxis, SIGNAL(sizeChanged()), this, SLOT(setup()));
	if (bottomAxis)
		setup();
}

void DiveCartesianPlane::setBottomAxis(DiveCartesianAxis* axis)
{
	bottomAxis = axis;
	connect(bottomAxis, SIGNAL(sizeChanged()), this, SLOT(setup()));
	if (leftAxis)
		setup();
}

QLineF DiveCartesianPlane::horizontalLine() const
{
	return (bottomAxis) ? bottomAxis->line() : QLineF() ;
}

void DiveCartesianPlane::setHorizontalLine(QLineF line)
{
	if ( horizontalSize == line.length())
		return;
	horizontalSize = line.length();
	setup();
}

void DiveCartesianPlane::setVerticalLine(QLineF line)
{
	if (verticalSize == line.length())
		return;
	verticalSize = line.length();
	setup();
}

QLineF DiveCartesianPlane::verticalLine() const
{
	return (leftAxis) ? leftAxis->line() : QLineF() ;
}

void DiveCartesianPlane::setup()
{
	if (!leftAxis || !bottomAxis || !scene())
		return;

	// This creates a Grid around the axis, creating the cartesian plane.
	const int top = leftAxis->posAtValue(leftAxis->minimum());
	// unused for now:
	// const int bottom = leftAxis->posAtValue(leftAxis->maximum());
	const int left = bottomAxis->posAtValue(bottomAxis->minimum());
	// unused for now:
	// const int right = bottomAxis->posAtValue(bottomAxis->maximum());

	setRect(0, 0, horizontalSize, verticalSize);
	setPos(left, top);

	qDeleteAll(horizontalLines);
	qDeleteAll(verticalLines);
	horizontalLines.clear();
	verticalLines.clear();

	// DEPTH is M_OR_FEET(10,30), Minutes are 600, per line.
	for (int i = leftAxis->minimum(), max = leftAxis->maximum(); i < max; i += M_OR_FT(10,30)) {
		DiveLineItem *line = new DiveLineItem();
		line->setLine(0, 0, horizontalSize, 0);
		line->setPos(left,leftAxis->posAtValue(i));
		line->setZValue(-1);
		horizontalLines.push_back(line);
		scene()->addItem(line);
	}

	for (int i = bottomAxis->minimum(), max = bottomAxis->maximum(); i < max; i += 600) { // increments by 10 minutes.
		DiveLineItem *line = new DiveLineItem();
		line->setLine(0, 0, 0, verticalSize);
		line->setPos(bottomAxis->posAtValue(i), top);
		line->setZValue(-1);
		verticalLines.push_back(line);
		scene()->addItem(line);
	}
}
