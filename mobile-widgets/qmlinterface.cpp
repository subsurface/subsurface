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
	connect(qPrefUnits::instance(), &qPrefUnits::duration_unitsChanged,
			[=] (int value) { emit instance()->duration_unitsChanged(DURATION(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::lengthChanged,
			[=] (int value) { emit instance()->lengthChanged(LENGTH(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::pressureChanged,
			[=] (int value) { emit instance()->pressureChanged(PRESSURE(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::temperatureChanged,
			[=] (int value) { emit instance()->temperatureChanged(TEMPERATURE(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::unit_systemChanged,
			[=] (int value) { emit instance()->unit_systemChanged(UNIT_SYSTEM(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::vertical_speed_timeChanged,
			[=] (int value) { emit instance()->vertical_speed_timeChanged(TIME(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::volumeChanged,
			[=] (int value) { emit instance()->volumeChanged(VOLUME(value)); });
	connect(qPrefUnits::instance(), &qPrefUnits::weightChanged,
			[=] (int value) { emit instance()->weightChanged(WEIGHT(value)); });

	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascratelast6mChanged,
			instance(), &QMLInterface::ascratelast6mChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascratestopsChanged,
			instance(), &QMLInterface::ascratestopsChanged);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascrate50Changed,
			instance(), &QMLInterface::ascrate50Changed);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascrate75Changed,
			instance(), &QMLInterface::ascrate75Changed);
	connect(qPrefDivePlanner::instance(), &qPrefDivePlanner::descrateChanged,
			instance(), &QMLInterface::descrateChanged);
}
