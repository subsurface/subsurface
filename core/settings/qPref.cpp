// SPDX-License-Identifier: GPL-2.0
#include "qPref_private.h"
#include "qPref.h"


qPref *qPref::m_instance = NULL;
qPref *qPref::instance()
{
	if (!m_instance)
		m_instance = new qPref;
	return m_instance;
}



void qPref::loadSync(bool doSync)
{
}
