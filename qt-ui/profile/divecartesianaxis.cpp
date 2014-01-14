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
	qDeleteAll(ticks);
	ticks.clear();
	qDeleteAll(labels);
	labels.clear();

	QLineF m = line();
	DiveLineItem *item = NULL;
	DiveTextItem *label = NULL;

	double steps = (max - min) / interval;
	qreal pos;
	double currValue = min;

	if (orientation == Qt::Horizontal) {
		double stepSize = (m.x2() - m.x1()) / steps;
		for (pos = m.x1(); pos <= m.x2(); pos += stepSize, currValue += interval) {
			item = new DiveLineItem(this);
			item->setLine(pos, m.y1(), pos, m.y1() + tickSize);
			item->setPen(pen());
			ticks.push_back(item);

			label = new DiveTextItem(this);
			label->setText(QString::number(currValue));
			label->setBrush(QBrush(textColor));
			label->setFlag(ItemIgnoresTransformations);
			label->setPos(pos - label->boundingRect().width()/2, m.y1() + tickSize + 5);
			labels.push_back(label);
		}
	} else {
		double stepSize = (m.y2() - m.y1()) / steps;
		for (pos = m.y1(); pos <= m.y2(); pos += stepSize, currValue += interval) {
			item = new DiveLineItem(this);
			item->setLine(m.x1(), pos, m.x1() - tickSize, pos);
			item->setPen(pen());
			ticks.push_back(item);

			label = new DiveTextItem(this);
			label->setText(get_depth_string(currValue, false, false));
			label->setBrush(QBrush(textColor));
			label->setFlag(ItemIgnoresTransformations);
			label->setPos(m.x2() - 80, pos);
			labels.push_back(label);
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
