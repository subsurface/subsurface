#include "divelineitem.h"
#include <QPropertyAnimation>

DiveLineItem::DiveLineItem(QGraphicsItem *parent) : QGraphicsLineItem(parent)
{

}

void DiveLineItem::animatedHide()
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
	connect(animation, SIGNAL(finished()), SLOT(deleteLater()));
	animation->setStartValue(1);
	animation->setEndValue(0);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DiveLineItem::animateMoveTo(qreal x, qreal y)
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
	animation->setStartValue(property("pos").toPointF());
	animation->setEndValue(QPointF(x, y));
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}
