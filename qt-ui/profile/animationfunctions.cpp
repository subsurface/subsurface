#include "animationfunctions.h"
#include <QPropertyAnimation>
#include <QPointF>

namespace Animations{

void hide(QObject* obj)
{
	QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
	obj->connect(animation, SIGNAL(finished()), SLOT(deleteLater()));
	animation->setStartValue(1);
	animation->setEndValue(0);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void moveTo(QObject* obj, qreal x, qreal y)
{
	QPropertyAnimation *animation = new QPropertyAnimation(obj, "pos");
	animation->setStartValue(obj->property("pos").toPointF());
	animation->setEndValue(QPointF(x, y));
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

}
