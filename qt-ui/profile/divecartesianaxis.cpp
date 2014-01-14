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
}

void DiveCartesianAxis::setMinimum(double minimum)
{
	min = minimum;
}

void DiveCartesianAxis::setTextColor(const QColor& color)
{
	textColor = color;
}

DiveCartesianAxis::DiveCartesianAxis() : orientation(Qt::Horizontal)
{
}

DiveCartesianAxis::~DiveCartesianAxis()
{

}

void DiveCartesianAxis::setOrientation(Qt::Orientation o)
{
	orientation = o;
	// position the elements on the screen.
	setMinimum(minimum());
	setMaximum(maximum());
}

void DiveCartesianAxis::updateTicks()
{
	QLineF m = line();
	QGraphicsView *view = scene()->views().first();
	double steps = (max - min) / interval;
	double currValue = min;

	// Remove the uneeded Ticks / Texts.
	if (ticks.size() > steps){
		while (ticks.size() > steps){
			DiveLineItem *removedLine = ticks.takeLast();
			removedLine->animatedHide();
			DiveTextItem *removedText = labels.takeLast();
			removedText->animatedHide();
		}
	}

	// Move the remaining Ticks / Text to it's corerct position
	// Regartind the possibly new values for the Axis
	qreal begin = orientation == Qt::Horizontal ? m.x1() : m.y1();
	qreal end = orientation == Qt::Horizontal ? m.x2() : m.y2();
	double stepSize =  orientation == Qt::Horizontal ? (m.x2() - m.x1()) : (m.y2() - m.y1());
	stepSize = stepSize / steps;
	for(int i = 0, count = ticks.size(); i < count; i++, currValue += interval){
		qreal childPos = begin + i * stepSize;
		labels[i]->setText(textForValue(currValue));
		if ( orientation == Qt::Horizontal ){
			ticks[i]->animateMoveTo(childPos, m.y1() + tickSize);
			labels[i]->animateMoveTo(childPos, m.y1() + tickSize);
		}
		else{
			ticks[i]->animateMoveTo(m.x1() - tickSize, childPos);
			labels[i]->animateMoveTo(m.x1() - tickSize, childPos);
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
	double retValue =  orientation == Qt::Horizontal ?
				max * (p.x() - m.x1()) / (m.x2() - m.x1()) :
				max * (p.y() - m.y1()) / (m.y2() - m.y1());
	return retValue;
}

qreal DiveCartesianAxis::posAtValue(qreal value)
{
	QLineF m = line();
	QPointF p = pos();

	double size = max - min;
	double percent = value / size;
	double realSize = orientation == Qt::Horizontal ?
				m.x2() - m.x1() :
				m.y2() - m.y1();
	double retValue = realSize * percent;
	retValue =  (orientation == Qt::Horizontal) ?
				retValue + m.x1() + p.x():
				retValue + m.y1() + p.y();
	return retValue;
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
