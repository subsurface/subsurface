// SPDX-License-Identifier: GPL-2.0
#include "qPrefLocationService.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("LocationService");

qPrefLocationService::qPrefLocationService(QObject *parent) : QObject(parent)
{
}

qPrefLocationService *qPrefLocationService::instance()
{
	static qPrefLocationService *self = new qPrefLocationService;
	return self;
}

void qPrefLocationService::loadSync(bool doSync)
{
	disk_distance_threshold(doSync);
	disk_time_threshold(doSync);
}

HANDLE_PREFERENCE_INT(LocationService, "distance_threshold", distance_threshold);

HANDLE_PREFERENCE_INT(LocationService, "time_threshold", time_threshold);
