// SPDX-License-Identifier: GPL-2.0
#include "profilescene.h"
#include "diveeventitem.h"
#include "divecartesianaxis.h"
#include "divepercentageitem.h"
#include "divepixmapcache.h"
#include "diveprofileitem.h"
#include "divetextitem.h"
#include "tankitem.h"
#include "core/dive.h"
#include "core/device.h"
#include "core/divecomputer.h"
#include "core/event.h"
#include "core/pref.h"
#include "core/profile.h"
#include "core/qthelper.h"	// for decoMode()
#include "core/range.h"
#include "core/subsurface-float.h"
#include "core/subsurface-string.h"
#include "core/settings/qPrefDisplay.h"
#include "qt-models/diveplannermodel.h"
#include <QAbstractAnimation>

static const double diveComputerTextBorder = 1.0;

// Class for animations (if any). Might want to do our own.
class ProfileAnimation : public QAbstractAnimation {
	ProfileScene &scene;
	// For historical reasons, speed is actually the duration
	// (i.e. the reciprocal of speed). Ouch, that hurts.
	int speed;

	int duration() const override
	{
		return speed;
	}
	void updateCurrentTime(int time) override
	{
		// Note: we explicitly pass 1.0 at the end, so that
		// the callee can do a simple float comparison for "end".
		scene.anim(time == speed ? 1.0
					 : static_cast<double>(time) / speed);
	}
public:
	ProfileAnimation(ProfileScene &scene, int animSpeed) :
		scene(scene),
		speed(animSpeed)
	{
		start();
	}
};

template<typename T, class... Args>
T *ProfileScene::createItem(const DiveCartesianAxis &vAxis, DataAccessor accessor, int z, Args&&... args)
{
	T *res = new T(plotInfo, *timeAxis, vAxis, accessor, std::forward<Args>(args)...);
	res->setZValue(static_cast<double>(z));
	profileItems.push_back(res);
	return res;
}

PartialPressureGasItem *ProfileScene::createPPGas(DataAccessor accessor, color_index_t color, color_index_t colorAlert,
						  const double *thresholdSettingsMin, const double *thresholdSettingsMax)
{
	PartialPressureGasItem *item = createItem<PartialPressureGasItem>(*gasYAxis, accessor, 99, dpr);
	item->setThresholdSettingsKey(thresholdSettingsMin, thresholdSettingsMax);
	item->setColors(getColor(color, isGrayscale), getColor(colorAlert, isGrayscale));
	return item;
}

template <int IDX>
double accessTissue(const plot_data &item)
{
	return static_cast<double>(item.ceilings[IDX].mm);
}

// For now, the accessor functions for the profile data do not possess a payload.
// To generate the 16 tissue (ceiling) accessor functions, use iterative templates.
// Thanks to C++17's constexpr if, this is actually easy to read and follow.
template <int ACT, int MAX>
void ProfileScene::addTissueItems(double dpr)
{
	if constexpr (ACT < MAX) {
		DiveCalculatedTissue *tissueItem = createItem<DiveCalculatedTissue>(*profileYAxis, &accessTissue<ACT>, ACT + 1, dpr);
		allTissues.push_back(tissueItem);
		addTissueItems<ACT + 1, MAX>(dpr);
	}
}

ProfileScene::ProfileScene(double dpr, bool printMode, bool isGrayscale) :
	d(nullptr),
	dc(-1),
	dpr(dpr),
	printMode(printMode),
	isGrayscale(isGrayscale),
	empty(true),
	maxtime(-1),
	maxdepth(-1),
	profileYAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Left, true, 3, 0, TIME_GRID, Qt::red, true, true,
				   dpr, 1.0, printMode, isGrayscale, *this)),
	gasYAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Right, false, 1, 2, TIME_GRID, Qt::black, true, true,
				       dpr, 0.7, printMode, isGrayscale, *this)),
	temperatureAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Right, false, 3, 0, TIME_GRID, Qt::black, false, false,
					    dpr, 1.0, printMode, isGrayscale, *this)),
	timeAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Bottom, false, 2, 2, TIME_GRID, Qt::blue, true, true,
			      dpr, 1.0, printMode, isGrayscale, *this)),
	cylinderPressureAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Right, false, 4, 0, TIME_GRID, Qt::black, false, false,
						   dpr, 1.0, printMode, isGrayscale, *this)),
	heartBeatAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Left, false, 3, 0, HR_AXIS, Qt::black, true, true,
					    dpr, 0.7, printMode, isGrayscale, *this)),
	percentageAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Right, false, 2, 0, TIME_GRID, Qt::black, false, false,
					     dpr, 0.7, printMode, isGrayscale, *this)),
	diveProfileItem(createItem<DiveProfileItem>(*profileYAxis,
						    [](const plot_data &item) { return (double)item.depth.mm; },
						    0, dpr)),
	temperatureItem(createItem<DiveTemperatureItem>(*temperatureAxis,
							[](const plot_data &item) { return (double)item.temperature; },
							1, dpr)),
	meanDepthItem(createItem<DiveMeanDepthItem>(*profileYAxis,
						    [](const plot_data &item) { return (double)item.running_sum.mm; },
						    1, dpr)),
	gasPressureItem(createItem<DiveGasPressureItem>(*cylinderPressureAxis,
							[](const plot_data &item) { return 0.0; }, // unused
							1, dpr)),
	diveComputerText(new DiveTextItem(dpr, 1.0, Qt::AlignRight | Qt::AlignTop, nullptr)),
	reportedCeiling(createItem<DiveReportedCeiling>(*profileYAxis,
							[](const plot_data &item) { return (double)item.ceiling.mm; },
							1, dpr)),
	pn2GasItem(createPPGas([](const plot_data &item) { return (double)item.pressures.n2; },
			       PN2, PN2_ALERT, NULL, &prefs.pp_graphs.pn2_threshold)),
	pheGasItem(createPPGas([](const plot_data &item) { return (double)item.pressures.he; },
			       PHE, PHE_ALERT, NULL, &prefs.pp_graphs.phe_threshold)),
	po2GasItem(createPPGas([](const plot_data &item) { return (double)item.pressures.o2; },
			       PO2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	o2SetpointGasItem(createPPGas([](const plot_data &item) { return item.o2setpoint.mbar / 1000.0; },
				      O2SETPOINT, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor1GasItem(createPPGas([](const plot_data &item) { return item.o2sensor[0].mbar / 1000.0; },
				      CCRSENSOR1, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor2GasItem(createPPGas([](const plot_data &item) { return item.o2sensor[1].mbar / 1000.0; },
				      CCRSENSOR2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor3GasItem(createPPGas([](const plot_data &item) { return item.o2sensor[2].mbar / 1000.0; },
				      CCRSENSOR3, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ocpo2GasItem(createPPGas([](const plot_data &item) { return item.scr_OC_pO2.mbar / 1000.0; },
				 SCR_OCPO2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	diveCeiling(createItem<DiveCalculatedCeiling>(*profileYAxis,
						      [](const plot_data &item) { return (double)item.ceiling.mm; },
						      1, dpr)),
	decoModelParameters(new DiveTextItem(dpr, 1.0, Qt::AlignHCenter | Qt::AlignTop, nullptr)),
	heartBeatItem(createItem<DiveHeartrateItem>(*heartBeatAxis,
						    [](const plot_data &item) { return (double)item.heartbeat; },
						    1, dpr)),
	percentageItem(new DivePercentageItem(*timeAxis, *percentageAxis)),
	tankItem(new TankItem(*timeAxis, dpr)),
	pixmaps(getDivePixmaps(dpr))
{
	setSceneRect(0, 0, 100, 100);
	setItemIndexMethod(QGraphicsScene::NoIndex);

	gasYAxis->setZValue(timeAxis->zValue() + 1);
	tankItem->setZValue(100);

	// These axes are not locale-dependent. Set their scale factor once here.
	timeAxis->setTransform(1.0/60.0);
	heartBeatAxis->setTransform(1.0);
	gasYAxis->setTransform(1.0); // Non-metric countries likewise use bar (disguised as "percentage") for partial pressure.

	addTissueItems<0,16>(dpr);

	percentageItem->setZValue(1.0);
	profileYAxis->setGridIsMultipleOfThree( qPrefDisplay::three_m_based_grid() );

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
	addItem(percentageItem);

	for (AbstractProfilePolygonItem *item: profileItems)
		addItem(item);
}

ProfileScene::~ProfileScene()
{
}

void ProfileScene::clear()
{
	for (AbstractProfilePolygonItem *item: profileItems)
		item->clear();

	// the events will have connected slots which can fire after
	// the dive and its data have been deleted - so explictly delete
	// the DiveEventItems
	qDeleteAll(eventItems);
	eventItems.clear();
	plotInfo = plot_info();
	empty = true;
}

static bool ppGraphsEnabled(const struct divecomputer *dc, bool simplified)
{
	return simplified ? (dc->divemode == CCR && prefs.pp_graphs.po2)
			  : (prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe);
}

// Update visibility of non-interactive chart features according to preferences
void ProfileScene::updateVisibility(bool diveHasHeartBeat, bool simplified)
{
	if (!d)
		return;
	const struct divecomputer *currentdc = d->get_dc(dc);
	bool ppGraphs = ppGraphsEnabled(currentdc, simplified);

	diveCeiling->setVisible(prefs.calcceiling);
	for (DiveCalculatedTissue *tissue: allTissues)
		tissue->setVisible(prefs.calcalltissues && prefs.calcceiling);
	reportedCeiling->setVisible(prefs.dcceiling);

	if (simplified) {
		pn2GasItem->setVisible(false);
		po2GasItem->setVisible(ppGraphs);
		pheGasItem->setVisible(false);

		temperatureItem->setVisible(!ppGraphs);
		tankItem->setVisible(!ppGraphs && prefs.tankbar);

		o2SetpointGasItem->setVisible(ppGraphs && prefs.show_ccr_setpoint);
		ccrsensor1GasItem->setVisible(ppGraphs && prefs.show_ccr_sensors);
		ccrsensor2GasItem->setVisible(ppGraphs && prefs.show_ccr_sensors && (currentdc->no_o2sensors > 1));
		ccrsensor3GasItem->setVisible(ppGraphs && prefs.show_ccr_sensors && (currentdc->no_o2sensors > 2));
		ocpo2GasItem->setVisible((currentdc->divemode == PSCR) && prefs.show_scr_ocpo2);
		// No point to show the gradient factor if we're not showing the calculated ceiling that is derived from it
		decoModelParameters->setVisible(prefs.calcceiling);
	} else {
		pn2GasItem->setVisible(prefs.pp_graphs.pn2);
		po2GasItem->setVisible(prefs.pp_graphs.po2);
		pheGasItem->setVisible(prefs.pp_graphs.phe);

		bool setpointflag = currentdc->divemode == CCR && prefs.pp_graphs.po2;
		bool sensorflag = setpointflag && prefs.show_ccr_sensors;
		o2SetpointGasItem->setVisible(setpointflag && prefs.show_ccr_setpoint);
		ccrsensor1GasItem->setVisible(sensorflag);
		ccrsensor2GasItem->setVisible(sensorflag && currentdc->no_o2sensors > 1);
		ccrsensor3GasItem->setVisible(sensorflag && currentdc->no_o2sensors > 2);
		ocpo2GasItem->setVisible(currentdc->divemode == PSCR && prefs.show_scr_ocpo2);

		heartBeatItem->setVisible(prefs.hrgraph && diveHasHeartBeat);

		decoModelParameters->setVisible(prefs.decoinfo);

		percentageItem->setVisible(prefs.percentagegraph);

		meanDepthItem->setVisible(prefs.show_average_depth);
		tankItem->setVisible(prefs.tankbar);
		temperatureItem->setVisible(true);
	}
}

void ProfileScene::resize(QSizeF size)
{
	setSceneRect(QRectF(QPointF(), size));
}

// Helper templates to determine slope and intersect of a linear function.
// The function arguments are supposed to be integral types.
template<typename Func>
static auto intercept(Func f)
{
	return f(0);
}
template<typename Func>
static auto slope(Func f)
{
	return f(1) - f(0);
}

// Helper structure for laying out secondary plots.
struct VerticalAxisLayout {
	DiveCartesianAxis *axis;
	double height;		// in pixels (times dpr) or a weight if not enough space
	double bottom_border;	// in pixels (times dpr)
	bool visible;
};

void ProfileScene::updateAxes(bool diveHasHeartBeat, bool simplified)
{
	if (!d)
		return;
	const struct divecomputer *currentdc = d->get_dc(dc);

	// Calculate left and right border needed for the axes and other chart items.
	double leftBorder = profileYAxis->width();
	if (prefs.hrgraph)
		leftBorder = std::max(leftBorder, heartBeatAxis->width());

	double rightWidth = timeAxis->horizontalOverhang();
	if (prefs.show_average_depth)
		rightWidth = std::max(rightWidth, meanDepthItem->labelWidth);
	if (ppGraphsEnabled(currentdc, simplified))
		rightWidth = std::max(rightWidth, gasYAxis->width());
	double rightBorder = sceneRect().width() - rightWidth;
	double width = rightBorder - leftBorder;

	if (width <= 10.0 * dpr)
		return clear();

	profileYAxis->setGridIsMultipleOfThree( qPrefDisplay::three_m_based_grid() );

	// Place the fixed dive computer text at the bottom
	double bottomBorder = sceneRect().height() - diveComputerText->height() - 2.0 * dpr * diveComputerTextBorder;
	diveComputerText->setPos(0.0, bottomBorder + dpr * diveComputerTextBorder);

	double topBorder = 0.0;

	// show the deco model parameters at the top in the center
	if (prefs.decoinfo) {
		decoModelParameters->setPos(leftBorder + width / 2.0, topBorder);
		topBorder += decoModelParameters->height();
	}

	bottomBorder -= timeAxis->height();
	profileRegion = QRectF(leftBorder, topBorder, width, bottomBorder - topBorder);
	timeAxis->setPosition(profileRegion);

	if (prefs.tankbar) {
		bottomBorder -= tankItem->height();
		// Note: we set x to 0.0, because the tank item uses the timeAxis to set the x-coordinate.
		tankItem->setPos(0.0, bottomBorder);
	}

	double height = bottomBorder - topBorder;
	if (height <= 50.0 * dpr)
		return clear();

	// The fraction of the gasYAxis can be customized through prefs.gasplot_frac
	// Fractions for other YAxes (percentagegraph, HartBeat, temperature) are fixed (for now)
	// The profile should have at least minProfileFraction
	const double minProfileFraction = 0.1;		// changed this to 0.1, to allow for much larger gasplot_frac!
	const double heartBeatFraction = 0.125;		// these fraction were determined by their earlier pixel-
	const double percentagegraphFraction = 0.083;	//  height dived by 600. looks much like before.
	double temperatureFraction = 0.14;	//  0.14 looks like before

	// first we sum up the fixed fractions of all displayed axes
	double _sumOfFractions = 0;
	if (!(ppGraphsEnabled(currentdc, simplified) && simplified) )  // dont add temperature in 'simplified' with ppGraphs
		_sumOfFractions += temperatureFraction;
	if (prefs.hrgraph && diveHasHeartBeat)
		_sumOfFractions += heartBeatFraction;
	if (prefs.percentagegraph)
		_sumOfFractions += percentagegraphFraction;

	// lower gasplot_frac until minProfileFraction is reached
	// the max value of 0.7 as set through tec-settings may be too much in some circumstances
	double _used_gasplot_frac = prefs.gasplot_frac;
	if (prefs.zoomed_plot) {
		while ( (1 - (_sumOfFractions + _used_gasplot_frac)) < minProfileFraction )
			_used_gasplot_frac -= 0.05;

		if (ppGraphsEnabled(currentdc, simplified))
			temperatureFraction = 0.083;  // with gasplot-zoom slightly reduce temp-plot
	} else {
		_used_gasplot_frac = 0.1;
	}

	if (simplified)  // simplified-mobile (make them a bit larger)
		_used_gasplot_frac = 0.2;		// until configurable we'll use a larger fixed value

	VerticalAxisLayout secondaryAxes[] = {
		// Note: axes are listed from bottom to top, since they are added that way.
		// using absolute numbers here causes troubles with small windows! all changed to fractions of height
		{ heartBeatAxis, heartBeatFraction * height, 0.0, prefs.hrgraph && diveHasHeartBeat },
		{ percentageAxis, percentagegraphFraction * height, 0.0, prefs.percentagegraph },
		{ gasYAxis, _used_gasplot_frac * height, 0.0, ppGraphsEnabled(currentdc, simplified) },
		{ temperatureAxis, temperatureFraction * height, 2.0, !(ppGraphsEnabled(currentdc, simplified) && simplified) }, //set temp invisible for simplified with ppGraphs
	};

	for (const VerticalAxisLayout &l: secondaryAxes) {
		l.axis->setVisible(l.visible);
		if (!l.visible)
			continue;
		bottomBorder -= l.height * dpr;
		l.axis->setPosition(QRectF(leftBorder, bottomBorder, width, (l.height - l.bottom_border) * dpr));
	}

	height = bottomBorder - topBorder;
	profileYAxis->setPosition(QRectF(leftBorder, topBorder, width, height));

	// The cylinders are displayed in the 24-80% region of the profile
	cylinderPressureAxis->setPosition(QRectF(leftBorder, topBorder + 0.24 * height, width, 0.56 * height));

	// Set scale factors depending on locale.
	// The conversion calls, such as mm_to_feet(), will be optimized away.
	profileYAxis->setTransform(prefs.units.length == units::METERS ? 0.001 : slope(mm_to_feet));
	cylinderPressureAxis->setTransform(prefs.units.pressure == units::BAR ? 0.001 : slope(mbar_to_PSI));
	// Temperature is special: this is not a linear transformation, but requires a shift of origin.
	if (prefs.units.temperature == units::CELSIUS)
		temperatureAxis->setTransform(slope(mkelvin_to_C), intercept(mkelvin_to_C));
	else
		temperatureAxis->setTransform(slope(mkelvin_to_F), intercept(mkelvin_to_F));
}

bool ProfileScene::pointOnProfile(const QPointF &point) const
{
	return timeAxis->pointInRange(point.x()) && profileYAxis->pointInRange(point.y());
}

static double max_gas(const plot_info &pi, double gas_pressures::*gas)
{
	double ret = -1;
	for (int i = 0; i < pi.nr; ++i) {
		if (pi.entry[i].pressures.*gas > ret)
			ret = pi.entry[i].pressures.*gas;
	}
	return ret;
}

void ProfileScene::plotDive(const struct dive *dIn, int dcIn, DivePlannerPointsModel *plannerModel,
			   bool inPlanner, bool instant, bool keepPlotInfo, bool calcMax, double zoom, double zoomedPosition)
{
	d = dIn;
	dc = dcIn;
	animatedAxes.clear();
	if (!d) {
		clear();
		return;
	}

	if (!plannerModel) {
		if (decoMode(false) == VPMB)
			decoModelParameters->set(QString("Subsurface VPM-B +%1").arg(prefs.vpmb_conservatism), getColor(PRESSURE_TEXT));
		else
			decoModelParameters->set(QString("Subsurface GF %1/%2").arg(prefs.gflow).arg(prefs.gfhigh), getColor(PRESSURE_TEXT));
	} else {
		struct diveplan &diveplan = plannerModel->getDiveplan();
		if (decoMode(inPlanner) == VPMB)
			decoModelParameters->set(QString("VPM-B +%1").arg(diveplan.vpmb_conservatism), getColor(PRESSURE_TEXT));
		else
			decoModelParameters->set(QString("GF %1/%2").arg(diveplan.gflow).arg(diveplan.gfhigh), getColor(PRESSURE_TEXT));
	}

	const struct divecomputer *currentdc = d->get_dc(dc);
	if (!currentdc || currentdc->samples.empty()) {
		clear();
		return;
	}

	// If we come from the empty state, the plot info has to be recalculated.
	if (empty)
		keepPlotInfo = false;
	empty = false;

	int animSpeed = instant || printMode ? 0 : qPrefDisplay::animation_speed();

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
	if (!keepPlotInfo)
		plotInfo = create_plot_info_new(d, currentdc, planner_ds);

	bool hasHeartBeat = plotInfo.maxhr;
	// For mobile we might want to turn of some features that are normally shown.
#ifdef SUBSURFACE_MOBILE
	bool simplified = true;
#else
	bool simplified = false;
#endif
	updateVisibility(hasHeartBeat, simplified);
	updateAxes(hasHeartBeat, simplified);

	int newMaxtime = get_maxtime(plotInfo);
	if (calcMax || newMaxtime > maxtime)
		maxtime = newMaxtime;

	/* Only update the max. depth if it's bigger than the current ones
	 * when we are dragging the handler to plan / add dive.
	 * otherwhise, update normally.
	 */
	int newMaxDepth = get_maxdepth(plotInfo);
	if (!calcMax) {
		if (maxdepth < newMaxDepth)
			maxdepth = newMaxDepth;
	} else {
		maxdepth = newMaxDepth;
	}

	// It seems that I'll have a lot of boilerplate setting the model / axis for
	// each item, I'll mostly like to fix this in the future, but I'll keep at this for now.
	profileYAxis->setBounds(0.0, maxdepth);
	profileYAxis->updateTicks(animSpeed);
	animatedAxes.push_back(profileYAxis);

	temperatureAxis->setBounds(plotInfo.mintemp,
				   plotInfo.maxtemp - plotInfo.mintemp > 2000 ? plotInfo.maxtemp : plotInfo.mintemp + 2000);

	if (hasHeartBeat) {
		heartBeatAxis->setBounds(plotInfo.minhr, plotInfo.maxhr);
		heartBeatAxis->updateTicks(animSpeed);
		animatedAxes.push_back(heartBeatAxis);
	}

	percentageAxis->setBounds(0, 100);
	percentageAxis->setVisible(false);
	percentageAxis->updateTicks(animSpeed);
	animatedAxes.push_back(percentageAxis);

	double relStart = (1.0 - 1.0/zoom) * zoomedPosition;
	double relEnd = relStart + 1.0/zoom;
	timeAxis->setBounds(round(relStart * maxtime), round(relEnd * maxtime));

	// Find first and last plotInfo entry
	int firstSecond = lrint(timeAxis->minimum());
	int lastSecond = lrint(timeAxis->maximum());
	auto it1 = std::lower_bound(plotInfo.entry.begin(), plotInfo.entry.end(), firstSecond,
				   [](const plot_data &d, int s)
				   { return d.sec < s; });
	auto it2 = std::lower_bound(it1, plotInfo.entry.end(), lastSecond,
				    [](const plot_data &d, int s)
				    { return d.sec < s; });
	if (it1 > plotInfo.entry.begin() && it1->sec > firstSecond)
		--it1;
	if (it2 < plotInfo.entry.end())
		++it2;
	int from = it1 - plotInfo.entry.begin();
	int to = it2 - plotInfo.entry.begin();

	timeAxis->updateTicks(animSpeed);
	animatedAxes.push_back(timeAxis);
	cylinderPressureAxis->setBounds(plotInfo.minpressure, plotInfo.maxpressure);

	tankItem->setData(d, currentdc, firstSecond, lastSecond);

	if (ppGraphsEnabled(currentdc, simplified)) {
		double max = prefs.pp_graphs.phe ? max_gas(plotInfo, &gas_pressures::he) : -1;
		if (prefs.pp_graphs.pn2)
			max = std::max(max_gas(plotInfo, &gas_pressures::n2), max);
		if (prefs.pp_graphs.po2)
			max = std::max(max_gas(plotInfo, &gas_pressures::o2), max);

		gasYAxis->setBounds(0.0, max);
		gasYAxis->updateTicks(animSpeed);
		animatedAxes.push_back(gasYAxis);
	}

	// Replot dive items
	for (AbstractProfilePolygonItem *item: profileItems)
		item->replot(d, from, to, inPlanner);

	if (prefs.percentagegraph)
		percentageItem->replot(d, currentdc, plotInfo);

	// The event items are a bit special since we don't know how many events are going to
	// exist on a dive, so I cant create cache items for that. that's why they are here
	// while all other items are up there on the constructor.
	qDeleteAll(eventItems);
	eventItems.clear();
	const struct gasmix *lastgasmix;
	divemode_t lastdivemode;
	int cylinder_index = gasmix_loop(*d, *currentdc).next_cylinder_index().first;
	if (cylinder_index == -1) {
		lastgasmix = &gasmix_air;
		lastdivemode = OC;
	} else {
		const cylinder_t *cylinder = d->get_cylinder(cylinder_index);
		lastgasmix = &cylinder->gasmix;
		lastdivemode = get_effective_divemode(*currentdc, *cylinder);
	}
	for (auto [idx, event]: enumerated_range(currentdc->events)) {
		// if print mode is selected only draw headings, SP change, gas events or bookmark event
		if (printMode) {
			if (event.name.empty() ||
			    !(event.name == "heading" ||
			      (event.name == "SP change" && event.time.seconds == 0) ||
			      event.is_gaschange() ||
			      event.is_divemodechange() ||
			      event.type == SAMPLE_EVENT_BOOKMARK))
				continue;
		}
		if (DiveEventItem::isInteresting(d, currentdc, event, plotInfo, firstSecond, lastSecond)) {
			auto item = new DiveEventItem(d, currentdc, idx, event, *lastgasmix, lastdivemode, plotInfo,
						      timeAxis, profileYAxis, animSpeed, *pixmaps);
			item->setZValue(2);
			addItem(item);
			eventItems.push_back(item);
		}
		if (event.is_gaschange()) {
			auto [gasmix, divemode] = d->get_gasmix_from_event(event, *currentdc);
			lastgasmix = &gasmix;
			lastdivemode = divemode;
		} else if (event.is_divemodechange())
			lastdivemode = event.value ? CCR : OC;
	}

	QString dcText = QString::fromStdString(get_dc_nickname(currentdc));
	if (is_dc_planner(currentdc))
		dcText = tr("Planned dive");
	else if (is_dc_manually_added_dive(currentdc))
		dcText = tr("Manually added dive");
	else if (dcText.isEmpty())
		dcText = tr("Unknown dive computer");
	int nr = d->number_of_computers();
	if (nr > 1)
		dcText += tr(" (#%1 of %2)").arg(dc + 1).arg(nr);
	diveComputerText->set(dcText, getColor(TIME_TEXT, isGrayscale));

	// Reset animation.
	if (animSpeed <= 0)
		animation.reset();
	else
		animation = std::make_unique<ProfileAnimation>(*this, animSpeed);
}

void ProfileScene::anim(double fraction)
{
	for (DiveCartesianAxis *axis: animatedAxes)
		axis->anim(fraction);
}

void ProfileScene::draw(QPainter *painter, const QRect &pos,
			const struct dive *d, int dc,
			DivePlannerPointsModel *plannerModel, bool inPlanner)
{
	QSize size = pos.size();
	resize(QSizeF(size));
	plotDive(d, dc, plannerModel, inPlanner, true, false, true);

	QImage image(pos.size(), QImage::Format_ARGB32);
	image.fill(getColor(::BACKGROUND, isGrayscale));

	QPainter imgPainter(&image);
	imgPainter.setRenderHint(QPainter::Antialiasing);
	imgPainter.setRenderHint(QPainter::SmoothPixmapTransform);
	render(&imgPainter, QRect(QPoint(), size), sceneRect(), Qt::IgnoreAspectRatio);
	imgPainter.end();

	if (isGrayscale) {
		// convert QImage to grayscale before rendering
		for (int i = 0; i < image.height(); i++) {
			QRgb *pixel = reinterpret_cast<QRgb *>(image.scanLine(i));
			QRgb *end = pixel + image.width();
			for (; pixel != end; pixel++) {
				int gray_val = qGray(*pixel);
				*pixel = QColor(gray_val, gray_val, gray_val).rgb();
			}
		}
	}
	painter->drawImage(pos, image);
}

// Calculate the new zoom position when the mouse is dragged by delta.
// This is annoyingly complex, because the zoom position is given as
// a real between 0 and 1.
double ProfileScene::calcZoomPosition(double zoom, double originalPos, double delta)
{
	double factor = 1.0 - 1.0/zoom;
	if (nearly_0(factor))
		return 0.0;
	double relStart = factor * originalPos;
	double start = relStart * maxtime;
	double newStart = start + timeAxis->deltaToValue(delta);
	double newRelStart = newStart / maxtime;
	double newPos = newRelStart / factor;
	return std::clamp(newPos, 0.0, 1.0);
}
