// SPDX-License-Identifier: GPL-2.0
#ifndef PLANNERSHARED_H
#define PLANNERSHARED_H
#include <QObject>
#include "core/pref.h"

// This is a shared class (mobile/desktop), and contains the core of the diveplanner
// without UI entanglement.
// It make variables and functions available to QML, these are referenced directly
// in the desktop version
//
// The mobile diveplanner shows all diveplans, but the editing functionality is
// limited to keep the UI simpler.

class plannerShared: public QObject {
	Q_OBJECT

	// Planning data
	Q_PROPERTY(deco_mode planner_deco_mode READ planner_deco_mode WRITE set_planner_deco_mode NOTIFY planner_deco_modeChanged);
	Q_PROPERTY(int reserve_gas READ reserve_gas WRITE set_reserve_gas NOTIFY reserve_gasChanged);
	Q_PROPERTY(bool dobailout READ dobailout WRITE set_dobailout NOTIFY dobailoutChanged);
	Q_PROPERTY(bool doo2breaks READ doo2breaks WRITE set_doo2breaks NOTIFY doo2breaksChanged);
	Q_PROPERTY(int min_switch_duration READ min_switch_duration WRITE set_min_switch_duration NOTIFY min_switch_durationChanged);
	Q_PROPERTY(int surface_segment READ surface_segment WRITE set_surface_segment NOTIFY surface_segmentChanged);

	// Gas data
	Q_PROPERTY(double bottomsac READ bottomsac WRITE set_bottomsac NOTIFY bottomsacChanged);
	Q_PROPERTY(double decosac READ decosac WRITE set_decosac NOTIFY decosacChanged);
	Q_PROPERTY(double sacfactor READ sacfactor WRITE set_sacfactor NOTIFY sacfactorChanged);
	Q_PROPERTY(bool o2narcotic READ o2narcotic WRITE set_o2narcotic NOTIFY o2narcoticChanged);
	Q_PROPERTY(double bottompo2 READ bottompo2 WRITE set_bottompo2 NOTIFY bottompo2Changed);
	Q_PROPERTY(double decopo2 READ decopo2 WRITE set_decopo2 NOTIFY decopo2Changed);
	Q_PROPERTY(int bestmixend READ bestmixend WRITE set_bestmixend NOTIFY bestmixendChanged);

public:
	static plannerShared *instance();

	// Planning data
	static deco_mode planner_deco_mode();
	static int reserve_gas();
	static bool dobailout();
	static bool doo2breaks();
	static int min_switch_duration();
	static int surface_segment();

	// Gas data
	static double bottomsac();
	static double decosac();
	static double sacfactor();
	static bool o2narcotic();
	static double bottompo2();
	static double decopo2();
	static int bestmixend();

public slots:

	// Planning data
	static void set_planner_deco_mode(deco_mode value);
	static void set_reserve_gas(int value);
	static void set_dobailout(bool value);
	static void set_doo2breaks(bool value);
	static void set_min_switch_duration(int value);
	static void set_surface_segment(int value);

	// Gas data
	static void set_bottomsac(double value);
	static void set_decosac(double value);
	static void set_sacfactor(double value);
	static void set_o2narcotic(bool value);
	static void set_bottompo2(double value);
	static void set_decopo2(double value);
	static void set_bestmixend(int value);

signals:
	// Planning data
	void planner_deco_modeChanged(deco_mode value);
	void reserve_gasChanged(int value);
	void dobailoutChanged(bool value);
	void doo2breaksChanged(bool value);
	void min_switch_durationChanged(int value);
	void surface_segmentChanged(int value);

	// Gas data
	void bottomsacChanged(double value);
	void decosacChanged(double value);
	void sacfactorChanged(double value);
	void o2narcoticChanged(bool value);
	void bottompo2Changed(double value);
	void decopo2Changed(double value);
	void bestmixendChanged(int value);

private:
	plannerShared() {}
};

#endif // PLANNERSHARED_H
