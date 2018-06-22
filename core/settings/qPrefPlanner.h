// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPLANNER_H
#define QPREFPLANNER_H

#include <QObject>


class qPrefPlanner : public QObject {
	Q_OBJECT
	Q_PROPERTY(int			asc_rate_last_6m
				READ		ascRateLast6m
				WRITE		setAscRateLast6m
				NOTIFY      ascRateLast6mChanged);
	Q_PROPERTY(int			asc_rate_stops
				READ		ascRateStops
				WRITE		setAscRateStops
				NOTIFY      ascRateStopsChanged);
	Q_PROPERTY(int			asc_rate_50
				READ		ascRate50
				WRITE		setAscRate50
				NOTIFY      ascRate50Changed);
	Q_PROPERTY(int			asc_rate_75
				READ		ascRate75
				WRITE		setAscRate75
				NOTIFY      ascRate75Changed);
	Q_PROPERTY(int			bestmix_end
				READ		bestmixEnd
				WRITE		setBestmixEnd
				NOTIFY      bestmixEndChanged);
	Q_PROPERTY(int			bottom_po2
				READ		bottomPo2
				WRITE		setBottomPo2
				NOTIFY      bottomPo2Changed);
	Q_PROPERTY(int			bottom_sac
				READ		bottomSac
				WRITE		setBottomSac
				NOTIFY      bottomSacChanged);
	Q_PROPERTY(deco_mode	deco_mode
				READ		decoMode
				WRITE		setDecoMode
				NOTIFY      decoModeChanged);
	Q_PROPERTY(int			deco_po2
				READ		decoPo2
				WRITE		setDecoPo2
				NOTIFY      decoPo2Changed);
	Q_PROPERTY(int			deco_sac
				READ		decoSac
				WRITE		setDecoSac
				NOTIFY      decoSacChanged);
	Q_PROPERTY(int			desc_rate
				READ		descRate
				WRITE		setDescRate
				NOTIFY      descRateChanged);
	Q_PROPERTY(bool			display_duration
				READ		displayDuration
				WRITE		setDisplayDuration
				NOTIFY      displayDurationChanged);
	Q_PROPERTY(bool			display_runtime
				READ		displayRuntime
				WRITE		setDisplayRuntime
				NOTIFY      displayRuntimeChanged);
	Q_PROPERTY(bool			display_transitions
				READ		displayTransitions
				WRITE		setDisplayTransitions
				NOTIFY      displayTransitionsChanged);
	Q_PROPERTY(bool			display_variations
				READ		displayVariations
				WRITE		setDisplayVariations
				NOTIFY      displayVariationsChanged);
	Q_PROPERTY(bool			planner_doo2_breaks
				READ		doo2Breaks
				WRITE		setDoo2Breaks
				NOTIFY      doo2BreaksChanged);
	Q_PROPERTY(bool			drop_stone_mode
				READ		dropStoneMode
				WRITE		setDropStoneMode
				NOTIFY      dropStoneModeChanged);
	Q_PROPERTY(bool			last_stop
				READ		lastStop
				WRITE		setLastStop
				NOTIFY      lastStopChanged);
	Q_PROPERTY(int			min_switch_duration
				READ		minSwitchDuration
				WRITE		setMinSwitchDuration
				NOTIFY      minSwitchDurationChanged);
	Q_PROPERTY(int			problem_solving_time
				READ		problemSolvingTime
				WRITE		setProblemSolvingTime
				NOTIFY      problemSolvingTimeChanged);
	Q_PROPERTY(int			reserve_gas
				READ		reserveGas
				WRITE		setReserveGas
				NOTIFY      reserveGasChanged);
	Q_PROPERTY(int			sac_factor
				READ		sacFactor
				WRITE		setSacFactor
				NOTIFY      sacFactorChanged);
	Q_PROPERTY(bool			safety_stop
				READ		safetyStop
				WRITE		setSafetyStop
				NOTIFY      safetyStopChanged);
	Q_PROPERTY(bool			switch_at_req_stop
				READ		switchAtRequiredStop
				WRITE		setSwitchAtRequiredStop
				NOTIFY      switchAtRequiredStopChanged);
	Q_PROPERTY(bool			verbatim_plan
				READ		verbatimPlan
				WRITE		setVerbatimPlan
				NOTIFY      verbatimPlanChanged);

public:
	qPrefPlanner(QObject *parent = NULL) : QObject(parent) {};
	~qPrefPlanner() {};
	static qPrefPlanner *instance();

// Load/Sync local settings (disk) and struct preference
void loadSync(bool doSync);

public:
	int ascRateLast6m() const;
	int ascRateStops() const;
	int ascRate50() const;
	int ascRate75() const;
	int bestmixEnd() const;
	int bottomPo2() const;
	int bottomSac() const;
	deco_mode decoMode() const;
	int decoPo2() const;
	int decoSac() const;
	int descRate() const;
	bool displayDuration() const;
	bool displayRuntime() const;
	bool displayTransitions() const;
	bool displayVariations() const;
	bool doo2Breaks() const;
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
	void setAscRateLast6m(int value);
	void setAscRateStops(int value);
	void setAscRate50(int value);
	void setAscRate75(int value);
	void setBestmixEnd(int value);
	void setBottomPo2(int value);
	void setBottomSac(int value);
	void setDecoMode(deco_mode value);
	void setDecoPo2(int value);
	void setDecoSac(int value);
	void setDescRate(int value);
	void setDisplayDuration(bool value);
	void setDisplayRuntime(bool value);
	void setDisplayTransitions(bool value);
	void setDisplayVariations(bool value);
	void setDoo2Breaks(bool value);
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
	void ascRateLast6mChanged(int value);
	void ascRateStopsChanged(int value);
	void ascRate50Changed(int value);
	void ascRate75Changed(int value);
	void bestmixEndChanged(int value);
	void bottomPo2Changed(int value);
	void bottomSacChanged(int value);
	void decoModeChanged(deco_mode value);
	void decoPo2Changed(int value);
	void decoSacChanged(int value);
	void descRateChanged(int value);
	void displayDurationChanged(bool value);
	void displayRuntimeChanged(bool value);
	void displayTransitionsChanged(bool value);
	void displayVariationsChanged(bool value);
	void doo2BreaksChanged(bool value);
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
	static qPrefPlanner *m_instance;

	void diskAscRateLast6m(bool doSync);
	void diskAscRateStops(bool doSync);
	void diskAscRate50(bool doSync);
	void diskAscRate75(bool doSync);
	void diskBestmixEnd(bool doSync);
	void diskBottomPo2(bool doSync);
	void diskBottomSac(bool doSync);
	void diskDecoMode(bool doSync);
	void diskDecoPo2(bool doSync);
	void diskDecoSac(bool doSync);
	void diskDescRate(bool doSync);
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
