// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFANIMATIONS_H
#define QPREFANIMATIONS_H
#include "core/pref.h"

#include <QObject>

class qPrefAnimations : public QObject {
	Q_OBJECT
	Q_PROPERTY(int animation_speed READ animation_speed WRITE set_animation_speed NOTIFY animation_speed_changed);

public:
	qPrefAnimations(QObject *parent = NULL);
	static qPrefAnimations *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static int animation_speed() { return prefs.animation_speed; }

public slots:
	static void set_animation_speed(int value);

signals:
	void animation_speed_changed(int value);

private:
	// functions to load/sync variable with disk
	static void disk_animation_speed(bool doSync);
};

#endif
