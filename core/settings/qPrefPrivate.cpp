// SPDX-License-Identifier: GPL-2.0
#include "qPrefPrivate.h"

qPrefPrivate::qPrefPrivate(QObject *parent) : QObject(parent)
{
}
qPrefPrivate *qPrefPrivate::instance()
{
	static qPrefPrivate *self = new qPrefPrivate;
	return self;
}

void qPrefPrivate::copy_txt(const char **name, const QString &string)
{
	free((void *)*name);
	*name = copy_qstring(string);
}
