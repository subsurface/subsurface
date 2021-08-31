// SPDX-License-Identifier: GPL-2.0
#include "profilescene.h"
#include "diveeventitem.h"
#include "divecartesianaxis.h"
#include "divepercentageitem.h"
#include "divepixmapcache.h"
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

static const double diveComputerTextBorder = 1.0;

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
	PartialPressureGasItem *item = createItem<PartialPressureGasItem>(*gasYAxis, column, 99, dpr);
	item->setThresholdSettingsKey(thresholdSettingsMin, thresholdSettingsMax);
	item->setColors(getColor(color, isGrayscale), getColor(colorAlert, isGrayscale));
	return item;
}

ProfileScene::ProfileScene(double dpr, bool printMode, bool isGrayscale) :
	d(nullptr),
	dc(-1),
	dpr(dpr),
	printMode(printMode),
	isGrayscale(isGrayscale),
	maxtime(-1),
	maxdepth(-1),
	dataModel(new DivePlotDataModel(this)),
	profileYAxis(new DepthAxis(DiveCartesianAxis::Position::Left, TIME_GRID, dpr, printMode, *this)),
	gasYAxis(new PartialGasPressureAxis(*dataModel, DiveCartesianAxis::Position::Right, TIME_GRID, dpr, printMode, *this)),
	temperatureAxis(new TemperatureAxis(DiveCartesianAxis::Position::Right, TIME_GRID, dpr, printMode, *this)),
	timeAxis(new TimeAxis(DiveCartesianAxis::Position::Bottom, TIME_GRID, dpr, printMode, *this)),
	cylinderPressureAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Right, TIME_GRID, dpr, printMode, *this)),
	heartBeatAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Left, HR_AXIS, dpr, printMode, *this)),
	percentageAxis(new DiveCartesianAxis(DiveCartesianAxis::Position::Right, TIME_GRID, dpr, printMode, *this)),
	diveProfileItem(createItem<DiveProfileItem>(*profileYAxis, DivePlotDataModel::DEPTH, 0, dpr)),
	temperatureItem(createItem<DiveTemperatureItem>(*temperatureAxis, DivePlotDataModel::TEMPERATURE, 1, dpr)),
	meanDepthItem(createItem<DiveMeanDepthItem>(*profileYAxis, DivePlotDataModel::INSTANT_MEANDEPTH, 1, dpr)),
	gasPressureItem(createItem<DiveGasPressureItem>(*cylinderPressureAxis, DivePlotDataModel::TEMPERATURE, 1, dpr)),
	diveComputerText(new DiveTextItem(dpr, 1.0, Qt::AlignRight | Qt::AlignTop, nullptr)),
	reportedCeiling(createItem<DiveReportedCeiling>(*profileYAxis, DivePlotDataModel::CEILING, 1, dpr)),
	pn2GasItem(createPPGas(DivePlotDataModel::PN2, PN2, PN2_ALERT, NULL, &prefs.pp_graphs.pn2_threshold)),
	pheGasItem(createPPGas(DivePlotDataModel::PHE, PHE, PHE_ALERT, NULL, &prefs.pp_graphs.phe_threshold)),
	po2GasItem(createPPGas(DivePlotDataModel::PO2, PO2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	o2SetpointGasItem(createPPGas(DivePlotDataModel::O2SETPOINT, O2SETPOINT, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor1GasItem(createPPGas(DivePlotDataModel::CCRSENSOR1, CCRSENSOR1, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor2GasItem(createPPGas(DivePlotDataModel::CCRSENSOR2, CCRSENSOR2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ccrsensor3GasItem(createPPGas(DivePlotDataModel::CCRSENSOR3, CCRSENSOR3, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	ocpo2GasItem(createPPGas(DivePlotDataModel::SCR_OC_PO2, SCR_OCPO2, PO2_ALERT, &prefs.pp_graphs.po2_threshold_min, &prefs.pp_graphs.po2_threshold_max)),
	diveCeiling(createItem<DiveCalculatedCeiling>(*profileYAxis, DivePlotDataModel::CEILING, 1, dpr)),
	decoModelParameters(new DiveTextItem(dpr, 1.0, Qt::AlignHCenter | Qt::AlignTop, nullptr)),
	heartBeatItem(createItem<DiveHeartrateItem>(*heartBeatAxis, DivePlotDataModel::HEARTBEAT, 1, dpr)),
	percentageItem(new DivePercentageItem(*timeAxis, *percentageAxis, dpr)),
	tankItem(new TankItem(*timeAxis, dpr)),
	pixmaps(getDivePixmaps(dpr))
{
	init_plot_info(&plotInfo);

	setSceneRect(0, 0, 100, 100);
	setItemIndexMethod(QGraphicsScene::NoIndex);

	// Initialize axes. Perhaps part of this should be moved down to the axes code?
	profileYAxis->setOrientation(DiveCartesianAxis::TopToBottom);
	profileYAxis->setMinimum(0);
	profileYAxis->setTickInterval(M_OR_FT(10, 30));

	gasYAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	gasYAxis->setTickInterval(1);
	gasYAxis->setMinimum(0);
	gasYAxis->setFontLabelScale(0.7);

#ifndef SUBSURFACE_MOBILE
	heartBeatAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	heartBeatAxis->setTickInterval(10);
	heartBeatAxis->setFontLabelScale(0.7);

	percentageAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	percentageAxis->setTickInterval(10);
	percentageAxis->setFontLabelScale(0.7);
#endif

	temperatureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	temperatureAxis->setTickInterval(300);

	cylinderPressureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
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

	for (int i = 0; i < 16; i++) {
		DiveCalculatedTissue *tissueItem = createItem<DiveCalculatedTissue>(*profileYAxis, DivePlotDataModel::TISSUE_1 + i, i + 1, dpr);
		allTissues.append(tissueItem);
	}

	percentageItem->setZValue(1.0);

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

	updateAxes(true);
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
	decoModelParameters->setVisible(prefs.decoinfo);
#ifndef SUBSURFACE_MOBILE
	for (DiveCalculatedTissue *tissue: allTissues)
		tissue->setVisible(prefs.calcalltissues && prefs.calcceiling);
	percentageItem->setVisible(prefs.percentagegraph);
#endif
	meanDepthItem->setVisible(prefs.show_average_depth);
	reportedCeiling->setVisible(prefs.dcceiling);
	tankItem->setVisible(prefs.tankbar);
}

void ProfileScene::resize(QSizeF size)
{
	setSceneRect(QRectF(QPointF(), size));
	updateAxes(true); // disable animations when resizing
}

// Helper structure for laying out secondary plots.
struct VerticalAxisLayout {
	DiveCartesianAxis *axis;
	double height;
	bool visible;
};

void ProfileScene::updateAxes(bool instant)
{
	int animSpeed = instant || printMode ? 0 : qPrefDisplay::animation_speed();

	// Calculate left and right border needed for the axes.
	// viz. the depth axis to the left and the partial pressure axis to the right.
	// Thus, calculating the "border" of the graph is trivial.
	double leftBorder = profileYAxis->width();
	if (prefs.hrgraph)
		leftBorder = std::max(leftBorder, heartBeatAxis->width());
	double rightWidth = ppGraphsEnabled() ? gasYAxis->width() : 0.0;
	double rightBorder = sceneRect().width() - rightWidth;
	double width = rightBorder - leftBorder;

	if (width <= 10.0 * dpr)
		return clear();

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
	timeAxis->animateChangeLine(QRectF(leftBorder, topBorder, width, bottomBorder - topBorder), animSpeed);

	if (prefs.tankbar) {
		bottomBorder -= tankItem->height();
		// Note: we set x to 0.0, because the tank item uses the timeAxis to set the x-coordinate.
		tankItem->setPos(0.0, bottomBorder);
	}

	double height = bottomBorder - topBorder;
	if (height <= 50.0 * dpr)
		return clear();

	// The rest is laid out dynamically. Give at least 50% to the actual profile.
	// The max heights are given for DPR=1, i.e. a ca. 800x600 pixels profile.
	const double minProfileFraction = 0.5;
        VerticalAxisLayout secondaryAxes[] = {
		// Note: axes are listed from bottom to top, since they are added that way.
		{ heartBeatAxis, 75.0, prefs.hrgraph },
		{ percentageAxis, 50.0, prefs.percentagegraph },
		{ gasYAxis, 75.0, ppGraphsEnabled() },
		{ temperatureAxis, 50.0, true },
        };

	// A loop is probably easier to read than std::accumulate.
	double totalSecondaryHeight = 0.0;
	for (const VerticalAxisLayout &l: secondaryAxes) {
		if (l.visible)
			totalSecondaryHeight += l.height * dpr;
	}

	if (totalSecondaryHeight > height * minProfileFraction) {
		// Use 50% for the profile and the rest for the remaining graphs, scaled by their maximum height.
		double remainingSpace = height * minProfileFraction;
		for (VerticalAxisLayout &l: secondaryAxes)
			l.height *= remainingSpace / totalSecondaryHeight;
	}

	for (const VerticalAxisLayout &l: secondaryAxes) {
		l.axis->setVisible(l.visible);
		if (!l.visible)
			continue;
		bottomBorder -= l.height * dpr;
		l.axis->animateChangeLine(QRectF(leftBorder, bottomBorder, width, l.height * dpr), animSpeed);
	}

	height = bottomBorder - topBorder;
	profileYAxis->animateChangeLine(QRectF(leftBorder, topBorder, width, height), animSpeed);

	// The cylinders are displayed in the 24-80% region of the profile
	cylinderPressureAxis->animateChangeLine(QRectF(leftBorder, topBorder + 0.24 * height, width, 0.56 * height), animSpeed);
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
			decoModelParameters->set(QString("VPM-B +%1").arg(prefs.vpmb_conservatism), getColor(PRESSURE_TEXT));
		else
			decoModelParameters->set(QString("GF %1/%2").arg(prefs.gflow).arg(prefs.gfhigh), getColor(PRESSURE_TEXT));
	} else {
		struct diveplan &diveplan = plannerModel->getDiveplan();
		if (decoMode(inPlanner) == VPMB)
			decoModelParameters->set(QString("VPM-B +%1").arg(diveplan.vpmb_conservatism), getColor(PRESSURE_TEXT));
		else
			decoModelParameters->set(QString("GF %1/%2").arg(diveplan.gflow).arg(diveplan.gfhigh), getColor(PRESSURE_TEXT));
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
		heartBeatAxis->updateTicks(animSpeed); // this shows the ticks
	}
	heartBeatAxis->setVisible(prefs.hrgraph && plotInfo.maxhr);

	percentageAxis->setMinimum(0);
	percentageAxis->setMaximum(100);
	percentageAxis->setVisible(false);
	percentageAxis->updateTicks(animSpeed);

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

	if (prefs.percentagegraph)
		percentageItem->replot(d, currentdc, dataModel->data());

	// The event items are a bit special since we don't know how many events are going to
	// exist on a dive, so I cant create cache items for that. that's why they are here
	// while all other items are up there on the constructor.
	qDeleteAll(eventItems);
	eventItems.clear();
	struct event *event = currentdc->events;
	struct gasmix lastgasmix = get_gasmix_at_time(d, get_dive_dc_const(d, dc), duration_t{1});

	while (event) {
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
		if (DiveEventItem::isInteresting(d, currentdc, event, plotInfo)) {
			auto item = new DiveEventItem(d, event, lastgasmix, plotInfo,
						      timeAxis, profileYAxis, animSpeed, *pixmaps);
			item->setZValue(2);
			addItem(item);
			eventItems.push_back(item);
		}
		if (event_is_gaschange(event))
			lastgasmix = get_gasmix_from_event(d, event);
		event = event->next;
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
	diveComputerText->set(dcText, getColor(TIME_TEXT, isGrayscale));
}

void ProfileScene::draw(QPainter *painter, const QRect &pos,
			const struct dive *d, int dc,
			DivePlannerPointsModel *plannerModel, bool inPlanner)
{
	QSize size = pos.size();
	resize(QSizeF(size));
	plotDive(d, dc, plannerModel, inPlanner, true, true);

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
