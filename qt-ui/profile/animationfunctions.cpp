#include "animationfunctions.h"
#include <QPropertyAnimation>
#include <QPointF>

namespace Animations {

void hide(QObject* obj)
{
	QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
	animation->setStartValue(1);
	animation->setEndValue(0);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void animDelete(QObject* obj)
{
	QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
	obj->connect(animation, SIGNAL(finished()), SLOT(deleteLater()));
	animation->setStartValue(1);
	animation->setEndValue(0);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void moveTo(QObject* obj, qreal x, qreal y, int msecs)
{
	QPropertyAnimation *animation = new QPropertyAnimation(obj, "pos");
	animation->setDuration(msecs);
	animation->setStartValue(obj->property("pos").toPointF());
	animation->setEndValue(QPointF(x, y));
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void moveTo(QObject* obj, const QPointF& pos, int msecs)
{
	moveTo(obj, pos.x(), pos.y(), msecs);
}

}
