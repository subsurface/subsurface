// SPDX-License-Identifier: GPL-2.0
#include "profilescene.h"
#include "diveeventitem.h"
#include "divecartesianaxis.h"
#include "diveprofileitem.h"
#include "divetextitem.h"
#include "tankitem.h"
#include "core/device.h"
#include "core/event.h"
#include "core/pref.h"
#include "core/profile.h"
#include "core/qthelper.h"	// for decoMode()
#include "core/subsurface-string.h"
#include "core/settings/qPrefDisplay.h"
#include "qt-models/diveplotdatamodel.h"
#include "qt-models/diveplannermodel.h"

const static struct ProfileItemPos {
	struct Pos {
		QPointF on;
		QPointF off;
	};
	struct Axis : public Pos {
		QLineF shrinked;
		QLineF expanded;
		QLineF intermediate;
	};
	Pos dcLabel;
	Pos tankBar;
	Axis depth;
	Axis partialPressure;
	Axis partialPressureTissue;
	Axis partialPressureWithTankBar;
	Axis percentage;
	Axis percentageWithTankBar;
	Axis time;
	Axis cylinder;
	Axis temperature;
	Axis temperatureAll;
	Axis heartBeat;
	Axis heartBeatWithTankBar;
	ProfileItemPos();
} itemPos;

template<typename T, class... Args>
T *ProfileScene::createItem(const DiveCartesianAxis &vAxis, int vColumn, int z, Args&&... args)
{
	T *res = new T(*dataModel, *timeAxis, DivePlotDataModel::TIME, vAxis, vColumn,
		       std::forward<Args>(args)...);
	res->setZValue(static_cast<double>(z));
	profileItems.push_back(res);
	return res;
}

PartialPressureGasItem *ProfileScene::createPPGas(int column, color_index_t color, color_index_t colorAlert,
						    const double *thresholdSettingsMin, const double *thresholdSettingsMax)
{
	PartialPressureGasItem *item = createItem<PartialPressureGasItem>(*gasYAxis, column, 99, fontPrintScale);
	item->setThresholdSettingsKey(thresholdSettingsMin, thresholdSettingsMax);
	item->setColors(getColor(color, isGrayscale), getColor(colorAlert, isGrayscale));
	return item;
}

ProfileScene::ProfileScene(double fontPrintScale, bool printMode, bool isGrayscale) :
	d(nullptr),
	dc(-1),
	fontPrintScale(fontPrintScale),
	printMode(printMode),
	isGrayscale(isGrayscale),
	maxtime(-1),
	maxdepth(-1),
	dataModel(new DivePlotDataModel(this)),
	profileYAxis(new DepthAxis(fontPrintScale, printMode, *this)),
	gasYAxis(new PartialGasPressureAxis(*dataModel, fontPrintScale, printMode, *this)),
	temperatureAxis(new TemperatureAxis(fontPrintScale, printMode, *this)),
	timeAxis(new TimeAxis(fontPrintScale, printMode, *this)),
	cylinderPressureAxis(new DiveCartesianAxis(fontPrintScale, printMode, *this)),
	heartBeatAxis(new DiveCartesianAxis(fontPrintScale, printMode, *this)),
	percentageAxis(new DiveCartesianAxis(fontPrintScale, printMode, *this)),
	diveProfileItem(createItem<DiveProfileItem>(*profileYAxis, DivePlotDataModel::DEPTH, 0, fontPrintScale)),
	temperatureItem(createItem<DiveTemperatureItem>(*temperatureAxis, DivePlotDataModel::TEMPERATURE, 1, fontPrintScale)),
	meanDepthItem(createItem<DiveMeanDepthItem>(*profileYAxis, DivePlotDataModel::INSTANT_MEANDEPTH, 1, fontPrintScale)),
	gasPressureItem(createItem<DiveGasPressureItem>(*cylinderPressureAxis, DivePlotDataModel::TEMPERATURE, 1, fontPrintScale)),
	diveComputerText(new DiveTextItem(fontPrintScale)),
	reportedCeiling(createItem<DiveReportedCeiling>(*profileYAxis, DivePlotDataModel::CEILING, 1, fontPrintScale)),
	pn2GasItem(createPPGas(DivePlotDataModel::PN2, PN2, PN2_ALERT, NULL, &prefs.pp_graphs.pn2_threshold)),
	pheGasItem(createPPGas(DivePlotDataModel::PHE, PHE, PHE_ALERT, NULL, &prefs.pp_graphs.phe_threshold)),
	po2GasItem(createPPGas(DivePlotDataModel::PO2, PO2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	o2SetpointGasItem(createPPGas(DivePlotDataModel::O2SETPOINT, O2SETPOINT, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor1GasItem(createPPGas(DivePlotDataModel::CCRSENSOR1, CCRSENSOR1, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor2GasItem(createPPGas(DivePlotDataModel::CCRSENSOR2, CCRSENSOR2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor3GasItem(createPPGas(DivePlotDataModel::CCRSENSOR3, CCRSENSOR3, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ocpo2GasItem(createPPGas(DivePlotDataModel::SCR_OC_PO2, SCR_OCPO2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	diveCeiling(createItem<DiveCalculatedCeiling>(*profileYAxis, DivePlotDataModel::CEILING, 1, fontPrintScale)),
	decoModelParameters(new DiveTextItem(fontPrintScale)),
	heartBeatItem(createItem<DiveHeartrateItem>(*heartBeatAxis, DivePlotDataModel::HEARTBEAT, 1, fontPrintScale)),
	tankItem(new TankItem(*timeAxis, fontPrintScale))
{
	init_plot_info(&plotInfo);

	setSceneRect(0, 0, 100, 100);
	setItemIndexMethod(QGraphicsScene::NoIndex);

	// Initialize axes. Perhaps part of this should be moved down to the axes code?
	profileYAxis->setOrientation(DiveCartesianAxis::TopToBottom);
	profileYAxis->setMinimum(0);
	profileYAxis->setTickInterval(M_OR_FT(10, 30));
	profileYAxis->setTickSize(0.5);
	profileYAxis->setLineSize(96);

	timeAxis->setLineSize(92);
	timeAxis->setTickSize(-0.5);

	gasYAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	gasYAxis->setTickInterval(1);
	gasYAxis->setTickSize(1);
	gasYAxis->setMinimum(0);
	gasYAxis->setFontLabelScale(0.7);
	gasYAxis->setLineSize(96);

#ifndef SUBSURFACE_MOBILE
	heartBeatAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	heartBeatAxis->setTickSize(0.2);
	heartBeatAxis->setTickInterval(10);
	heartBeatAxis->setFontLabelScale(0.7);
	heartBeatAxis->setLineSize(96);

	percentageAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	percentageAxis->setTickSize(0.2);
	percentageAxis->setTickInterval(10);
	percentageAxis->setFontLabelScale(0.7);
	percentageAxis->setLineSize(96);
#endif

	temperatureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	temperatureAxis->setTickSize(2);
	temperatureAxis->setTickInterval(300);

	cylinderPressureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	cylinderPressureAxis->setTickSize(2);
	cylinderPressureAxis->setTickInterval(30000);

	heartBeatAxis->setTextVisible(true);
	heartBeatAxis->setLinesVisible(true);
	percentageAxis->setTextVisible(true);
	percentageAxis->setLinesVisible(true);

	temperatureAxis->setTextVisible(false);
	temperatureAxis->setLinesVisible(false);
	cylinderPressureAxis->setTextVisible(false);
	cylinderPressureAxis->setLinesVisible(false);
	timeAxis->setLinesVisible(true);
	profileYAxis->setLinesVisible(true);
	gasYAxis->setZValue(timeAxis->zValue() + 1);
	tankItem->setZValue(100);

	// show the deco model parameters at the top in the center
	decoModelParameters->setY(0);
	decoModelParameters->setX(50);
	decoModelParameters->setBrush(getColor(PRESSURE_TEXT));
	decoModelParameters->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

	diveComputerText->setAlignment(Qt::AlignRight | Qt::AlignTop);
	diveComputerText->setBrush(getColor(TIME_TEXT, isGrayscale));
	diveComputerText->setPos(itemPos.dcLabel.on);

	tankItem->setPos(itemPos.tankBar.on);

	for (int i = 0; i < 16; i++) {
		DiveCalculatedTissue *tissueItem = createItem<DiveCalculatedTissue>(*profileYAxis, DivePlotDataModel::TISSUE_1 + i, i + 1, fontPrintScale);
		allTissues.append(tissueItem);
		DivePercentageItem *percentageItem = createItem<DivePercentageItem>(*percentageAxis, DivePlotDataModel::PERCENTAGE_1 + i, i + 1, i, fontPrintScale);
		allPercentages.append(percentageItem);
	}

	// Add items to scene
	addItem(diveComputerText);
	addItem(tankItem);
	addItem(decoModelParameters);
	addItem(profileYAxis);
	addItem(gasYAxis);
	addItem(temperatureAxis);
	addItem(timeAxis);
	addItem(cylinderPressureAxis);
	addItem(percentageAxis);
	addItem(heartBeatAxis);

	for (AbstractProfilePolygonItem *item: profileItems)
		addItem(item);
}

ProfileScene::~ProfileScene()
{
	free_plot_info_data(&plotInfo);
}

void ProfileScene::clear()
{
	dataModel->clear();

	for (AbstractProfilePolygonItem *item: profileItems)
		item->clear();

	// the events will have connected slots which can fire after
	// the dive and its data have been deleted - so explictly delete
	// the DiveEventItems
	qDeleteAll(eventItems);
	eventItems.clear();
}

static bool ppGraphsEnabled()
{
	return prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe;
}

// Update visibility of non-interactive chart features according to preferences
void ProfileScene::updateVisibility()
{
#ifndef SUBSURFACE_MOBILE
	pn2GasItem->setVisible(prefs.pp_graphs.pn2);
	po2GasItem->setVisible(prefs.pp_graphs.po2);
	pheGasItem->setVisible(prefs.pp_graphs.phe);

	const struct divecomputer *currentdc = d ? get_dive_dc_const(d, dc) : nullptr;
	bool setpointflag = currentdc && currentdc->divemode == CCR && prefs.pp_graphs.po2;
	bool sensorflag = setpointflag && prefs.show_ccr_sensors;
	o2SetpointGasItem->setVisible(setpointflag && prefs.show_ccr_setpoint);
	ccrsensor1GasItem->setVisible(sensorflag);
	ccrsensor2GasItem->setVisible(currentdc && sensorflag && currentdc->no_o2sensors > 1);
	ccrsensor3GasItem->setVisible(currentdc && sensorflag && currentdc->no_o2sensors > 2);
	ocpo2GasItem->setVisible(currentdc && currentdc->divemode == PSCR && prefs.show_scr_ocpo2);

	heartBeatItem->setVisible(prefs.hrgraph);
#endif
	diveCeiling->setVisible(prefs.calcceiling);
	decoModelParameters->setVisible(prefs.calcceiling);
#ifndef SUBSURFACE_MOBILE
	for (DiveCalculatedTissue *tissue: allTissues)
		tissue->setVisible(prefs.calcalltissues && prefs.calcceiling);
	for (DivePercentageItem *percentage: allPercentages)
		percentage->setVisible(prefs.percentagegraph);
#endif
	meanDepthItem->setVisible(prefs.show_average_depth);
	reportedCeiling->setVisible(prefs.dcceiling);
	tankItem->setVisible(prefs.tankbar);
}

void ProfileScene::updateAxes(bool instant)
{
	int animSpeed = instant || printMode ? 0 : qPrefDisplay::animation_speed();

	profileYAxis->setPos(itemPos.depth.on);

#ifndef SUBSURFACE_MOBILE
	gasYAxis->update(animSpeed);	// Initialize ticks of partial pressure graph
	if ((prefs.percentagegraph||prefs.hrgraph) && ppGraphsEnabled()) {
		profileYAxis->animateChangeLine(itemPos.depth.shrinked, animSpeed);
		temperatureAxis->setPos(itemPos.temperatureAll.on);
		temperatureAxis->animateChangeLine(itemPos.temperature.shrinked, animSpeed);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.shrinked, animSpeed);

		if (prefs.tankbar) {
			percentageAxis->setPos(itemPos.percentageWithTankBar.on);
			percentageAxis->animateChangeLine(itemPos.percentageWithTankBar.expanded, animSpeed);
			heartBeatAxis->setPos(itemPos.heartBeatWithTankBar.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeatWithTankBar.expanded, animSpeed);
		} else {
			percentageAxis->setPos(itemPos.percentage.on);
			percentageAxis->animateChangeLine(itemPos.percentage.expanded, animSpeed);
			heartBeatAxis->setPos(itemPos.heartBeat.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeat.expanded, animSpeed);
		}
		gasYAxis->setPos(itemPos.partialPressureTissue.on);
		gasYAxis->animateChangeLine(itemPos.partialPressureTissue.expanded, animSpeed);
	} else if (ppGraphsEnabled() || prefs.hrgraph || prefs.percentagegraph) {
		profileYAxis->animateChangeLine(itemPos.depth.intermediate, animSpeed);
		temperatureAxis->setPos(itemPos.temperature.on);
		temperatureAxis->animateChangeLine(itemPos.temperature.intermediate, animSpeed);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.intermediate, animSpeed);
		if (prefs.tankbar) {
			percentageAxis->setPos(itemPos.percentageWithTankBar.on);
			percentageAxis->animateChangeLine(itemPos.percentageWithTankBar.expanded, animSpeed);
			gasYAxis->setPos(itemPos.partialPressureWithTankBar.on);
			gasYAxis->animateChangeLine(itemPos.partialPressureWithTankBar.expanded, animSpeed);
			heartBeatAxis->setPos(itemPos.heartBeatWithTankBar.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeatWithTankBar.expanded, animSpeed);
		} else {
			gasYAxis->setPos(itemPos.partialPressure.on);
			gasYAxis->animateChangeLine(itemPos.partialPressure.expanded, animSpeed);
			percentageAxis->setPos(itemPos.percentage.on);
			percentageAxis->animateChangeLine(itemPos.percentage.expanded, animSpeed);
			heartBeatAxis->setPos(itemPos.heartBeat.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeat.expanded, animSpeed);
		}
	} else {
#else
	{
#endif
		profileYAxis->animateChangeLine(itemPos.depth.expanded, animSpeed);
		if (prefs.tankbar) {
			temperatureAxis->setPos(itemPos.temperatureAll.on);
		} else {
			temperatureAxis->setPos(itemPos.temperature.on);
		}
		temperatureAxis->animateChangeLine(itemPos.temperature.expanded, animSpeed);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.expanded, animSpeed);
	}

	timeAxis->setPos(itemPos.time.on);
	timeAxis->setLine(itemPos.time.expanded);

	cylinderPressureAxis->setPos(itemPos.cylinder.on);
}

bool ProfileScene::isPointOutOfBoundaries(const QPointF &point) const
{
	double xpos = timeAxis->valueAt(point);
	double ypos = profileYAxis->valueAt(point);
	return xpos > timeAxis->maximum() ||
	       xpos < timeAxis->minimum() ||
	       ypos > profileYAxis->maximum() ||
	       ypos < profileYAxis->minimum();
}

ProfileItemPos::ProfileItemPos()
{
	// Scene is *always* (double) 100 / 100.
	// TODO: The relative scaling doesn't work well with large or small
	// profiles and needs to be fixed.

	dcLabel.on.setX(3);
	dcLabel.on.setY(100);
	dcLabel.off.setX(-10);
	dcLabel.off.setY(100);

	tankBar.on.setX(0);
#ifndef SUBSURFACE_MOBILE
	tankBar.on.setY(91.95);
#else
	tankBar.on.setY(86.4);
#endif

	//Depth Axis Config
	depth.on.setX(3);
	depth.on.setY(3);
	depth.off.setX(-2);
	depth.off.setY(3);
	depth.expanded.setP1(QPointF(0, 0));
#ifndef SUBSURFACE_MOBILE
	depth.expanded.setP2(QPointF(0, 85));
#else
	depth.expanded.setP2(QPointF(0, 65));
#endif
	depth.shrinked.setP1(QPointF(0, 0));
	depth.shrinked.setP2(QPointF(0, 55));
	depth.intermediate.setP1(QPointF(0, 0));
	depth.intermediate.setP2(QPointF(0, 65));

	// Time Axis Config
	time.on.setX(3);
#ifndef SUBSURFACE_MOBILE
	time.on.setY(95);
#else
	time.on.setY(89.5);
#endif
	time.off.setX(3);
	time.off.setY(110);
	time.expanded.setP1(QPointF(0, 0));
	time.expanded.setP2(QPointF(94, 0));

	// Partial Gas Axis Config
	partialPressure.on.setX(97);
#ifndef SUBSURFACE_MOBILE
	partialPressure.on.setY(75);
#else
	partialPressure.on.setY(70);
#endif
	partialPressure.off.setX(110);
	partialPressure.off.setY(63);
	partialPressure.expanded.setP1(QPointF(0, 0));
#ifndef SUBSURFACE_MOBILE
	partialPressure.expanded.setP2(QPointF(0, 19));
#else
	partialPressure.expanded.setP2(QPointF(0, 20));
#endif
	partialPressureWithTankBar = partialPressure;
	partialPressureWithTankBar.expanded.setP2(QPointF(0, 17));
	partialPressureTissue = partialPressure;
	partialPressureTissue.on.setX(97);
	partialPressureTissue.on.setY(65);
	partialPressureTissue.expanded.setP2(QPointF(0, 16));

	// cylinder axis config
	cylinder.on.setX(3);
	cylinder.on.setY(20);
	cylinder.off.setX(-10);
	cylinder.off.setY(20);
	cylinder.expanded.setP1(QPointF(0, 15));
	cylinder.expanded.setP2(QPointF(0, 50));
	cylinder.shrinked.setP1(QPointF(0, 0));
	cylinder.shrinked.setP2(QPointF(0, 20));
	cylinder.intermediate.setP1(QPointF(0, 0));
	cylinder.intermediate.setP2(QPointF(0, 20));

	// Temperature axis config
	temperature.on.setX(3);
	temperature.off.setX(-10);
	temperature.off.setY(40);
	temperature.expanded.setP1(QPointF(0, 20));
	temperature.expanded.setP2(QPointF(0, 33));
	temperature.shrinked.setP1(QPointF(0, 2));
	temperature.shrinked.setP2(QPointF(0, 12));
#ifndef SUBSURFACE_MOBILE
	temperature.on.setY(60);
	temperatureAll.on.setY(51);
	temperature.intermediate.setP1(QPointF(0, 2));
	temperature.intermediate.setP2(QPointF(0, 12));
#else
	temperature.on.setY(51);
	temperatureAll.on.setY(47);
	temperature.intermediate.setP1(QPointF(0, 2));
	temperature.intermediate.setP2(QPointF(0, 12));
#endif

	// Heart rate axis config
	heartBeat.on.setX(3);
	heartBeat.on.setY(82);
	heartBeat.expanded.setP1(QPointF(0, 0));
	heartBeat.expanded.setP2(QPointF(0, 10));
	heartBeatWithTankBar = heartBeat;
	heartBeatWithTankBar.expanded.setP2(QPointF(0, 7));

	// Percentage axis config
	percentage.on.setX(3);
	percentage.on.setY(80);
	percentage.expanded.setP1(QPointF(0, 0));
	percentage.expanded.setP2(QPointF(0, 15));
	percentageWithTankBar = percentage;
	percentageWithTankBar.expanded.setP2(QPointF(0, 11.9));
}

void ProfileScene::plotDive(const struct dive *dIn, int dcIn, DivePlannerPointsModel *plannerModel, bool inPlanner, bool instant, bool calcMax)
{
	d = dIn;
	dc = dcIn;
	if (!d) {
		clear();
		return;
	}

	if (!plannerModel) {
		if (decoMode(false) == VPMB)
			decoModelParameters->setText(QString("VPM-B +%1").arg(prefs.vpmb_conservatism));
		else
			decoModelParameters->setText(QString("GF %1/%2").arg(prefs.gflow).arg(prefs.gfhigh));
	} else {
		struct diveplan &diveplan = plannerModel->getDiveplan();
		if (decoMode(inPlanner) == VPMB)
			decoModelParameters->setText(QString("VPM-B +%1").arg(diveplan.vpmb_conservatism));
		else
			decoModelParameters->setText(QString("GF %1/%2").arg(diveplan.gflow).arg(diveplan.gfhigh));
	}

	const struct divecomputer *currentdc = get_dive_dc_const(d, dc);
	if (!currentdc || !currentdc->samples)
		return;

	int animSpeed = instant || printMode ? 0 : qPrefDisplay::animation_speed();

	bool setpointflag = (currentdc->divemode == CCR) && prefs.pp_graphs.po2;
	bool sensorflag = setpointflag && prefs.show_ccr_sensors;
	o2SetpointGasItem->setVisible(setpointflag && prefs.show_ccr_setpoint);
	ccrsensor1GasItem->setVisible(sensorflag);
	ccrsensor2GasItem->setVisible(sensorflag && (currentdc->no_o2sensors > 1));
	ccrsensor3GasItem->setVisible(sensorflag && (currentdc->no_o2sensors > 2));
	ocpo2GasItem->setVisible((currentdc->divemode == PSCR) && prefs.show_scr_ocpo2);

	updateVisibility();

	// A non-null planner_ds signals to create_plot_info_new that the dive is currently planned.
	struct deco_state *planner_ds = inPlanner && plannerModel ? &plannerModel->final_deco_state : nullptr;

	/* This struct holds all the data that's about to be plotted.
	 * I'm not sure this is the best approach ( but since we are
	 * interpolating some points of the Dive, maybe it is... )
	 * The  Calculation of the points should be done per graph,
	 * so I'll *not* calculate everything if something is not being
	 * shown.
	 * create_plot_info_new() automatically frees old plot data.
	 */
	create_plot_info_new(d, get_dive_dc_const(d, dc), &plotInfo, !calcMax, planner_ds);

	int newMaxtime = get_maxtime(&plotInfo);
	if (calcMax || newMaxtime > maxtime)
		maxtime = newMaxtime;

	/* Only update the max. depth if it's bigger than the current ones
	 * when we are dragging the handler to plan / add dive.
	 * otherwhise, update normally.
	 */
	int newMaxDepth = get_maxdepth(&plotInfo);
	if (!calcMax) {
		if (maxdepth < newMaxDepth) {
			maxdepth = newMaxDepth;
		}
	} else {
		maxdepth = newMaxDepth;
	}

	dataModel->setDive(plotInfo);

	// It seems that I'll have a lot of boilerplate setting the model / axis for
	// each item, I'll mostly like to fix this in the future, but I'll keep at this for now.
	profileYAxis->setMaximum(maxdepth);
	profileYAxis->updateTicks(animSpeed);

	temperatureAxis->setMinimum(plotInfo.mintemp);
	temperatureAxis->setMaximum(plotInfo.maxtemp - plotInfo.mintemp > 2000 ? plotInfo.maxtemp : plotInfo.mintemp + 2000);

	if (plotInfo.maxhr) {
		int heartBeatAxisMin = lrint(plotInfo.minhr / 5.0 - 0.5) * 5;
		int heartBeatAxisMax, heartBeatAxisTick;
		if (plotInfo.maxhr - plotInfo.minhr < 40)
			heartBeatAxisTick = 10;
		else if (plotInfo.maxhr - plotInfo.minhr < 80)
			heartBeatAxisTick = 20;
		else if (plotInfo.maxhr - plotInfo.minhr < 100)
			heartBeatAxisTick = 25;
		else
			heartBeatAxisTick = 50;
		for (heartBeatAxisMax = heartBeatAxisMin; heartBeatAxisMax < plotInfo.maxhr; heartBeatAxisMax += heartBeatAxisTick);
		heartBeatAxis->setMinimum(heartBeatAxisMin);
		heartBeatAxis->setMaximum(heartBeatAxisMax + 1);
		heartBeatAxis->setTickInterval(heartBeatAxisTick);
		heartBeatAxis->updateTicks(HR_AXIS); // this shows the ticks
	}
	heartBeatAxis->setVisible(prefs.hrgraph && plotInfo.maxhr);

	percentageAxis->setMinimum(0);
	percentageAxis->setMaximum(100);
	percentageAxis->setVisible(false);
	percentageAxis->updateTicks(HR_AXIS);

	if (calcMax)
		timeAxis->setMaximum(maxtime);

	int i, incr;
	static int increments[8] = { 10, 20, 30, 60, 5 * 60, 10 * 60, 15 * 60, 30 * 60 };
	/* Time markers: at most every 10 seconds, but no more than 12 markers.
	 * We start out with 10 seconds and increment up to 30 minutes,
	 * depending on the dive time.
	 * This allows for 6h dives - enough (I hope) for even the craziest
	 * divers - but just in case, for those 8h depth-record-breaking dives,
	 * we double the interval if this still doesn't get us to 12 or fewer
	 * time markers */
	i = 0;
	while (i < 7 && maxtime / increments[i] > 12)
		i++;
	incr = increments[i];
	while (maxtime / incr > 12)
		incr *= 2;
	timeAxis->setTickInterval(incr);
	timeAxis->updateTicks(animSpeed);
	cylinderPressureAxis->setMinimum(plotInfo.minpressure);
	cylinderPressureAxis->setMaximum(plotInfo.maxpressure);

#ifdef SUBSURFACE_MOBILE
	if (currentdc->divemode == CCR) {
		gasYAxis->setPos(itemPos.partialPressure.on);
		gasYAxis->setLine(itemPos.partialPressure.expanded);

		tankItem->setVisible(false);
		pn2GasItem->setVisible(false);
		po2GasItem->setVisible(prefs.pp_graphs.po2);
		pheGasItem->setVisible(false);
		o2SetpointGasItem->setVisible(prefs.show_ccr_setpoint);
		ccrsensor1GasItem->setVisible(prefs.show_ccr_sensors);
		ccrsensor2GasItem->setVisible(prefs.show_ccr_sensors && (currentdc->no_o2sensors > 1));
		ccrsensor3GasItem->setVisible(prefs.show_ccr_sensors && (currentdc->no_o2sensors > 1));
		ocpo2GasItem->setVisible((currentdc->divemode == PSCR) && prefs.show_scr_ocpo2);
		//when no gas graph, we can show temperature
		if (!po2GasItem->isVisible() &&
		    !o2SetpointGasItem->isVisible() &&
		    !ccrsensor1GasItem->isVisible() &&
		    !ccrsensor2GasItem->isVisible() &&
		    !ccrsensor3GasItem->isVisible() &&
		    !ocpo2GasItem->isVisible())
			temperatureItem->setVisible(true);
		else
			temperatureItem->setVisible(false);
	} else {
		tankItem->setVisible(prefs.tankbar);
		gasYAxis->setPos(itemPos.partialPressure.off);
		pn2GasItem->setVisible(false);
		po2GasItem->setVisible(false);
		pheGasItem->setVisible(false);
		o2SetpointGasItem->setVisible(false);
		ccrsensor1GasItem->setVisible(false);
		ccrsensor2GasItem->setVisible(false);
		ccrsensor3GasItem->setVisible(false);
		ocpo2GasItem->setVisible(false);
	}
#endif
	tankItem->setData(&plotInfo, d);

	gasYAxis->update(animSpeed);

	// Replot dive items
	for (AbstractProfilePolygonItem *item: profileItems)
		item->replot(d, inPlanner);

	// The event items are a bit special since we don't know how many events are going to
	// exist on a dive, so I cant create cache items for that. that's why they are here
	// while all other items are up there on the constructor.
	qDeleteAll(eventItems);
	eventItems.clear();
	struct event *event = currentdc->events;
	struct gasmix lastgasmix = get_gasmix_at_time(d, get_dive_dc_const(d, dc), duration_t{1});

	while (event) {
#ifndef SUBSURFACE_MOBILE
		// if print mode is selected only draw headings, SP change, gas events or bookmark event
		if (printMode) {
			if (empty_string(event->name) ||
			    !(strcmp(event->name, "heading") == 0 ||
			      (same_string(event->name, "SP change") && event->time.seconds == 0) ||
			      event_is_gaschange(event) ||
			      event->type == SAMPLE_EVENT_BOOKMARK)) {
				event = event->next;
				continue;
			}
		}
#else
		// printMode is always selected for SUBSURFACE_MOBILE due to font problems
		// BUT events are wanted.
#endif
		DiveEventItem *item = new DiveEventItem(d, event, lastgasmix, dataModel,
							timeAxis, profileYAxis, animSpeed,
							fontPrintScale);
		item->setZValue(2);
		addItem(item);
		eventItems.push_back(item);
		if (event_is_gaschange(event))
			lastgasmix = get_gasmix_from_event(d, event);
		event = event->next;
	}

	// Only set visible the events that should be visible
	Q_FOREACH (DiveEventItem *event, eventItems) {
		event->setVisible(!event->shouldBeHidden());
	}
	QString dcText = get_dc_nickname(currentdc);
	if (dcText == "planned dive")
		dcText = tr("Planned dive");
	else if (dcText == "manually added dive")
		dcText = tr("Manually added dive");
	else if (dcText.isEmpty())
		dcText = tr("Unknown dive computer");
#ifndef SUBSURFACE_MOBILE
	int nr;
	if ((nr = number_of_computers(d)) > 1)
		dcText += tr(" (#%1 of %2)").arg(dc + 1).arg(nr);
#endif
	diveComputerText->setText(dcText);
}
