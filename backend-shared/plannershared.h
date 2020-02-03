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

class PlannerShared: public QObject {
	Q_OBJECT

public:
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
};

#endif // PLANNERSHARED_H
