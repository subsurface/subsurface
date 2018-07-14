// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPref_private.h"
#include "qPrefAnimations.h"

qPrefAnimations::qPrefAnimations(QObject *parent) : QObject(parent)
{
}
qPrefAnimations *qPrefAnimations::instance()
{
	static qPrefAnimations *self = new qPrefAnimations;
	return self;
}

void qPrefAnimations::loadSync(bool doSync)
{
	disk_animation_speed(doSync);
}

HANDLE_PREFERENCE_INT(Animations, "/animation_speed", animation_speed);
