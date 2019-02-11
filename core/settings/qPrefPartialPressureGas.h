// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPARTICULARPRESSUREGAS_H
#define QPREFPARTICULARPRESSUREGAS_H
#include "core/pref.h"

#include <QObject>

class qPrefPartialPressureGas : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool phe READ phe WRITE set_phe NOTIFY pheChanged)
	Q_PROPERTY(double phe_threshold READ phe_threshold WRITE set_phe_threshold NOTIFY phe_thresholdChanged)
	Q_PROPERTY(bool pn2 READ pn2 WRITE set_pn2 NOTIFY pn2Changed)
	Q_PROPERTY(double pn2_threshold READ pn2_threshold WRITE set_pn2_threshold NOTIFY pn2_thresholdChanged)
	Q_PROPERTY(bool po2 READ po2 WRITE set_po2 NOTIFY po2Changed)
	Q_PROPERTY(double po2_threshold_max READ po2_threshold_max WRITE set_po2_threshold_max NOTIFY po2_threshold_maxChanged)
	Q_PROPERTY(double po2_threshold_min READ po2_threshold_min WRITE set_po2_threshold_min NOTIFY po2_threshold_minChanged)

public:
	static qPrefPartialPressureGas *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static bool phe() { return prefs.pp_graphs.phe; }
	static double phe_threshold() { return prefs.pp_graphs.phe_threshold; }
	static bool pn2() { return prefs.pp_graphs.pn2; }
	static double pn2_threshold() { return prefs.pp_graphs.pn2_threshold; }
	static bool po2() { return prefs.pp_graphs.po2; }
	static double po2_threshold_max() { return prefs.pp_graphs.po2_threshold_max; }
	static double po2_threshold_min() { return prefs.pp_graphs.po2_threshold_min; }

public slots:
	static void set_phe(bool value);
	static void set_phe_threshold(double value);
	static void set_pn2(bool value);
	static void set_pn2_threshold(double value);
	static void set_po2(bool value);
	static void set_po2_threshold_min(double value);
	static void set_po2_threshold_max(double value);

signals:
	void pheChanged(bool value);
	void phe_thresholdChanged(double value);
	void pn2Changed(bool value);
	void pn2_thresholdChanged(double value);
	void po2Changed(bool value);
	void po2_threshold_maxChanged(double value);
	void po2_threshold_minChanged(double value);

private:
	qPrefPartialPressureGas() {}

	static void disk_phe(bool doSync);
	static void disk_phe_threshold(bool doSync);
	static void disk_pn2(bool doSync);
	static void disk_pn2_threshold(bool doSync);
	static void disk_po2(bool doSync);
	static void disk_po2_threshold_min(bool doSync);
	static void disk_po2_threshold_max(bool doSync);
};

#endif
