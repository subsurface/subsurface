// SPDX-License-Identifier: GPL-2.0
#include "qPref_private.h"
#include "qPref.h"
#include "ssrf-version.h"

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

const QString qPref::canonical_version() const
{
	return QString(CANONICAL_VERSION_STRING);
}

const QString qPref::mobile_version() const
{
	return QString(MOBILE_VERSION_STRING);
}
