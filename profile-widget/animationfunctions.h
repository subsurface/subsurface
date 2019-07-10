// SPDX-License-Identifier: GPL-2.0
#ifndef ANIMATIONFUNCTIONS_H
#define ANIMATIONFUNCTIONS_H

#include <QtGlobal>
#include <QPointF>

class QObject;

namespace Animations {
	void hide(QObject *obj, int speed);
	void show(QObject *obj, int speed);
	void moveTo(QObject *obj, int speed, qreal x, qreal y);
	void moveTo(QObject *obj, int speed, const QPointF &pos);
	void animDelete(QObject *obj, int speed);
	void scaleTo(QObject *obj, int speed, qreal scale);
}

#endif // ANIMATIONFUNCTIONS_H
