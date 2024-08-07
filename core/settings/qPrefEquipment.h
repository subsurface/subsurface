// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFEQUIPMENT_H
#define QPREFEQUIPMENT_H
#include "core/pref.h"

#include <QObject>

class qPrefEquipment : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString default_cylinder READ default_cylinder WRITE set_default_cylinder NOTIFY default_cylinderChanged)
	Q_PROPERTY(bool include_unused_tanks READ include_unused_tanks WRITE set_include_unused_tanks NOTIFY include_unused_tanksChanged)
	Q_PROPERTY(bool display_default_tank_infos READ display_default_tank_infos WRITE set_display_default_tank_infos NOTIFY display_default_tank_infosChanged)

public:
	static qPrefEquipment *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static QString default_cylinder() { return QString::fromStdString(prefs.default_cylinder); }
	static bool include_unused_tanks() { return prefs.include_unused_tanks; }
	static bool display_default_tank_infos() { return prefs.display_default_tank_infos; }

public slots:
	static void set_default_cylinder(const QString& value);
	static void set_include_unused_tanks(bool value);
	static void set_display_default_tank_infos(bool value);

signals:
	void default_cylinderChanged(const QString& value);
	void include_unused_tanksChanged(bool value);
	void display_default_tank_infosChanged(bool value);

private:
	qPrefEquipment() {}
	static void disk_default_cylinder(bool doSync);
	static void disk_include_unused_tanks(bool doSync);
	static void disk_display_default_tank_infos(bool doSync);
};

#endif

