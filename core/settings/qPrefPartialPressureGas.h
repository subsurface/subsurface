// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPARTICULARPRESSUREGAS_H
#define QPREFPARTICULARPRESSUREGAS_H
#include "core/pref.h"

#include <QObject>

class qPrefPartialPressureGas : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool phe READ phe WRITE set_phe NOTIFY phe_changed);
	Q_PROPERTY(double phe_threshold READ phe_threshold WRITE set_phe_threshold NOTIFY phe_threshold_changed);
	Q_PROPERTY(bool pn2 READ pn2 WRITE set_pn2 NOTIFY pn2_changed);
	Q_PROPERTY(double pn2_threshold READ pn2_threshold WRITE set_pn2_threshold NOTIFY pn2_threshold_changed);
	Q_PROPERTY(bool po2 READ po2 WRITE set_po2 NOTIFY po2_changed);
	Q_PROPERTY(double po2_threshold_max READ po2_threshold_max WRITE set_po2_threshold_max NOTIFY po2_threshold_max_changed);
	Q_PROPERTY(double po2_threshold_min READ po2_threshold_min WRITE set_po2_threshold_min NOTIFY po2_threshold_min_changed);

public:
	qPrefPartialPressureGas(QObject *parent = NULL);
	static qPrefPartialPressureGas *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	bool phe() { return prefs.pp_graphs.phe; }
	double phe_threshold() { return prefs.pp_graphs.phe_threshold; }
	bool pn2() { return prefs.pp_graphs.pn2; }
	double pn2_threshold() { return prefs.pp_graphs.pn2_threshold; }
	bool po2() { return prefs.pp_graphs.po2; }
	double po2_threshold_max() { return prefs.pp_graphs.po2_threshold_max; }
	double po2_threshold_min() { return prefs.pp_graphs.po2_threshold_min; }

public slots:
	void set_phe(bool value);
	void set_phe_threshold(double value);
	void set_pn2(bool value);
	void set_pn2_threshold(double value);
	void set_po2(bool value);
	void set_po2_threshold_min(double value);
	void set_po2_threshold_max(double value);

signals:
	void phe_changed(bool value);
	void phe_threshold_changed(double value);
	void pn2_changed(bool value);
	void pn2_threshold_changed(double value);
	void po2_changed(bool value);
	void po2_threshold_max_changed(double value);
	void po2_threshold_min_changed(double value);

private:
	void disk_phe(bool doSync);
	void disk_phe_threshold(bool doSync);
	void disk_pn2(bool doSync);
	void disk_pn2_threshold(bool doSync);
	void disk_po2(bool doSync);
	void disk_po2_threshold_min(bool doSync);
	void disk_po2_threshold_max(bool doSync);
};

#endif
