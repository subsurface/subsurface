// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFGENERAL_H
#define QPREFGENERAL_H
#include "core/pref.h"

#include <QObject>

class qPrefGeneral : public QObject {
	Q_OBJECT
	Q_PROPERTY(int defaultsetpoint READ defaultsetpoint WRITE set_defaultsetpoint NOTIFY defaultsetpointChanged)
	Q_PROPERTY(int o2consumption READ o2consumption WRITE set_o2consumption NOTIFY o2consumptionChanged)
	Q_PROPERTY(int pscr_ratio READ pscr_ratio WRITE set_pscr_ratio NOTIFY pscr_ratioChanged)
	Q_PROPERTY(QString diveshareExport_uid READ diveshareExport_uid WRITE set_diveshareExport_uid NOTIFY diveshareExport_uidChanged)
	Q_PROPERTY(bool diveshareExport_private READ diveshareExport_private WRITE set_diveshareExport_private NOTIFY diveshareExport_privateChanged)

public:
	static qPrefGeneral *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { return loadSync(false); }
	static void sync() { return loadSync(true); }

public:
	static int defaultsetpoint() { return prefs.defaultsetpoint; }
	static int o2consumption() { return prefs.o2consumption; }
	static int pscr_ratio() { return prefs.pscr_ratio; }
	static QString diveshareExport_uid() { return st_diveshareExport_uid; }
	static bool diveshareExport_private() { return st_diveshareExport_private; }

public slots:
	static void set_defaultsetpoint(int value);
	static void set_o2consumption(int value);
	static void set_pscr_ratio(int value);
	static void set_diveshareExport_uid(const QString& value);
	static void set_diveshareExport_private(bool value);

signals:
	void defaultsetpointChanged(int value);
	void o2consumptionChanged(int value);
	void pscr_ratioChanged(int value);
	void diveshareExport_uidChanged(const QString& value);
	void diveshareExport_privateChanged(bool value);
	void salinityEditDefaultChanged(bool value);

private:
	qPrefGeneral() {}

	static void disk_defaultsetpoint(bool doSync);
	static void disk_o2consumption(bool doSync);
	static void disk_pscr_ratio(bool doSync);

	// class variables are load only
	static void load_diveshareExport_uid();
	static void load_diveshareExport_private();

	// class variables
	static QString st_diveshareExport_uid;
	static bool st_diveshareExport_private;
};

#endif
