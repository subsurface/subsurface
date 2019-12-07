// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFEQUIPMENT_H
#define QPREFEQUIPMENT_H
#include "core/pref.h"

#include <QObject>

class qPrefEquipment : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString default_cylinder READ default_cylinder WRITE set_default_cylinder NOTIFY default_cylinderChanged)
	Q_PROPERTY(bool display_unused_tanks READ display_unused_tanks WRITE set_display_unused_tanks NOTIFY display_unused_tanksChanged)

public:
	static qPrefEquipment *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static QString default_cylinder() { return prefs.default_cylinder; }
	static bool display_unused_tanks() { return prefs.display_unused_tanks; }

public slots:
	static void set_default_cylinder(const QString& value);
	static void set_display_unused_tanks(bool value);

signals:
	void default_cylinderChanged(const QString& value);
	void display_unused_tanksChanged(bool value);

private:
	qPrefEquipment() {}
	static void disk_default_cylinder(bool doSync);
	static void disk_display_unused_tanks(bool doSync);
};

#endif

