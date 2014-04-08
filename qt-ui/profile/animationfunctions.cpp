#include "animationfunctions.h"
#include "dive.h"
#include "pref.h"
#include <QPropertyAnimation>
#include <QPointF>

namespace Animations
{

	void hide(QObject *obj)
	{
		QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
		animation->setStartValue(1);
		animation->setEndValue(0);
		animation->start(QAbstractAnimation::DeleteWhenStopped);
	}

	void animDelete(QObject *obj)
	{
		QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
		obj->connect(animation, SIGNAL(finished()), SLOT(deleteLater()));
		animation->setStartValue(1);
		animation->setEndValue(0);
		animation->start(QAbstractAnimation::DeleteWhenStopped);
	}

	void moveTo(QObject *obj, qreal x, qreal y)
	{
		if (prefs.animation != 0){
			QPropertyAnimation *animation = new QPropertyAnimation(obj, "pos");
			animation->setDuration(prefs.animation);
			animation->setStartValue(obj->property("pos").toPointF());
			animation->setEndValue(QPointF(x, y));
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		}
		else{
			obj->setProperty("pos", QPointF(x,y));
		}
	}

	void moveTo(QObject *obj, const QPointF &pos)
	{
		moveTo(obj, pos.x(), pos.y());
	}
}
