// SPDX-License-Identifier: GPL-2.0
#include "qmlinterface.h"

#include <QQmlEngine>

QMLInterface *QMLInterface::instance()
{
	static QMLInterface *self = new QMLInterface;
	return self;
}

void QMLInterface::setup(QQmlContext *ct)
{
	// Register interface class
	ct->setContextProperty("Backend", QMLInterface::instance());

	// Make enums available as types
	qmlRegisterUncreatableType<QMLInterface>("org.subsurfacedivelog.mobile",1,0,"Enums","Enum is not a type");

	// relink signals to QML
	connect(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_verification_statusChanged,
		[=] (int value) { emit instance()->cloud_verification_statusChanged(CLOUD_STATUS(value)); });
}
