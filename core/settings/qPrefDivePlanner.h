// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFDIVEPLANNER_H
#define QPREFDIVEPLANNER_H
#include "core/pref.h"

#include <QObject>

class qPrefDivePlanner : public QObject {
	Q_OBJECT
	Q_PROPERTY(int ascratelast6m READ ascratelast6m WRITE set_ascratelast6m NOTIFY ascratelast6mChanged)
	Q_PROPERTY(int ascratestops READ ascratestops WRITE set_ascratestops NOTIFY ascratestopsChanged)
	Q_PROPERTY(int ascrate50 READ ascrate50 WRITE set_ascrate50 NOTIFY ascrate50Changed)
	Q_PROPERTY(int ascrate75 READ ascrate75 WRITE set_ascrate75 NOTIFY ascrate75Changed)
	Q_PROPERTY(int bestmixend READ bestmixend WRITE set_bestmixend NOTIFY bestmixendChanged)
	Q_PROPERTY(int bottompo2 READ bottompo2 WRITE set_bottompo2 NOTIFY bottompo2Changed)
	Q_PROPERTY(int bottomsac READ bottomsac WRITE set_bottomsac NOTIFY bottomsacChanged)
	Q_PROPERTY(int decopo2 READ decopo2 WRITE set_decopo2 NOTIFY decopo2Changed)
	Q_PROPERTY(int decosac READ decosac WRITE set_decosac NOTIFY decosacChanged)
	Q_PROPERTY(int descrate READ descrate WRITE set_descrate NOTIFY descrateChanged)
	Q_PROPERTY(bool display_duration READ display_duration WRITE set_display_duration NOTIFY display_durationChanged)
	Q_PROPERTY(bool display_runtime READ display_runtime WRITE set_display_runtime NOTIFY display_runtimeChanged)
	Q_PROPERTY(bool display_transitions READ display_transitions WRITE set_display_transitions NOTIFY      display_transitionsChanged)
	Q_PROPERTY(bool display_variations READ display_variations WRITE set_display_variations NOTIFY display_variationsChanged)
	Q_PROPERTY(bool doo2breaks READ doo2breaks WRITE set_doo2breaks NOTIFY doo2breaksChanged)
	Q_PROPERTY(bool dobailout READ dobailout WRITE set_dobailout NOTIFY dobailoutChanged)
	Q_PROPERTY(bool o2narcotic READ o2narcotic WRITE set_o2narcotic NOTIFY o2narcoticChanged)
	Q_PROPERTY(bool drop_stone_mode READ drop_stone_mode WRITE set_drop_stone_mode NOTIFY drop_stone_modeChanged)
	Q_PROPERTY(bool last_stop READ last_stop WRITE set_last_stop NOTIFY last_stopChanged)
	Q_PROPERTY(int min_switch_duration READ min_switch_duration WRITE set_min_switch_duration NOTIFY min_switch_durationChanged)
	Q_PROPERTY(deco_mode planner_deco_mode READ planner_deco_mode WRITE set_planner_deco_mode NOTIFY planner_deco_modeChanged)
	Q_PROPERTY(int problemsolvingtime READ problemsolvingtime WRITE set_problemsolvingtime NOTIFY problemsolvingtimeChanged)
	Q_PROPERTY(int reserve_gas READ reserve_gas WRITE set_reserve_gas NOTIFY reserve_gasChanged)
	Q_PROPERTY(int sacfactor READ sacfactor WRITE set_sacfactor NOTIFY sacfactorChanged)
	Q_PROPERTY(bool safetystop READ safetystop WRITE set_safetystop NOTIFY safetystopChanged)
	Q_PROPERTY(bool switch_at_req_stop READ switch_at_req_stop WRITE set_switch_at_req_stop NOTIFY switch_at_req_stopChanged)
	Q_PROPERTY(bool verbatim_plan READ verbatim_plan WRITE set_verbatim_plan NOTIFY verbatim_planChanged)

public:
	static qPrefDivePlanner *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static int ascratelast6m() { return prefs.ascratelast6m; }
	static int ascratestops() { return prefs.ascratestops; }
	static int ascrate50() { return prefs.ascrate50; }
	static int ascrate75() { return prefs.ascrate75; }
	static int bestmixend() { return prefs.bestmixend.mm; }
	static int bottompo2() { return prefs.bottompo2; }
	static int bottomsac() { return prefs.bottomsac; }
	static int decopo2() { return prefs.decopo2; }
	static int decosac() { return prefs.decosac; }
	static int descrate() { return prefs.descrate; }
	static bool display_duration() { return prefs.display_duration; }
	static bool display_runtime() { return prefs.display_runtime; }
	static bool display_transitions() { return prefs.display_transitions; }
	static bool display_variations() { return prefs.display_variations; }
	static bool doo2breaks() { return prefs.doo2breaks; }
	static bool dobailout() { return prefs.dobailout; }
	static bool o2narcotic() { return prefs.o2narcotic; }
	static bool drop_stone_mode() { return prefs.drop_stone_mode; }
	static bool last_stop() { return prefs.last_stop; }
	static int min_switch_duration() { return prefs.min_switch_duration; }
	static int surface_segment() { return prefs.surface_segment; }
	static deco_mode planner_deco_mode() { return prefs.planner_deco_mode; }
	static int problemsolvingtime() { return prefs.problemsolvingtime; }
	static int reserve_gas() { return prefs.reserve_gas; }
	static int sacfactor() { return prefs.sacfactor; }
	static bool safetystop() { return prefs.safetystop; }
	static bool switch_at_req_stop() { return prefs.switch_at_req_stop; }
	static bool verbatim_plan() { return prefs.verbatim_plan; }

public slots:
	static void set_ascratelast6m(int value);
	static void set_ascratestops(int value);
	static void set_ascrate50(int value);
	static void set_ascrate75(int value);
	static void set_bestmixend(int value);
	static void set_bottompo2(int value);
	static void set_bottomsac(int value);
	static void set_decopo2(int value);
	static void set_decosac(int value);
	static void set_descrate(int value);
	static void set_display_duration(bool value);
	static void set_display_runtime(bool value);
	static void set_display_transitions(bool value);
	static void set_display_variations(bool value);
	static void set_doo2breaks(bool value);
	static void set_dobailout(bool value);
	static void set_o2narcotic(bool value);
	static void set_drop_stone_mode(bool value);
	static void set_last_stop(bool value);
	static void set_min_switch_duration(int value);
	static void set_surface_segment(int vallue);
	static void set_planner_deco_mode(deco_mode value);
	static void set_problemsolvingtime(int value);
	static void set_reserve_gas(int value);
	static void set_sacfactor(int value);
	static void set_safetystop(bool value);
	static void set_switch_at_req_stop(bool value);
	static void set_verbatim_plan(bool value);

signals:
	void ascratelast6mChanged(int value);
	void ascratestopsChanged(int value);
	void ascrate50Changed(int value);
	void ascrate75Changed(int value);
	void bestmixendChanged(int value);
	void bottompo2Changed(int value);
	void bottomsacChanged(int value);
	void decopo2Changed(int value);
	void decosacChanged(int value);
	void descrateChanged(int value);
	void display_durationChanged(bool value);
	void display_runtimeChanged(bool value);
	void display_transitionsChanged(bool value);
	void display_variationsChanged(bool value);
	void doo2breaksChanged(bool value);
	void dobailoutChanged(bool value);
	void o2narcoticChanged(bool value);
	void drop_stone_modeChanged(bool value);
	void last_stopChanged(bool value);
	void min_switch_durationChanged(int value);
	void surface_segmentChanged(int value);
	void planner_deco_modeChanged(deco_mode value);
	void problemsolvingtimeChanged(int value);
	void reserve_gasChanged(int value);
	void sacfactorChanged(int value);
	void safetystopChanged(bool value);
	void switch_at_req_stopChanged(bool value);
	void verbatim_planChanged(bool value);

private:
	qPrefDivePlanner() {}

	static void disk_ascratelast6m(bool doSync);
	static void disk_ascratestops(bool doSync);
	static void disk_ascrate50(bool doSync);
	static void disk_ascrate75(bool doSync);
	static void disk_bestmixend(bool doSync);
	static void disk_bottompo2(bool doSync);
	static void disk_bottomsac(bool doSync);
	static void disk_decopo2(bool doSync);
	static void disk_decosac(bool doSync);
	static void disk_descrate(bool doSync);
	static void disk_display_deco_mode(bool doSync);
	static void disk_display_duration(bool doSync);
	static void disk_display_runtime(bool doSync);
	static void disk_display_transitions(bool doSync);
	static void disk_display_variations(bool doSync);
	static void disk_doo2breaks(bool doSync);
	static void disk_dobailout(bool doSync);
	static void disk_o2narcotic(bool doSync);
	static void disk_drop_stone_mode(bool doSync);
	static void disk_last_stop(bool doSync);
	static void disk_min_switch_duration(bool doSync);
	static void disk_surface_segment(bool doSync);
	static void disk_planner_deco_mode(bool doSync);
	static void disk_problemsolvingtime(bool doSync);
	static void disk_reserve_gas(bool doSync);
	static void disk_sacfactor(bool doSync);
	static void disk_safetystop(bool doSync);
	static void disk_switch_at_req_stop(bool doSync);
	static void disk_verbatim_plan(bool doSync);
};

#endif
