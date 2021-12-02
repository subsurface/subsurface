// SPDX-License-Identifier: GPL-2.0
#ifndef ANIMATIONFUNCTIONS_H
#define ANIMATIONFUNCTIONS_H

class QObject;

namespace Animations {
	void hide(QObject *obj, int speed);
	void show(QObject *obj, int speed);
	void scaleTo(QObject *obj, int speed, double scale);
}

#endif // ANIMATIONFUNCTIONS_H
