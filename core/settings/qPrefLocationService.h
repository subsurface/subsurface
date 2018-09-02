// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLOCATIONSERVICE_H
#define QPREFLOCATIONSERVICE_H
#include "core/pref.h"

#include <QObject>


class qPrefLocationService : public QObject {
	Q_OBJECT
	Q_PROPERTY(int distance_threshold READ distance_threshold WRITE set_distance_threshold NOTIFY distance_thresholdChanged);
	Q_PROPERTY(int time_threshold READ time_threshold WRITE set_time_threshold NOTIFY time_thresholdChanged);

public:
	qPrefLocationService(QObject *parent = NULL);
	static qPrefLocationService *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static int distance_threshold() { return prefs.distance_threshold; }
	static int time_threshold() { return prefs.time_threshold; }

public slots:
	static void set_distance_threshold(int value);
	static void set_time_threshold(int value);

signals:
	void distance_thresholdChanged(int value);
	void time_thresholdChanged(int value);


private:
	static void disk_distance_threshold(bool doSync);
	static void disk_time_threshold(bool doSync);
};

#endif
