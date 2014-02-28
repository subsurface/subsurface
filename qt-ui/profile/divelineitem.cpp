#include "divelineitem.h"
#include "animationfunctions.h"
#include <QPropertyAnimation>

DiveLineItem::DiveLineItem(QGraphicsItem *parent) : QGraphicsLineItem(parent)
{
}

void DiveLineItem::animatedHide()
{
	Animations::hide(this);
}

void DiveLineItem::animateMoveTo(qreal x, qreal y)
{
	Animations::moveTo(this, x, y);
}
