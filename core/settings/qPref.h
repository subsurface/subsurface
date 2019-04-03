// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H
#include "core/pref.h"

#include <QObject>
#include <QQmlEngine>

class qPref {
public:
	// Load/Sync local settings (disk) and struct preference
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

	// Register QML
	static void registerQML(QQmlEngine *engine);

private:
	static void loadSync(bool doSync);
};
#endif
