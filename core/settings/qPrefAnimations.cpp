// SPDX-License-Identifier: GPL-2.0
#include "qPrefAnimations.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("Animations");

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
