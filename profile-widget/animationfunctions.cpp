// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/animationfunctions.h"
#include "core/pref.h"
#include <QPropertyAnimation>

namespace Animations {

	void hide(QObject *obj, int speed)
	{
		if (speed != 0) {
			QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
			animation->setStartValue(1);
			animation->setEndValue(0);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		} else {
			obj->setProperty("opacity", 0);
		}
	}

	void show(QObject *obj, int speed)
	{
		if (speed != 0) {
			QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
			animation->setStartValue(0);
			animation->setEndValue(1);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		} else {
			obj->setProperty("opacity", 1);
		}
	}

	void animDelete(QObject *obj, int speed)
	{
		if (speed != 0) {
			QPropertyAnimation *animation = new QPropertyAnimation(obj, "opacity");
			obj->connect(animation, &QPropertyAnimation::finished, &QObject::deleteLater);
			animation->setStartValue(1);
			animation->setEndValue(0);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		} else {
			delete obj;
		}
	}

	void moveTo(QObject *obj, int speed, qreal x, qreal y)
	{
		if (speed != 0) {
			QPropertyAnimation *animation = new QPropertyAnimation(obj, "pos");
			animation->setDuration(prefs.animation_speed);
			animation->setStartValue(obj->property("pos").toPointF());
			animation->setEndValue(QPointF(x, y));
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		} else {
			obj->setProperty("pos", QPointF(x, y));
		}
	}

	void scaleTo(QObject *obj, int speed, qreal scale)
	{
		if (speed != 0) {
			QPropertyAnimation *animation = new QPropertyAnimation(obj, "scale");
			animation->setDuration(prefs.animation_speed);
			animation->setStartValue(obj->property("scale").toReal());
			animation->setEndValue(QVariant::fromValue(scale));
			animation->setEasingCurve(QEasingCurve::InCubic);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		} else {
			obj->setProperty("scale", QVariant::fromValue(scale));
		}
	}

	void moveTo(QObject *obj, int speed, const QPointF &pos)
	{
		moveTo(obj, speed, pos.x(), pos.y());
	}
}
