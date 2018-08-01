// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFDIVEPLANNER_H
#define QPREFDIVEPLANNER_H
#include "core/pref.h"

#include <QObject>

class qPrefDivePlanner : public QObject {
	Q_OBJECT
	Q_PROPERTY(int ascratelast6m READ ascratelast6m WRITE set_ascratelast6m NOTIFY ascratelast6m_changed);
	Q_PROPERTY(int ascratestops READ ascratestops WRITE set_ascratestops NOTIFY ascratestops_changed);
	Q_PROPERTY(int ascrate50 READ ascrate50 WRITE set_ascrate50 NOTIFY ascrate50_changed);
	Q_PROPERTY(int ascrate75 READ ascrate75 WRITE set_ascrate75 NOTIFY ascrate75_changed);
	Q_PROPERTY(depth_t bestmixend READ bestmixend WRITE set_bestmixend NOTIFY bestmixend_changed);
	Q_PROPERTY(int bottompo2 READ bottompo2 WRITE set_bottompo2 NOTIFY bottompo2_changed);
	Q_PROPERTY(int bottomsac READ bottomsac WRITE set_bottomsac NOTIFY bottomsac_changed);
	Q_PROPERTY(int decopo2 READ decopo2 WRITE set_decopo2 NOTIFY decopo2_changed);
	Q_PROPERTY(int decosac READ decosac WRITE set_decosac NOTIFY decosac_changed);
	Q_PROPERTY(int descrate READ descrate WRITE set_descrate NOTIFY descrate_changed);
	Q_PROPERTY(bool display_duration READ display_duration WRITE set_display_duration NOTIFY display_duration_changed);
	Q_PROPERTY(bool display_runtime READ display_runtime WRITE set_display_runtime NOTIFY display_runtime_changed);
	Q_PROPERTY(bool display_transitions READ display_transitions WRITE set_display_transitions NOTIFY      display_transitions_changed);
	Q_PROPERTY(bool display_variations READ display_variations WRITE set_display_variations NOTIFY display_variations_changed);
	Q_PROPERTY(bool doo2breaks READ doo2breaks WRITE set_doo2breaks NOTIFY doo2breaks_changed);
	Q_PROPERTY(bool drop_stone_mode READ drop_stone_mode WRITE set_drop_stone_mode NOTIFY drop_stone_mode_changed);
	Q_PROPERTY(bool last_stop READ last_stop WRITE set_last_stop NOTIFY last_stop_changed);
	Q_PROPERTY(int min_switch_duration READ min_switch_duration WRITE set_min_switch_duration NOTIFY min_switch_duration_changed);
	Q_PROPERTY(deco_mode planner_deco_mode READ planner_deco_mode WRITE set_planner_deco_mode NOTIFY planner_deco_mode_changed);
	Q_PROPERTY(int problemsolvingtime READ problemsolvingtime WRITE set_problemsolvingtime NOTIFY problemsolvingtime_changed);
	Q_PROPERTY(int reserve_gas READ reserve_gas WRITE set_reserve_gas NOTIFY reserve_gas_changed);
	Q_PROPERTY(int sacfactor READ sacfactor WRITE set_sacfactor NOTIFY sacfactor_changed);
	Q_PROPERTY(bool safetystop READ safetystop WRITE set_safetystop NOTIFY safetystop_changed);
	Q_PROPERTY(bool switch_at_req_stop READ switch_at_req_stop WRITE set_switch_at_req_stop NOTIFY switch_at_req_stop_changed);
	Q_PROPERTY(bool verbatim_plan READ verbatim_plan WRITE set_verbatim_plan NOTIFY verbatim_plan_changed);

public:
	qPrefDivePlanner(QObject *parent = NULL);
	static qPrefDivePlanner *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	static int ascratelast6m() { return prefs.ascratelast6m; }
	static int ascratestops() { return prefs.ascratestops; }
	static int ascrate50() { return prefs.ascrate50; }
	static int ascrate75() { return prefs.ascrate75; }
	static depth_t bestmixend() { return prefs.bestmixend; }
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
	static bool drop_stone_mode() { return prefs.drop_stone_mode; }
	static bool last_stop() { return prefs.last_stop; }
	static int min_switch_duration() { return prefs.min_switch_duration; }
	static deco_mode planner_deco_mode() { return prefs.planner_deco_mode; }
	static int problemsolvingtime() { return prefs.problemsolvingtime; }
	static int reserve_gas() { return prefs.reserve_gas; }
	static int sacfactor() { return prefs.sacfactor; }
	static bool safetystop() { return prefs.safetystop; }
	static bool switch_at_req_stop() { return prefs.switch_at_req_stop; }
	static bool verbatim_plan() { return prefs.verbatim_plan; }

public slots:
	void set_ascratelast6m(int value);
	void set_ascratestops(int value);
	void set_ascrate50(int value);
	void set_ascrate75(int value);
	void set_bestmixend(depth_t value);
	void set_bottompo2(int value);
	void set_bottomsac(int value);
	void set_decopo2(int value);
	void set_decosac(int value);
	void set_descrate(int value);
	void set_display_duration(bool value);
	void set_display_runtime(bool value);
	void set_display_transitions(bool value);
	void set_display_variations(bool value);
	void set_doo2breaks(bool value);
	void set_drop_stone_mode(bool value);
	void set_last_stop(bool value);
	void set_min_switch_duration(int value);
	void set_planner_deco_mode(deco_mode value);
	void set_problemsolvingtime(int value);
	void set_reserve_gas(int value);
	void set_sacfactor(int value);
	void set_safetystop(bool value);
	void set_switch_at_req_stop(bool value);
	void set_verbatim_plan(bool value);

signals:
	void ascratelast6m_changed(int value);
	void ascratestops_changed(int value);
	void ascrate50_changed(int value);
	void ascrate75_changed(int value);
	void bestmixend_changed(depth_t value);
	void bottompo2_changed(int value);
	void bottomsac_changed(int value);
	void decopo2_changed(int value);
	void decosac_changed(int value);
	void descrate_changed(int value);
	void display_duration_changed(bool value);
	void display_runtime_changed(bool value);
	void display_transitions_changed(bool value);
	void display_variations_changed(bool value);
	void doo2breaks_changed(bool value);
	void drop_stone_mode_changed(bool value);
	void last_stop_changed(bool value);
	void min_switch_duration_changed(int value);
	void planner_deco_mode_changed(deco_mode value);
	void problemsolvingtime_changed(int value);
	void reserve_gas_changed(int value);
	void sacfactor_changed(int value);
	void safetystop_changed(bool value);
	void switch_at_req_stop_changed(bool value);
	void verbatim_plan_changed(bool value);

private:
	void disk_ascratelast6m(bool doSync);
	void disk_ascratestops(bool doSync);
	void disk_ascrate50(bool doSync);
	void disk_ascrate75(bool doSync);
	void disk_bestmixend(bool doSync);
	void disk_bottompo2(bool doSync);
	void disk_bottomsac(bool doSync);
	void disk_decopo2(bool doSync);
	void disk_decosac(bool doSync);
	void disk_descrate(bool doSync);
	void disk_display_deco_mode(bool doSync);
	void disk_display_duration(bool doSync);
	void disk_display_runtime(bool doSync);
	void disk_display_transitions(bool doSync);
	void disk_display_variations(bool doSync);
	void disk_doo2breaks(bool doSync);
	void disk_drop_stone_mode(bool doSync);
	void disk_last_stop(bool doSync);
	void disk_min_switch_duration(bool doSync);
	void disk_planner_deco_mode(bool doSync);
	void disk_problemsolvingtime(bool doSync);
	void disk_reserve_gas(bool doSync);
	void disk_sacfactor(bool doSync);
	void disk_safetystop(bool doSync);
	void disk_switch_at_req_stop(bool doSync);
	void disk_verbatim_plan(bool doSync);
};

#endif
