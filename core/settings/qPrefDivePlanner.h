// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFDIVEPLANNER_H
#define QPREFDIVEPLANNER_H

#include "core/pref.h"
#include <QObject>


class qPrefDivePlanner : public QObject {
	Q_OBJECT
	Q_PROPERTY(int asc_rate_last_6m READ ascratelast6m WRITE setAscratelast6m NOTIFY ascratelast6mChanged);
	Q_PROPERTY(int asc_rate_stops READ ascratestops WRITE setAscratestops NOTIFY ascratestopsChanged);
	Q_PROPERTY(int asc_rate_50 READ ascrate50 WRITE setAscrate50 NOTIFY ascrate50Changed);
	Q_PROPERTY(int asc_rate_75 READ ascrate75 WRITE setAscrate75 NOTIFY ascrate75Changed);
	Q_PROPERTY(int bestmix_end READ bestmixend WRITE setBestmixend NOTIFY bestmixendChanged);
	Q_PROPERTY(int bottom_po2 READ bottompo2 WRITE setBottompo2 NOTIFY bottompo2Changed);
	Q_PROPERTY(int bottom_sac READ bottomSac WRITE setBottomSac NOTIFY bottomSacChanged);
	Q_PROPERTY(deco_mode deco_mode READ decoMode WRITE setDecoMode NOTIFY decoModeChanged);
	Q_PROPERTY(int deco_po2 READ decopo2 WRITE setDecopo2 NOTIFY decopo2Changed);
	Q_PROPERTY(int deco_sac READ decoSac WRITE setDecoSac NOTIFY decoSacChanged);
	Q_PROPERTY(int desc_rate READ descrate WRITE setDescrate NOTIFY descrateChanged);
	Q_PROPERTY(bool display_duration READ displayDuration WRITE setDisplayDuration NOTIFY displayDurationChanged);
	Q_PROPERTY(bool display_runtime READ displayRuntime WRITE setDisplayRuntime NOTIFY displayRuntimeChanged);
	Q_PROPERTY(bool display_transitions READ displayTransitions WRITE setDisplayTransitions NOTIFY      displayTransitionsChanged);
	Q_PROPERTY(bool display_variations READ displayVariations WRITE setDisplayVariations NOTIFY displayVariationsChanged);
	Q_PROPERTY(bool planner_doo2_breaks READ doo2breaks WRITE setDoo2breaks NOTIFY doo2breaksChanged);
	Q_PROPERTY(bool drop_stone_mode READ dropStoneMode WRITE setDropStoneMode NOTIFY dropStoneModeChanged);
	Q_PROPERTY(bool last_stop READ lastStop WRITE setLastStop NOTIFY lastStopChanged);
	Q_PROPERTY(int min_switch_duration READ minSwitchDuration WRITE setMinSwitchDuration NOTIFY minSwitchDurationChanged);
	Q_PROPERTY(int problem_solving_time READ problemSolvingTime WRITE setProblemSolvingTime NOTIFY problemSolvingTimeChanged);
	Q_PROPERTY(int reserve_gas READ reserveGas WRITE setReserveGas NOTIFY reserveGasChanged);
	Q_PROPERTY(int sac_factor READ sacFactor WRITE setSacFactor NOTIFY sacFactorChanged);
	Q_PROPERTY(bool safety_stop READ safetyStop WRITE setSafetyStop NOTIFY safetyStopChanged);
	Q_PROPERTY(bool switch_at_req_stop READ switchAtRequiredStop WRITE setSwitchAtRequiredStop NOTIFY switchAtRequiredStopChanged);
	Q_PROPERTY(bool verbatim_plan READ verbatimPlan WRITE setVerbatimPlan NOTIFY verbatimPlanChanged);

public:
	qPrefDivePlanner(QObject *parent = NULL) : QObject(parent) {};
	~qPrefDivePlanner() {};
	static qPrefDivePlanner *instance();

// Load/Sync local settings (disk) and struct preference
void loadSync(bool doSync);

public:
	int ascratelast6m() const;
	int ascratestops() const;
	int ascrate50() const;
	int ascrate75() const;
	int bestmixend() const;
	int bottompo2() const;
	int bottomSac() const;
	deco_mode decoMode() const;
	int decopo2() const;
	int decoSac() const;
	int descrate() const;
	bool displayDuration() const;
	bool displayRuntime() const;
	bool displayTransitions() const;
	bool displayVariations() const;
	bool doo2breaks() const;
	bool dropStoneMode() const;
	bool lastStop() const;
	int minSwitchDuration() const;
	int problemSolvingTime() const;
	int reserveGas() const;
	int sacFactor() const;
	bool safetyStop() const;
	bool switchAtRequiredStop() const;
	bool verbatimPlan() const;

public slots:
	void setAscratelast6m(int value);
	void setAscratestops(int value);
	void setAscrate50(int value);
	void setAscrate75(int value);
	void setBestmixend(int value);
	void setBottompo2(int value);
	void setBottomSac(int value);
	void setDecoMode(deco_mode value);
	void setDecopo2(int value);
	void setDecoSac(int value);
	void setDescrate(int value);
	void setDisplayDuration(bool value);
	void setDisplayRuntime(bool value);
	void setDisplayTransitions(bool value);
	void setDisplayVariations(bool value);
	void setDoo2breaks(bool value);
	void setDropStoneMode(bool value);
	void setLastStop(bool value);
	void setMinSwitchDuration(int value);
	void setProblemSolvingTime(int value);
	void setReserveGas(int value);
	void setSacFactor(int value);
	void setSafetyStop(bool value);
	void setSwitchAtRequiredStop(bool value);
	void setVerbatimPlan(bool value);

signals:
	void ascratelast6mChanged(int value);
	void ascratestopsChanged(int value);
	void ascrate50Changed(int value);
	void ascrate75Changed(int value);
	void bestmixendChanged(int value);
	void bottompo2Changed(int value);
	void bottomSacChanged(int value);
	void decoModeChanged(deco_mode value);
	void decopo2Changed(int value);
	void decoSacChanged(int value);
	void descrateChanged(int value);
	void displayDurationChanged(bool value);
	void displayRuntimeChanged(bool value);
	void displayTransitionsChanged(bool value);
	void displayVariationsChanged(bool value);
	void doo2breaksChanged(bool value);
	void dropStoneModeChanged(bool value);
	void lastStopChanged(bool value);
	void minSwitchDurationChanged(int value);
	void problemSolvingTimeChanged(int value);
	void reserveGasChanged(int value);
	void sacFactorChanged(int value);
	void safetyStopChanged(bool value);
	void switchAtRequiredStopChanged(bool value);
	void verbatimPlanChanged(bool value);

private:
	const QString group = QStringLiteral("Planner");
	static qPrefDivePlanner *m_instance;

	void diskAscratelast6m(bool doSync);
	void diskAscratestops(bool doSync);
	void diskAscrate50(bool doSync);
	void diskAscrate75(bool doSync);
	void diskBestmixend(bool doSync);
	void diskBottompo2(bool doSync);
	void diskBottomSac(bool doSync);
	void diskDecoMode(bool doSync);
	void diskDecopo2(bool doSync);
	void diskDecoSac(bool doSync);
	void diskDescrate(bool doSync);
	void diskDisplayDuration(bool doSync);
	void diskDisplayRuntime(bool doSync);
	void diskDisplayTransitions(bool doSync);
	void diskDisplayVariations(bool doSync);
	void diskDoo2Breaks(bool doSync);
	void diskDropStoneMode(bool doSync);
	void diskLastStop(bool doSync);
	void diskMinSwitchDuration(bool doSync);
	void diskProblemSolvingTime(bool doSync);
	void diskReserveGas(bool doSync);
	void diskSacFactor(bool doSync);
	void diskSafetyStop(bool doSync);
	void diskSwitchAtRequiredStop(bool doSync);
	void diskVerbatimPlan(bool doSync);
};

#endif
