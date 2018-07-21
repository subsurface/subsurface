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
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	static inline int animation_speed() { return prefs.animation_speed; };

public slots:
	void set_animation_speed(int value);

signals:
	void animation_speed_changed(int value);

private:
	// functions to load/sync variable with disk
	void disk_animation_speed(bool doSync);
};

#endif
