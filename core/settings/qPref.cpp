// SPDX-License-Identifier: GPL-2.0
#include "qPref_private.h"
#include "qPref.h"

qPref::qPref(QObject *parent) : 
	QObject(parent)
{
}
qPref *qPref::instance()
{
static qPref *self = new qPref;
	return self;
}

void qPref::loadSync(bool doSync)
{
}
