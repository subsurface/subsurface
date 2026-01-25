// SPDX-License-Identifier: GPL-2.0
#ifndef QPREF_H
#define QPREF_H
#include "core/pref.h"

#include <QObject>
#ifndef SUBSURFACE_CLI
#include <QQmlEngine>
#endif

class qPref {
public:
	// Load/Sync local settings (disk) and struct preference
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

#ifndef SUBSURFACE_CLI
	// Register QML
	static void registerQML(QQmlEngine *engine);
#endif

private:
	static void loadSync(bool doSync);
};
#endif
