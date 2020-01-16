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

	// Ascend/Descend data, converted to meter/feet depending on user selection
	// Settings these will automatically update the corresponding qPrefDivePlanner
	// Variables
	Q_PROPERTY(int ascratelast6m READ ascratelast6m WRITE set_ascratelast6m NOTIFY ascratelast6mChanged);
	Q_PROPERTY(int ascratestops READ ascratestops WRITE set_ascratestops NOTIFY ascratestopsChanged);
	Q_PROPERTY(int ascrate50 READ ascrate50 WRITE set_ascrate50 NOTIFY ascrate50Changed);
	Q_PROPERTY(int ascrate75 READ ascrate75 WRITE set_ascrate75 NOTIFY ascrate75Changed);
	Q_PROPERTY(int descrate READ descrate WRITE set_descrate NOTIFY descrateChanged);

	// Planning data
	Q_PROPERTY(int planner_deco_mode READ planner_deco_mode WRITE set_planner_deco_mode NOTIFY planner_deco_modeChanged);
	Q_PROPERTY(int dive_mode READ dive_mode WRITE set_dive_mode NOTIFY dive_modeChanged);
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
	Q_PROPERTY(int surface_segment READ surface_segment WRITE set_surface_segment NOTIFY surface_segmentChanged);

	// Gas data
	Q_PROPERTY(double bottomsac READ bottomsac WRITE set_bottomsac NOTIFY bottomsacChanged);
	Q_PROPERTY(double decosac READ decosac WRITE set_decosac NOTIFY decosacChanged);
	Q_PROPERTY(int problemsolvingtime READ problemsolvingtime WRITE set_problemsolvingtime NOTIFY problemsolvingtimeChanged);
	Q_PROPERTY(double sacfactor READ sacfactor WRITE set_sacfactor NOTIFY sacfactorChanged);
	Q_PROPERTY(bool o2narcotic READ o2narcotic WRITE set_o2narcotic NOTIFY o2narcoticChanged);
	Q_PROPERTY(double bottompo2 READ bottompo2 WRITE set_bottompo2 NOTIFY bottompo2Changed);
	Q_PROPERTY(double decopo2 READ decopo2 WRITE set_decopo2 NOTIFY decopo2Changed);
	Q_PROPERTY(int bestmixend READ bestmixend WRITE set_bestmixend NOTIFY bestmixendChanged);

	// Notes data
	Q_PROPERTY(bool display_runtime READ display_runtime WRITE set_display_runtime NOTIFY display_runtimeChanged);
	Q_PROPERTY(bool display_duration READ display_duration WRITE set_display_duration NOTIFY display_durationChanged);
	Q_PROPERTY(bool display_transitions READ display_transitions WRITE set_display_transitions NOTIFY display_transitionsChanged);
	Q_PROPERTY(bool verbatim_plan READ verbatim_plan WRITE set_verbatim_plan NOTIFY verbatim_planChanged);
	Q_PROPERTY(bool display_variations READ display_variations WRITE set_display_variations NOTIFY display_variationsChanged);

public:
	static plannerShared *instance();

	// Ascend/Descend data, converted to meter/feet depending on user selection
	static int ascratelast6m();
	static int ascratestops();
	static int ascrate50();
	static int ascrate75();
	static int descrate();

	// Planning data
	static int planner_deco_mode();
	static int dive_mode();
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
	static int surface_segment();

	// Gas data
	static double bottomsac();
	static double decosac();
	static int problemsolvingtime();
	static double sacfactor();
	static bool o2narcotic();
	static double bottompo2();
	static double decopo2();
	static int bestmixend();

	// Notes data
	static bool display_runtime();
	static bool display_duration();
	static bool display_transitions();
	static bool verbatim_plan();
	static bool display_variations();

public slots:
	// Ascend/Descend data, converted to meter/feet depending on user selection
	static void set_ascratelast6m(int value);
	static void set_ascratestops(int value);
	static void set_ascrate50(int value);
	static void set_ascrate75(int value);
	static void set_descrate(int value);

	// Planning data
	static void set_planner_deco_mode(int value);
	static void set_dive_mode(int value);
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
	static void set_surface_segment(int value);

	// Gas data
	static void set_bottomsac(double value);
	static void set_decosac(double value);
	static void set_problemsolvingtime(int value);
	static void set_sacfactor(double value);
	static void set_o2narcotic(bool value);
	static void set_bottompo2(double value);
	static void set_decopo2(double value);
	static void set_bestmixend(int value);

	// Notes data
	static void set_display_runtime(bool value);
	static void set_display_duration(bool value);
	static void set_display_transitions(bool value);
	static void set_verbatim_plan(bool value);
	static void set_display_variations(bool value);

signals:
	// Ascend/Descend data, converted to meter/feet depending on user selection
	void ascratelast6mChanged(int value);
	void ascratestopsChanged(int value);
	void ascrate50Changed(int value);
	void ascrate75Changed(int value);
	void descrateChanged(int value);

	// Planning data
	void planner_deco_modeChanged(int value);
	void dive_modeChanged(int value);
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
	void surface_segmentChanged(int value);

	// Gas data
	void bottomsacChanged(double value);
	void decosacChanged(double value);
	void problemsolvingtimeChanged(int value);
	void sacfactorChanged(double value);
	void o2narcoticChanged(bool value);
	void bottompo2Changed(double value);
	void decopo2Changed(double value);
	void bestmixendChanged(int value);

	// Notes data
	void display_runtimeChanged(bool value);
	void display_durationChanged(bool value);
	void display_transitionsChanged(bool value);
	void verbatim_planChanged(bool value);
	void display_variationsChanged(bool value);

private slots:
	static void unit_lengthChangedSlot(int value);
	static void unit_volumeChangedSlot(int value);

private:
	plannerShared();

	static int divemode;
};

#endif // PLANNERSHARED_H
