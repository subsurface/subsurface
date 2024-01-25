// SPDX-License-Identifier: GPL-2.0
#ifndef QMLINTERFACE_H
#define QMLINTERFACE_H
#include "core/qthelper.h"
#include "core/downloadfromdcthread.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefUnit.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefDiveComputer.h"
#include "qt-models/diveplannermodel.h"
#include "backend-shared/plannershared.h"

#include <QObject>
#include <QQmlContext>
#include <QStringList>

// This class is a pure interface class and may not contain any implementation code
// Allowed are:
//     header
//        Q_PROPERTY
//        signal/slot for Q_PROPERTY functions
//        the functions may contain either
//             a) a function call to the implementation
//             b) a reference to a global variable like e.g. prefs.
//        Q_INVOCABLE functions
//        the functions may contain
//             a) a function call to the implementation
//    source
//        connect signal/signal to pass signals from implementation


class QMLInterface : public QObject {
	Q_OBJECT

	// Q_PROPERTY used in QML
	Q_PROPERTY(CLOUD_STATUS cloud_verification_status READ cloud_verification_status WRITE set_cloud_verification_status NOTIFY cloud_verification_statusChanged)
	Q_PROPERTY(DURATION duration_units READ duration_units WRITE set_duration_units NOTIFY duration_unitsChanged)
	Q_PROPERTY(LENGTH length READ length WRITE set_length NOTIFY lengthChanged)
	Q_PROPERTY(PRESSURE pressure READ pressure WRITE set_pressure NOTIFY pressureChanged)
	Q_PROPERTY(TEMPERATURE temperature READ temperature WRITE set_temperature NOTIFY temperatureChanged)
	Q_PROPERTY(UNIT_SYSTEM unit_system READ unit_system WRITE set_unit_system NOTIFY unit_systemChanged)
	Q_PROPERTY(TIME vertical_speed_time READ vertical_speed_time WRITE set_vertical_speed_time NOTIFY vertical_speed_timeChanged)
	Q_PROPERTY(VOLUME volume READ volume WRITE set_volume NOTIFY volumeChanged)
	Q_PROPERTY(WEIGHT weight READ weight WRITE set_weight NOTIFY weightChanged)

	Q_PROPERTY(int ascratelast6m READ ascratelast6m WRITE set_ascratelast6m NOTIFY ascratelast6mChanged);
	Q_PROPERTY(int ascratestops READ ascratestops WRITE set_ascratestops NOTIFY ascratestopsChanged);
	Q_PROPERTY(int ascrate50 READ ascrate50 WRITE set_ascrate50 NOTIFY ascrate50Changed);
	Q_PROPERTY(int ascrate75 READ ascrate75 WRITE set_ascrate75 NOTIFY ascrate75Changed);
	Q_PROPERTY(int descrate READ descrate WRITE set_descrate NOTIFY descrateChanged);

	Q_PROPERTY(DIVE_MODE dive_mode READ dive_mode WRITE set_dive_mode NOTIFY dive_modeChanged);
	Q_PROPERTY(DECO_MODE planner_deco_mode READ planner_deco_mode WRITE set_planner_deco_mode NOTIFY planner_deco_modeChanged);
	Q_PROPERTY(int reserve_gas READ reserve_gas WRITE set_reserve_gas NOTIFY reserve_gasChanged);
	Q_PROPERTY(bool safetystop READ safetystop WRITE set_safetystop NOTIFY safetystopChanged);
	Q_PROPERTY(int planner_gflow READ planner_gflow WRITE set_planner_gflow NOTIFY planner_gflowChanged);
	Q_PROPERTY(int planner_gfhigh READ planner_gfhigh WRITE set_planner_gfhigh NOTIFY planner_gfhighChanged);
	Q_PROPERTY(int vpmb_conservatism READ vpmb_conservatism WRITE set_vpmb_conservatism NOTIFY vpmb_conservatismChanged);
	Q_PROPERTY(bool dobailout READ dobailout WRITE set_dobailout NOTIFY dobailoutChanged);
	Q_PROPERTY(bool drop_stone_mode READ drop_stone_mode WRITE set_drop_stone_mode NOTIFY drop_stone_modeChanged);
	Q_PROPERTY(bool last_stop6m READ last_stop6m WRITE set_last_stop6m NOTIFY last_stop6mChanged);
	Q_PROPERTY(bool switch_at_req_stop READ switch_at_req_stop WRITE set_switch_at_req_stop NOTIFY switch_at_req_stopChanged);
	Q_PROPERTY(bool doo2breaks READ doo2breaks WRITE set_doo2breaks NOTIFY doo2breaksChanged);
	Q_PROPERTY(int min_switch_duration READ min_switch_duration WRITE set_min_switch_duration NOTIFY min_switch_durationChanged);
	Q_PROPERTY(int surface_segment READ surface_segment WRITE set_surface_segment NOTIFY surface_segmentChanged);

	Q_PROPERTY(int bottomsac READ bottomsac WRITE set_bottomsac NOTIFY bottomsacChanged);
	Q_PROPERTY(int decosac READ decosac WRITE set_decosac NOTIFY decosacChanged);
	Q_PROPERTY(int problemsolvingtime READ problemsolvingtime WRITE set_problemsolvingtime NOTIFY problemsolvingtimeChanged);
	Q_PROPERTY(int sacfactor READ sacfactor WRITE set_sacfactor NOTIFY sacfactorChanged);
	Q_PROPERTY(bool o2narcotic READ o2narcotic WRITE set_o2narcotic NOTIFY o2narcoticChanged);
	Q_PROPERTY(int bottompo2 READ bottompo2 WRITE set_bottompo2 NOTIFY bottompo2Changed);
	Q_PROPERTY(int decopo2 READ decopo2 WRITE set_decopo2 NOTIFY decopo2Changed);
	Q_PROPERTY(int bestmixend READ bestmixend WRITE set_bestmixend NOTIFY bestmixendChanged);

	Q_PROPERTY(bool display_runtime READ display_runtime WRITE set_display_runtime NOTIFY display_runtimeChanged);
	Q_PROPERTY(bool display_duration READ display_duration WRITE set_display_duration NOTIFY display_durationChanged);
	Q_PROPERTY(bool display_transitions READ display_transitions WRITE set_display_transitions NOTIFY display_transitionsChanged);
	Q_PROPERTY(bool verbatim_plan READ verbatim_plan WRITE set_verbatim_plan NOTIFY verbatim_planChanged);
	Q_PROPERTY(bool display_variations READ display_variations WRITE set_display_variations NOTIFY display_variationsChanged);

	Q_PROPERTY(bool sync_dc_time READ sync_dc_time WRITE set_sync_dc_time NOTIFY sync_dc_timeChanged);

public:
	// function to do the needed setup
	static void setup(QQmlContext *ct);

	// Duplicated enums, these enums are properly defined in the C/C++ structure
	// but duplicated here to make them available to QML.

	// Duplicating the enums poses a slight risk for forgetting to update
	// them if the proper enum is changed (e.g. assigning a new start value).

	// remark please do not use these enums outside the C++/QML interface.
	enum UNIT_SYSTEM {
		METRIC,
		IMPERIAL,
		PERSONALIZE
	};
	Q_ENUM(UNIT_SYSTEM);

	enum LENGTH {
		METERS,
		FEET
	};
	Q_ENUM(LENGTH);

	enum VOLUME {
		LITER,
		CUFT
	};
	Q_ENUM(VOLUME);

	enum PRESSURE {
		BAR,
		PSI,
		PASCALS
	};
	Q_ENUM(PRESSURE);

	enum TEMPERATURE {
		CELSIUS,
		FAHRENHEIT,
		KELVIN
	};
	Q_ENUM(TEMPERATURE);

	enum WEIGHT {
		KG,
		LBS
	};
	Q_ENUM(WEIGHT);

	enum TIME {
		SECONDS,
		MINUTES
	};
	Q_ENUM(TIME);

	enum DURATION {
		MIXED,
		MINUTES_ONLY,
		ALWAYS_HOURS
	};
	Q_ENUM(DURATION);

	enum CLOUD_STATUS {
			CS_UNKNOWN,
			CS_INCORRECT_USER_PASSWD,
			CS_NEED_TO_VERIFY,
			CS_VERIFIED,
			CS_NOCLOUD
	};
	Q_ENUM(CLOUD_STATUS);

	enum DECO_MODE {
		BUEHLMANN,
		RECREATIONAL,
		VPMB
	};
	Q_ENUM(DECO_MODE);

	enum DIVE_MODE {
		OC,
		CCR,
		PSCR,
		FREEDIVE
	};
	Q_ENUM(DIVE_MODE);

public:
	CLOUD_STATUS cloud_verification_status() { return (CLOUD_STATUS)prefs.cloud_verification_status; }
	DURATION duration_units() { return (DURATION)prefs.units.duration_units; }
	LENGTH length() { return (LENGTH)prefs.units.length; }
	PRESSURE pressure() { return (PRESSURE)prefs.units.pressure; }
	TEMPERATURE temperature() { return (TEMPERATURE)prefs.units.temperature; }
	UNIT_SYSTEM unit_system() { return (UNIT_SYSTEM)prefs.unit_system; }
	TIME vertical_speed_time() { return (TIME)prefs.units.vertical_speed_time; }
	VOLUME volume() { return (VOLUME)prefs.units.volume; }
	WEIGHT weight() { return (WEIGHT)prefs.units.weight; }

	int ascratelast6m() { return DivePlannerPointsModel::instance()->ascratelast6mDisplay(); }
	int ascratestops() { return DivePlannerPointsModel::instance()->ascratestopsDisplay(); }
	int ascrate50() { return DivePlannerPointsModel::instance()->ascrate50Display(); }
	int ascrate75() { return DivePlannerPointsModel::instance()->ascrate75Display(); }
	int descrate() { return DivePlannerPointsModel::instance()->descrateDisplay(); }

	DIVE_MODE dive_mode() { return OC; }
	DECO_MODE planner_deco_mode() { return (DECO_MODE)PlannerShared::planner_deco_mode(); }
	int reserve_gas() { return PlannerShared::reserve_gas(); }
	bool safetystop() { return prefs.safetystop; }
	int planner_gflow() { return DivePlannerPointsModel::instance()->gfLow(); }
	int planner_gfhigh() { return DivePlannerPointsModel::instance()->gfHigh(); }
	int vpmb_conservatism() { return prefs.vpmb_conservatism; }
	bool dobailout() { return PlannerShared::dobailout(); }
	bool drop_stone_mode() { return prefs.drop_stone_mode; }
	bool last_stop6m() { return prefs.last_stop; }
	bool switch_at_req_stop() { return prefs.switch_at_req_stop; }
	bool doo2breaks() { return PlannerShared::doo2breaks(); }
	int min_switch_duration() { return PlannerShared::min_switch_duration(); }
	int surface_segment() { return PlannerShared::surface_segment(); }

	int bottomsac() { return (int)PlannerShared::bottomsac(); }
	int decosac() { return (int)PlannerShared::decosac(); }
	int problemsolvingtime() { return prefs.problemsolvingtime; }
	int sacfactor() { return (int)PlannerShared::sacfactor(); }
	bool o2narcotic() { return (int)PlannerShared::o2narcotic(); }
	int bottompo2() { return (int)PlannerShared::bottompo2(); }
	int decopo2() { return (int)PlannerShared::decopo2(); }
	int bestmixend() { return PlannerShared::bestmixend(); }

	bool display_runtime() { return prefs.display_runtime; }
	bool display_duration() { return prefs.display_duration; }
	bool display_transitions() { return prefs.display_transitions; }
	bool verbatim_plan() { return prefs.verbatim_plan; }
	bool display_variations() { return prefs.display_variations; }

	bool sync_dc_time() { return prefs.sync_dc_time; }

public slots:
	void set_cloud_verification_status(CLOUD_STATUS value) {  qPrefCloudStorage::set_cloud_verification_status(value); }
	void set_duration_units(DURATION value) { qPrefUnits::set_duration_units((units::DURATION)value); }
	void set_length(LENGTH value) { qPrefUnits::set_length((units::LENGTH)value); }
	void set_pressure(PRESSURE value) { qPrefUnits::set_pressure((units::PRESSURE)value); }
	void set_temperature(TEMPERATURE value) { qPrefUnits::set_temperature((units::TEMPERATURE)value); }
	void set_unit_system(UNIT_SYSTEM value) { qPrefUnits::set_unit_system((unit_system_values)value); }
	void set_vertical_speed_time(TIME value) { qPrefUnits::set_vertical_speed_time((units::TIME)value); }
	void set_volume(VOLUME value) { qPrefUnits::set_volume((units::VOLUME)value); }
	void set_weight(WEIGHT value) { qPrefUnits::set_weight((units::WEIGHT)value); }

	void set_ascratelast6m(int value) { DivePlannerPointsModel::instance()->setAscratelast6mDisplay(value); }
	void set_ascratestops(int value) { DivePlannerPointsModel::instance()->setAscratestopsDisplay(value); }
	void set_ascrate50(int value) { DivePlannerPointsModel::instance()->setAscrate50Display(value); }
	void set_ascrate75(int value) { DivePlannerPointsModel::instance()->setAscrate75Display(value); }
	void set_descrate(int value) { DivePlannerPointsModel::instance()->setDescrateDisplay(value); }

	void set_dive_mode(DIVE_MODE value) { DivePlannerPointsModel::instance()->setRebreatherMode((int)value); }
	void set_planner_deco_mode(DECO_MODE value) { PlannerShared::set_planner_deco_mode((deco_mode)value); }
	void set_reserve_gas(int value) { PlannerShared::set_reserve_gas(value); }
	void set_safetystop(bool value) { DivePlannerPointsModel::instance()->setSafetyStop(value); }
	void set_planner_gflow(int value) { DivePlannerPointsModel::instance()->setGFLow(value); }
	void set_planner_gfhigh(int value) { DivePlannerPointsModel::instance()->setGFHigh(value); }
	void set_vpmb_conservatism(int value) { DivePlannerPointsModel::instance()->setVpmbConservatism(value); }
	void set_dobailout(bool value) { PlannerShared::set_dobailout(value); }
	void set_drop_stone_mode(bool value) { DivePlannerPointsModel::instance()->setDropStoneMode(value); }
	void set_last_stop6m(bool value) { DivePlannerPointsModel::instance()->setLastStop6m(value); }
	void set_switch_at_req_stop(bool value) { DivePlannerPointsModel::instance()->setSwitchAtReqStop(value); }
	void set_doo2breaks(bool value) { PlannerShared::set_doo2breaks(value); }
	void set_min_switch_duration(int value) { PlannerShared::set_min_switch_duration(value); }
	void set_surface_segment(int value) { PlannerShared::set_surface_segment(value); }

	void set_bottomsac(int value) { PlannerShared::set_bottomsac((double)value); }
	void set_decosac(int value) { PlannerShared::set_decosac((double)value); }
	void set_problemsolvingtime(int value) { DivePlannerPointsModel::instance()->setProblemSolvingTime(value); }
	void set_sacfactor(int value) { PlannerShared::set_sacfactor((double)value); }
	void set_o2narcotic(bool value) { PlannerShared::set_o2narcotic(value); }
	void set_bottompo2(int value) { PlannerShared::set_bottompo2((double)value); }
	void set_decopo2(int value) { PlannerShared::set_decopo2((double)value); }
	void set_bestmixend(int value) { PlannerShared::set_bestmixend(value); }

	void set_display_runtime(bool value) { DivePlannerPointsModel::instance()->setDisplayRuntime(value); }
	void set_display_duration(bool value) { DivePlannerPointsModel::instance()->setDisplayDuration(value); }
	void set_display_transitions(bool value) { DivePlannerPointsModel::instance()->setDisplayTransitions(value); }
	void set_verbatim_plan(bool value) { DivePlannerPointsModel::instance()->setVerbatim(value); }
	void set_display_variations(bool value) { DivePlannerPointsModel::instance()->setDisplayVariations(value); }
	void set_sync_dc_time(bool value) {
		qPrefDiveComputer::set_sync_dc_time(value);
		DCDeviceData::instance()->setSyncTime(value);
	}
	QString firstDiveDate() { return get_first_dive_date_string(); }
	QString lastDiveDate() { return get_last_dive_date_string(); }

signals:
	void cloud_verification_statusChanged(CLOUD_STATUS);
	void duration_unitsChanged(DURATION);
	void lengthChanged(LENGTH);
	void pressureChanged(PRESSURE);
	void temperatureChanged(TEMPERATURE);
	void unit_systemChanged(UNIT_SYSTEM);
	void vertical_speed_timeChanged(TIME);
	void volumeChanged(VOLUME);
	void weightChanged(WEIGHT);

	void ascratelast6mChanged(int);
	void ascratestopsChanged(int);
	void ascrate50Changed(int);
	void ascrate75Changed(int);
	void descrateChanged(int);

	void dive_modeChanged(DIVE_MODE value);
	void planner_deco_modeChanged(DECO_MODE value);
	void reserve_gasChanged(int value);
	void safetystopChanged(bool value);
	void planner_gflowChanged(int value);
	void planner_gfhighChanged(int value);
	void vpmb_conservatismChanged(int value);
	void dobailoutChanged(bool value);
	void drop_stone_modeChanged(bool value);
	void last_stop6mChanged(bool value);
	void switch_at_req_stopChanged(bool value);
	void doo2breaksChanged(bool value);
	void min_switch_durationChanged(int value);
	void surface_segmentChanged(int value);

	void bottomsacChanged(int value);
	void decosacChanged(int value);
	void sacfactorChanged(int value);
	void problemsolvingtimeChanged(int value);
	void o2narcoticChanged(bool value);
	void bottompo2Changed(int value);
	void decopo2Changed(int value);
	void bestmixendChanged(int value);

	void display_runtimeChanged(bool value);
	void display_durationChanged(bool value);
	void display_transitionsChanged(bool value);
	void verbatim_planChanged(bool value);
	void display_variationsChanged(bool value);

	void sync_dc_timeChanged(bool value);
private:
	QMLInterface();
};
#endif // QMLINTERFACE_H
