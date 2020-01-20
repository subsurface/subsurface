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
	Q_PROPERTY(bool safetystop READ safetystop WRITE set_safetystop NOTIFY safetystopChanged);
	Q_PROPERTY(int gflow READ gflow WRITE set_gflow NOTIFY gflowChanged);
	Q_PROPERTY(int gfhigh READ gfhigh WRITE set_gfhigh NOTIFY gfhighChanged);
	Q_PROPERTY(int vpmb_conservatism READ vpmb_conservatism WRITE set_vpmb_conservatism NOTIFY vpmb_conservatismChanged);
	Q_PROPERTY(bool dobailout READ dobailout WRITE set_dobailout NOTIFY dobailoutChanged);
	Q_PROPERTY(bool drop_stone_mode READ drop_stone_mode WRITE set_drop_stone_mode NOTIFY drop_stone_modeChanged);
	Q_PROPERTY(bool last_stop READ last_stop WRITE set_last_stop NOTIFY last_stopChanged);
	Q_PROPERTY(bool switch_at_req_stop READ switch_at_req_stop WRITE set_switch_at_req_stop NOTIFY switch_at_req_stopChanged);
	Q_PROPERTY(bool doo2breaks READ doo2breaks WRITE set_doo2breaks NOTIFY doo2breaksChanged);
	Q_PROPERTY(int min_switch_duration READ min_switch_duration WRITE set_min_switch_duration NOTIFY min_switch_durationChanged);

	// Gas data
	Q_PROPERTY(double bottomsac READ bottomsac WRITE set_bottomsac NOTIFY bottomsacChanged);
	Q_PROPERTY(double decosac READ decosac WRITE set_decosac NOTIFY decosacChanged);
	Q_PROPERTY(int problemsolvingtime READ problemsolvingtime WRITE set_problemsolvingtime NOTIFY problemsolvingtimeChanged);
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
	static bool safetystop();
	static int gflow();
	static int gfhigh();
	static int vpmb_conservatism();
	static bool dobailout();
	static bool drop_stone_mode();
	static bool last_stop();
	static bool switch_at_req_stop();
	static bool doo2breaks();
	static int min_switch_duration();

	// Gas data
	static double bottomsac();
	static double decosac();
	static int problemsolvingtime();
	static double sacfactor();
	static bool o2narcotic();
	static double bottompo2();
	static double decopo2();
	static int bestmixend();

public slots:

	// Planning data
	static void set_planner_deco_mode(deco_mode value);
	static void set_reserve_gas(int value);
	static void set_safetystop(bool value);
	static void set_gflow(int value);
	static void set_gfhigh(int value);
	static void set_vpmb_conservatism(int value);
	static void set_dobailout(bool value);
	static void set_drop_stone_mode(bool value);
	static void set_last_stop(bool value);
	static void set_switch_at_req_stop(bool value);
	static void set_doo2breaks(bool value);
	static void set_min_switch_duration(int value);

	// Gas data
	static void set_bottomsac(double value);
	static void set_decosac(double value);
	static void set_problemsolvingtime(int value);
	static void set_sacfactor(double value);
	static void set_o2narcotic(bool value);
	static void set_bottompo2(double value);
	static void set_decopo2(double value);
	static void set_bestmixend(int value);

signals:
	// Planning data
	void planner_deco_modeChanged(deco_mode value);
	void dobailoutChanged(bool value);
	void reserve_gasChanged(int value);
	void safetystopChanged(bool value);
	void gflowChanged(int value);
	void gfhighChanged(int value);
	void vpmb_conservatismChanged(int value);
	void drop_stone_modeChanged(bool value);
	void last_stopChanged(bool value);
	void switch_at_req_stopChanged(bool value);
	void doo2breaksChanged(bool value);
	void min_switch_durationChanged(int value);

	// Gas data
	void bottomsacChanged(double value);
	void decosacChanged(double value);
	void problemsolvingtimeChanged(int value);
	void sacfactorChanged(double value);
	void o2narcoticChanged(bool value);
	void bottompo2Changed(double value);
	void decopo2Changed(double value);
	void bestmixendChanged(int value);

private:
	plannerShared() {}
};

#endif // PLANNERSHARED_H
