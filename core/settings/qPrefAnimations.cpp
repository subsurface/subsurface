// SPDX-License-Identifier: GPL-2.0
#include "qPref_private.h"
#include "qPrefAnimations.h"


qPrefAnimations *qPrefAnimations::m_instance = NULL;
qPrefAnimations *qPrefAnimations::instance()
{
	if (!m_instance)
		m_instance = new qPrefAnimations;
	return m_instance;
}



void qPrefAnimations::loadSync(bool doSync)
{
	diskAnimationSpeed(doSync);
}


int qPrefAnimations::animationSpeed() const
{
	return prefs.animation_speed;
}
void qPrefAnimations::setAnimationSpeed(int value)
{
	if (value != prefs.animation_speed) {
		prefs.animation_speed = value;
		diskAnimationSpeed(true);
		emit animationSpeedChanged(value);
	}
}
void qPrefAnimations::diskAnimationSpeed(bool doSync)
{
	LOADSYNC_INT("/animation_speed", animation_speed);
}
