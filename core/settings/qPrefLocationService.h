// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLOCATIONSERVICE_H
#define QPREFLOCATIONSERVICE_H
#include "core/pref.h"

#include <QObject>


class qPrefLocationService : public QObject {
	Q_OBJECT
	Q_PROPERTY(int distance_threshold READ distance_threshold WRITE set_distance_threshold NOTIFY distance_threshold_changed);
	Q_PROPERTY(int time_threshold READ time_threshold WRITE set_time_threshold NOTIFY time_threshold_changed);

public:
	qPrefLocationService(QObject *parent = NULL);
	static qPrefLocationService *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	int distance_threshold() { return prefs.distance_threshold; }
	int time_threshold() { return prefs.time_threshold; }

public slots:
	void set_distance_threshold(int value);
	void set_time_threshold(int value);

signals:
	void distance_threshold_changed(int value);
	void time_threshold_changed(int value);


private:
	void disk_distance_threshold(bool doSync);
	void disk_time_threshold(bool doSync);
};

#endif
