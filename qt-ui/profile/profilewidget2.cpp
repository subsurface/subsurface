#include "profilewidget2.h"
#include "diveplotdatamodel.h"
#include "divepixmapitem.h"
#include "diverectitem.h"
#include "divecartesianaxis.h"
#include "diveprofileitem.h"
#include "helpers.h"
#include "profile.h"
#include "diveeventitem.h"
#include "divetextitem.h"
#include "divetooltipitem.h"
#include "animationfunctions.h"
#include "planner.h"
#include "device.h"
#include "ruleritem.h"
#include "tankitem.h"
#include "dive.h"
#include "pref.h"
#include <libdivecomputer/parser.h>
#include <QSignalTransition>
#include <QPropertyAnimation>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDebug>
#include <QScrollBar>
#include <QtCore/qmath.h>
#include <QMessageBox>
#include <QInputDialog>

#ifndef QT_NO_DEBUG
#include <QTableView>
#endif
#include "mainwindow.h"
#include <preferences.h>

/* This is the global 'Item position' variable.
 * it should tell you where to position things up
 * on the canvas.
 *
 * please, please, please, use this instead of
 * hard coding the item on the scene with a random
 * value.
 */
static struct _ItemPos {
	struct _Pos {
		QPointF on;
		QPointF off;
	};
	struct _Axis {
		_Pos pos;
		QLineF shrinked;
		QLineF expanded;
	};
	_Pos background;
	_Pos dcLabel;
	_Pos tankBar;
	_Axis depth;
	_Axis partialPressure;
	_Axis partialPressureWithTankBar;
	_Axis percentage;
	_Axis time;
	_Axis cylinder;
	_Axis temperature;
	_Axis heartBeat;
} itemPos;

ProfileWidget2::ProfileWidget2(QWidget *parent) : QGraphicsView(parent),
	currentState(INVALID),
	dataModel(new DivePlotDataModel(this)),
	zoomLevel(0),
	zoomFactor(1.15),
	background(new DivePixmapItem()),
	backgroundFile(":poster"),
	toolTipItem(new ToolTipItem()),
	isPlotZoomed(prefs.zoomed_plot),
	profileYAxis(new DepthAxis()),
	gasYAxis(new PartialGasPressureAxis()),
	temperatureAxis(new TemperatureAxis()),
	timeAxis(new TimeAxis()),
	diveProfileItem(new DiveProfileItem()),
	temperatureItem(new DiveTemperatureItem()),
	cylinderPressureAxis(new DiveCartesianAxis()),
	gasPressureItem(new DiveGasPressureItem()),
	meanDepth(new MeanDepthLine()),
	diveComputerText(new DiveTextItem()),
	diveCeiling(new DiveCalculatedCeiling()),
	reportedCeiling(new DiveReportedCeiling()),
	pn2GasItem(new PartialPressureGasItem()),
	pheGasItem(new PartialPressureGasItem()),
	po2GasItem(new PartialPressureGasItem()),
	heartBeatAxis(new DiveCartesianAxis()),
	heartBeatItem(new DiveHeartrateItem()),
	percentageAxis(new DiveCartesianAxis()),
	ambPressureItem(new DiveAmbPressureItem()),
	gflineItem(new DiveGFLineItem()),
	mouseFollowerVertical(new DiveLineItem()),
	mouseFollowerHorizontal(new DiveLineItem()),
	rulerItem(new RulerItem2()),
	tankItem(new TankItem()),
	isGrayscale(false),
	printMode(false),
	shouldCalculateMaxTime(true),
	shouldCalculateMaxDepth(true),
	fontPrintScale(1.0)
{
	memset(&plotInfo, 0, sizeof(plotInfo));

	setupSceneAndFlags();
	setupItemSizes();
	setupItemOnScene();
	addItemsToScene();
	scene()->installEventFilter(this);
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	QAction *action = NULL;
#define ADD_ACTION(SHORTCUT, Slot)                                  \
	action = new QAction(this);                                 \
	action->setShortcut(SHORTCUT);                              \
	action->setShortcutContext(Qt::WindowShortcut);             \
	addAction(action);                                          \
	connect(action, SIGNAL(triggered(bool)), this, SLOT(Slot)); \
	actionsForKeys[SHORTCUT] = action;

	ADD_ACTION(Qt::Key_Escape, keyEscAction());
	ADD_ACTION(Qt::Key_Delete, keyDeleteAction());
	ADD_ACTION(Qt::Key_Up, keyUpAction());
	ADD_ACTION(Qt::Key_Down, keyDownAction());
	ADD_ACTION(Qt::Key_Left, keyLeftAction());
	ADD_ACTION(Qt::Key_Right, keyRightAction());
#undef ADD_ACTION

#ifndef QT_NO_DEBUG
	QTableView *diveDepthTableView = new QTableView();
	diveDepthTableView->setModel(dataModel);
	diveDepthTableView->show();
#endif
}

#define SUBSURFACE_OBJ_DATA 1
#define SUBSURFACE_OBJ_DC_TEXT 0x42

void ProfileWidget2::addItemsToScene()
{
	scene()->addItem(background);
	scene()->addItem(toolTipItem);
	scene()->addItem(profileYAxis);
	scene()->addItem(gasYAxis);
	scene()->addItem(temperatureAxis);
	scene()->addItem(timeAxis);
	scene()->addItem(diveProfileItem);
	scene()->addItem(cylinderPressureAxis);
	scene()->addItem(temperatureItem);
	scene()->addItem(gasPressureItem);
	scene()->addItem(meanDepth);
	// I cannot seem to figure out if an object that I find with itemAt() on the scene
	// is the object I am looking for - my guess is there's a simple way in Qt to do that
	// but nothing I tried worked.
	// so instead this adds a special magic key/value pair to the object to mark it
	diveComputerText->setData(SUBSURFACE_OBJ_DATA, SUBSURFACE_OBJ_DC_TEXT);
	scene()->addItem(diveComputerText);
	scene()->addItem(diveCeiling);
	scene()->addItem(reportedCeiling);
	scene()->addItem(pn2GasItem);
	scene()->addItem(pheGasItem);
	scene()->addItem(po2GasItem);
	scene()->addItem(percentageAxis);
	scene()->addItem(heartBeatAxis);
	scene()->addItem(heartBeatItem);
	scene()->addItem(rulerItem);
	scene()->addItem(rulerItem->sourceNode());
	scene()->addItem(rulerItem->destNode());
	scene()->addItem(tankItem);
	scene()->addItem(mouseFollowerHorizontal);
	scene()->addItem(mouseFollowerVertical);
	QPen pen(QColor(Qt::red).lighter());
	pen.setWidth(0);
	mouseFollowerHorizontal->setPen(pen);
	mouseFollowerVertical->setPen(pen);
	Q_FOREACH (DiveCalculatedTissue *tissue, allTissues) {
		scene()->addItem(tissue);
	}
	Q_FOREACH (DivePercentageItem *percentage, allPercentages) {
		scene()->addItem(percentage);
	}
	scene()->addItem(ambPressureItem);
	scene()->addItem(gflineItem);
}

void ProfileWidget2::setupItemOnScene()
{
	background->setZValue(9999);
	toolTipItem->setZValue(9998);
	toolTipItem->setTimeAxis(timeAxis);
	rulerItem->setZValue(9997);
	tankItem->setZValue(100);

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
	gasYAxis->setModel(dataModel);
	gasYAxis->setFontLabelScale(0.7);
	gasYAxis->setLineSize(96);

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

	temperatureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	temperatureAxis->setTickSize(2);
	temperatureAxis->setTickInterval(300);

	cylinderPressureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	cylinderPressureAxis->setTickSize(2);
	cylinderPressureAxis->setTickInterval(30000);

	meanDepth->setLine(0, 0, 96, 0);
	meanDepth->setX(3);
	meanDepth->setPen(QPen(QBrush(Qt::red), 0, Qt::SolidLine));
	meanDepth->setZValue(1);
	meanDepth->setAxis(profileYAxis);

	diveComputerText->setAlignment(Qt::AlignRight | Qt::AlignTop);
	diveComputerText->setBrush(getColor(TIME_TEXT, isGrayscale));

	rulerItem->setAxis(timeAxis, profileYAxis);
	tankItem->setHorizontalAxis(timeAxis);

	setupItem(reportedCeiling, timeAxis, profileYAxis, dataModel, DivePlotDataModel::CEILING, DivePlotDataModel::TIME, 1);
	setupItem(diveCeiling, timeAxis, profileYAxis, dataModel, DivePlotDataModel::CEILING, DivePlotDataModel::TIME, 1);
	for (int i = 0; i < 16; i++) {
		DiveCalculatedTissue *tissueItem = new DiveCalculatedTissue();
		setupItem(tissueItem, timeAxis, profileYAxis, dataModel, DivePlotDataModel::TISSUE_1 + i, DivePlotDataModel::TIME, 1 + i);
		allTissues.append(tissueItem);
		DivePercentageItem *percentageItem = new DivePercentageItem(i);
		setupItem(percentageItem, timeAxis, percentageAxis, dataModel, DivePlotDataModel::PERCENTAGE_1 + i, DivePlotDataModel::TIME, 1 + i);
		allPercentages.append(percentageItem);
	}
	setupItem(gasPressureItem, timeAxis, cylinderPressureAxis, dataModel, DivePlotDataModel::TEMPERATURE, DivePlotDataModel::TIME, 1);
	setupItem(temperatureItem, timeAxis, temperatureAxis, dataModel, DivePlotDataModel::TEMPERATURE, DivePlotDataModel::TIME, 1);
	setupItem(heartBeatItem, timeAxis, heartBeatAxis, dataModel, DivePlotDataModel::HEARTBEAT, DivePlotDataModel::TIME, 1);
	setupItem(ambPressureItem, timeAxis, percentageAxis, dataModel, DivePlotDataModel::AMBPRESSURE, DivePlotDataModel::TIME, 1);
	setupItem(gflineItem, timeAxis, percentageAxis, dataModel, DivePlotDataModel::GFLINE, DivePlotDataModel::TIME, 1);
	setupItem(diveProfileItem, timeAxis, profileYAxis, dataModel, DivePlotDataModel::DEPTH, DivePlotDataModel::TIME, 0);

#define CREATE_PP_GAS(ITEM, VERTICAL_COLUMN, COLOR, COLOR_ALERT, THRESHOULD_SETTINGS, VISIBILITY_SETTINGS)              \
	setupItem(ITEM, timeAxis, gasYAxis, dataModel, DivePlotDataModel::VERTICAL_COLUMN, DivePlotDataModel::TIME, 0); \
	ITEM->setThreshouldSettingsKey(THRESHOULD_SETTINGS);                                                            \
	ITEM->setVisibilitySettingsKey(VISIBILITY_SETTINGS);                                                            \
	ITEM->setColors(getColor(COLOR, isGrayscale), getColor(COLOR_ALERT, isGrayscale));                              \
	ITEM->settingsChanged();                                                                                        \
	ITEM->setZValue(99);

	CREATE_PP_GAS(pn2GasItem, PN2, PN2, PN2_ALERT, "pn2threshold", "pn2graph");
	CREATE_PP_GAS(pheGasItem, PHE, PHE, PHE_ALERT, "phethreshold", "phegraph");
	CREATE_PP_GAS(po2GasItem, PO2, PO2, PO2_ALERT, "po2threshold", "po2graph");
#undef CREATE_PP_GAS

	temperatureAxis->setTextVisible(false);
	temperatureAxis->setLinesVisible(false);
	cylinderPressureAxis->setTextVisible(false);
	cylinderPressureAxis->setLinesVisible(false);
	timeAxis->setLinesVisible(true);
	profileYAxis->setLinesVisible(true);
	gasYAxis->setZValue(timeAxis->zValue() + 1);
	heartBeatAxis->setTextVisible(true);
	heartBeatAxis->setLinesVisible(true);
	percentageAxis->setTextVisible(true);
	percentageAxis->setLinesVisible(true);
}

void ProfileWidget2::replot()
{
	dataModel->clear();
	plotDive(0, true); // simply plot the displayed_dive again
}

void ProfileWidget2::setupItemSizes()
{
	// Scene is *always* (double) 100 / 100.
	// Background Config
	/* Much probably a better math is needed here.
	 * good thing is that we only need to change the
	 * Axis and everything else is auto-adjusted.*
	 */

	itemPos.background.on.setX(0);
	itemPos.background.on.setY(0);
	itemPos.background.off.setX(0);
	itemPos.background.off.setY(110);

	//Depth Axis Config
	itemPos.depth.pos.on.setX(3);
	itemPos.depth.pos.on.setY(3);
	itemPos.depth.pos.off.setX(-2);
	itemPos.depth.pos.off.setY(3);
	itemPos.depth.expanded.setP1(QPointF(0, 0));
	itemPos.depth.expanded.setP2(QPointF(0, 86));
	itemPos.depth.shrinked.setP1(QPointF(0, 0));
	itemPos.depth.shrinked.setP2(QPointF(0, 60));

	// Time Axis Config
	itemPos.time.pos.on.setX(3);
	itemPos.time.pos.on.setY(95);
	itemPos.time.pos.off.setX(3);
	itemPos.time.pos.off.setY(110);
	itemPos.time.expanded.setP1(QPointF(0, 0));
	itemPos.time.expanded.setP2(QPointF(94, 0));

	// Partial Gas Axis Config
	itemPos.partialPressure.pos.on.setX(97);
	itemPos.partialPressure.pos.on.setY(65);
	itemPos.partialPressure.pos.off.setX(110);
	itemPos.partialPressure.pos.off.setY(63);
	itemPos.partialPressure.expanded.setP1(QPointF(0, 0));
	itemPos.partialPressure.expanded.setP2(QPointF(0, 20));
	itemPos.partialPressureWithTankBar = itemPos.partialPressure;
	itemPos.partialPressureWithTankBar.expanded.setP2(QPointF(0, 27));

	// cylinder axis config
	itemPos.cylinder.pos.on.setX(3);
	itemPos.cylinder.pos.on.setY(20);
	itemPos.cylinder.pos.off.setX(-10);
	itemPos.cylinder.pos.off.setY(20);
	itemPos.cylinder.expanded.setP1(QPointF(0, 15));
	itemPos.cylinder.expanded.setP2(QPointF(0, 50));
	itemPos.cylinder.shrinked.setP1(QPointF(0, 0));
	itemPos.cylinder.shrinked.setP2(QPointF(0, 20));

	// Temperature axis config
	itemPos.temperature.pos.on.setX(3);
	itemPos.temperature.pos.on.setY(40);
	itemPos.temperature.pos.off.setX(-10);
	itemPos.temperature.pos.off.setY(40);
	itemPos.temperature.expanded.setP1(QPointF(0, 30));
	itemPos.temperature.expanded.setP2(QPointF(0, 50));
	itemPos.temperature.shrinked.setP1(QPointF(0, 5));
	itemPos.temperature.shrinked.setP2(QPointF(0, 15));

	itemPos.heartBeat.pos.on.setX(3);
	itemPos.heartBeat.pos.on.setY(60);
	itemPos.heartBeat.expanded.setP1(QPointF(0, 0));
	itemPos.heartBeat.expanded.setP2(QPointF(0, 15));

	itemPos.percentage.pos.on.setX(3);
	itemPos.percentage.pos.on.setY(85);
	itemPos.percentage.expanded.setP1(QPointF(0, 0));
	itemPos.percentage.expanded.setP2(QPointF(0, 15));

	itemPos.dcLabel.on.setX(3);
	itemPos.dcLabel.on.setY(100);
	itemPos.dcLabel.off.setX(-10);
	itemPos.dcLabel.off.setY(100);

	itemPos.tankBar.on.setX(0);
	itemPos.tankBar.on.setY(92);
}

void ProfileWidget2::setupItem(AbstractProfilePolygonItem *item, DiveCartesianAxis *hAxis,
	DiveCartesianAxis *vAxis, DivePlotDataModel *model,
	int vData, int hData, int zValue)
{
	item->setHorizontalAxis(hAxis);
	item->setVerticalAxis(vAxis);
	item->setModel(model);
	item->setVerticalDataColumn(vData);
	item->setHorizontalDataColumn(hData);
	item->setZValue(zValue);
}

void ProfileWidget2::setupSceneAndFlags()
{
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0, 0, 100, 100);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
	setOptimizationFlags(QGraphicsView::DontSavePainterState);
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
	setMouseTracking(true);
	background->setFlag(QGraphicsItem::ItemIgnoresTransformations);
}

// Currently just one dive, but the plan is to enable All of the selected dives.
void ProfileWidget2::plotDive(struct dive *d, bool force)
{
	static bool firstCall = true;
	QTime measureDuration; // let's measure how long this takes us (maybe we'll turn of TTL calculation later
	measureDuration.start();

	if (currentState != ADD && currentState != PLAN) {
		if (!d) {
			if (selected_dive == -1)
				return;
			d = current_dive; // display the current dive
		}

		// No need to do this again if we are already showing the same dive
		// computer of the same dive, so we check the unique id of the dive
		// and the selected dive computer number against the ones we are
		// showing (can't compare the dive pointers as those might change).
		if (d->id == displayed_dive.id && dc_number == dataModel->dcShown() && !force)
			return;

		// this copies the dive and makes copies of all the relevant additional data
		copy_dive(d, &displayed_dive);
	} else {
		DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
		plannerModel->createTemporaryPlan();
		if (!plannerModel->getDiveplan().dp) {
			plannerModel->deleteTemporaryPlan();
			return;
		}
	}

	// special handling for the first time we display things
	int animSpeedBackup = 0;
	if (firstCall && MainWindow::instance()->filesFromCommandLine()) {
		animSpeedBackup = prefs.animation_speed;
		prefs.animation_speed = 0;
		firstCall = false;
	}

	// restore default zoom level
	if (zoomLevel) {
		const qreal defScale = 1.0 / qPow(zoomFactor, (qreal)zoomLevel);
		scale(defScale, defScale);
		zoomLevel = 0;
	}

	// reset some item visibility on printMode changes
	toolTipItem->setVisible(!printMode);
	rulerItem->setVisible(prefs.rulergraph && !printMode);
	tankItem->setVisible(prefs.tankbar);
	if (prefs.tankbar) {
		gasYAxis->setPos(itemPos.partialPressureWithTankBar.pos.on);
		gasYAxis->setLine(itemPos.partialPressureWithTankBar.expanded);
	} else {
		gasYAxis->setPos(itemPos.partialPressure.pos.on);
		gasYAxis->setLine(itemPos.partialPressure.expanded);
	}

	if (currentState == EMPTY)
		setProfileState();

	// next get the dive computer structure - if there are no samples
	// let's create a fake profile that's somewhat reasonable for the
	// data that we have
	struct divecomputer *currentdc = select_dc(&displayed_dive);
	Q_ASSERT(currentdc);
	if (!currentdc || !currentdc->samples) {
		currentdc = fake_dc(currentdc);
	}

	/* This struct holds all the data that's about to be plotted.
	 * I'm not sure this is the best approach ( but since we are
	 * interpolating some points of the Dive, maybe it is... )
	 * The  Calculation of the points should be done per graph,
	 * so I'll *not* calculate everything if something is not being
	 * shown.
	 */
	struct plot_info pInfo = calculate_max_limits_new(&displayed_dive, currentdc);
	create_plot_info_new(&displayed_dive, currentdc, &pInfo);
	if(shouldCalculateMaxTime)
		maxtime = get_maxtime(&pInfo);

	/* Only update the max depth if it's bigger than the current ones
	 * when we are dragging the handler to plan / add dive.
	 * otherwhise, update normally.
	 */
	int newMaxDepth = get_maxdepth(&pInfo);
	if(!shouldCalculateMaxDepth) {
		if (maxdepth < newMaxDepth) {
			maxdepth = newMaxDepth;
		}
	} else {
		maxdepth = newMaxDepth;
	}

	dataModel->setDive(&displayed_dive, pInfo);
	toolTipItem->setPlotInfo(pInfo);

	// It seems that I'll have a lot of boilerplate setting the model / axis for
	// each item, I'll mostly like to fix this in the future, but I'll keep at this for now.
	profileYAxis->setMaximum(maxdepth);
	profileYAxis->updateTicks();

	temperatureAxis->setMinimum(pInfo.mintemp);
	temperatureAxis->setMaximum(pInfo.maxtemp);

	if (pInfo.maxhr) {
		heartBeatAxis->setMinimum(pInfo.minhr);
		heartBeatAxis->setMaximum(pInfo.maxhr);
		heartBeatAxis->updateTicks(HR_AXIS); // this shows the ticks
	}
	heartBeatAxis->setVisible(prefs.hrgraph && pInfo.maxhr);

	percentageAxis->setMinimum(0);
	percentageAxis->setMaximum(100);
	percentageAxis->setVisible(false);
	percentageAxis->updateTicks(HR_AXIS);

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
	timeAxis->updateTicks();
	cylinderPressureAxis->setMinimum(pInfo.minpressure);
	cylinderPressureAxis->setMaximum(pInfo.maxpressure);

	rulerItem->setPlotInfo(pInfo);
	tankItem->setData(dataModel, &pInfo, &displayed_dive);
	meanDepth->setVisible(prefs.show_average_depth);
	meanDepth->setMeanDepth(pInfo.meandepth);
	meanDepth->setLine(0, 0, timeAxis->posAtValue(currentdc->duration.seconds), 0);
	Animations::moveTo(meanDepth,3, profileYAxis->posAtValue(pInfo.meandepth));

	dataModel->emitDataChanged();
	// The event items are a bit special since we don't know how many events are going to
	// exist on a dive, so I cant create cache items for that. that's why they are here
	// while all other items are up there on the constructor.
	qDeleteAll(eventItems);
	eventItems.clear();
	struct event *event = currentdc->events;
	while (event) {
		DiveEventItem *item = new DiveEventItem();
		item->setHorizontalAxis(timeAxis);
		item->setVerticalAxis(profileYAxis);
		item->setModel(dataModel);
		item->setEvent(event);
		item->setZValue(2);
		scene()->addItem(item);
		eventItems.push_back(item);
		event = event->next;
	}
	// Only set visible the events that should be visible
	Q_FOREACH (DiveEventItem *event, eventItems) {
		event->setVisible(!event->shouldBeHidden());
	}
	QString dcText = get_dc_nickname(currentdc->model, currentdc->deviceid);
	int nr;
	if ((nr = number_of_computers(&displayed_dive)) > 1)
		dcText += tr(" (#%1 of %2)").arg(dc_number + 1).arg(nr);
	diveComputerText->setText(dcText);
	if (MainWindow::instance()->filesFromCommandLine() && animSpeedBackup != 0) {
		prefs.animation_speed = animSpeedBackup;
	}

	if (currentState == ADD || currentState == PLAN) { // TODO: figure a way to move this from here.
		repositionDiveHandlers();
		DivePlannerPointsModel *model = DivePlannerPointsModel::instance();
		model->deleteTemporaryPlan();
	}
	plotPictures();

	// OK, how long did this take us? Anything above the second is way too long,
	// so if we are calculation TTS / NDL then let's force that off.
	if (measureDuration.elapsed() > 1000 && prefs.calcndltts) {
		MainWindow::instance()->turnOffNdlTts();
		MainWindow::instance()->showError("Show NDL / TTS was disabled because of excessive processing time");
	}
}

void ProfileWidget2::settingsChanged()
{
	// if we are showing calculated ceilings then we have to replot()
	// because the GF could have changed; otherwise we try to avoid replot()
	bool needReplot = prefs.calcceiling;
	if (PP_GRAPHS_ENABLED || prefs.hrgraph) {
		profileYAxis->animateChangeLine(itemPos.depth.shrinked);
		temperatureAxis->animateChangeLine(itemPos.temperature.shrinked);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.shrinked);
	} else {
		profileYAxis->animateChangeLine(itemPos.depth.expanded);
		temperatureAxis->animateChangeLine(itemPos.temperature.expanded);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.expanded);
	}
	if (prefs.tankbar) {
		gasYAxis->setPos(itemPos.partialPressureWithTankBar.pos.on);
		gasYAxis->animateChangeLine(itemPos.partialPressureWithTankBar.expanded);
	} else {
		gasYAxis->setPos(itemPos.partialPressure.pos.on);
		gasYAxis->animateChangeLine(itemPos.partialPressure.expanded);
	}
	tankItem->setVisible(prefs.tankbar);
	if (prefs.zoomed_plot != isPlotZoomed) {
		isPlotZoomed = prefs.zoomed_plot;
		needReplot = true;
	}
	if (needReplot)
		replot();
}

void ProfileWidget2::resizeEvent(QResizeEvent *event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
	fixBackgroundPos();
}

void ProfileWidget2::mousePressEvent(QMouseEvent *event)
{
	QGraphicsView::mousePressEvent(event);
	if(currentState == PLAN)
		shouldCalculateMaxTime = false;
}

void ProfileWidget2::divePlannerHandlerClicked()
{
	shouldCalculateMaxDepth = false;
	replot();
}

void ProfileWidget2::divePlannerHandlerReleased()
{
	shouldCalculateMaxDepth = true;
	replot();
}

void ProfileWidget2::mouseReleaseEvent(QMouseEvent *event)
{
	QGraphicsView::mouseReleaseEvent(event);
	if(currentState == PLAN){
		shouldCalculateMaxTime = true;
		replot();
	}
}

void ProfileWidget2::fixBackgroundPos()
{
	static QPixmap toBeScaled(backgroundFile);
	if (currentState != EMPTY)
		return;
	QPixmap p = toBeScaled.scaledToHeight(viewport()->height() - 40, Qt::SmoothTransformation);
	int x = viewport()->width() / 2 - p.width() / 2;
	int y = viewport()->height() / 2 - p.height() / 2;
	background->setPixmap(p);
	background->setX(mapToScene(x, 0).x());
	background->setY(mapToScene(y, 20).y());
}

void ProfileWidget2::wheelEvent(QWheelEvent *event)
{
	if (currentState == EMPTY)
		return;
	QPoint toolTipPos = mapFromScene(toolTipItem->pos());
	if(event->buttons() == Qt::LeftButton)
		return;
	if (event->delta() > 0 && zoomLevel < 20) {
		scale(zoomFactor, zoomFactor);
		zoomLevel++;
	} else if (event->delta() < 0 && zoomLevel > 0) {
		// Zooming out
		scale(1.0 / zoomFactor, 1.0 / zoomFactor);
		zoomLevel--;
	}
	scrollViewTo(event->pos());
	toolTipItem->setPos(mapToScene(toolTipPos));
}

void ProfileWidget2::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (currentState == PLAN || currentState == ADD) {
		DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
		QPointF mappedPos = mapToScene(event->pos());
		if (isPointOutOfBoundaries(mappedPos))
			return;

		int minutes = rint(timeAxis->valueAt(mappedPos) / 60);
		int milimeters = rint(profileYAxis->valueAt(mappedPos) / M_OR_FT(1, 1)) * M_OR_FT(1, 1);
		plannerModel->addStop(milimeters, minutes * 60, 0, 0, true);
	}
}

bool ProfileWidget2::isPointOutOfBoundaries(const QPointF &point) const
{
	double xpos = timeAxis->valueAt(point);
	double ypos = profileYAxis->valueAt(point);
	return (xpos > timeAxis->maximum() ||
		xpos < timeAxis->minimum() ||
		ypos > profileYAxis->maximum() ||
		ypos < profileYAxis->minimum());
}

void ProfileWidget2::scrollViewTo(const QPoint &pos)
{
	/* since we cannot use translate() directly on the scene we hack on
 * the scroll bars (hidden) functionality */
	if (!zoomLevel || currentState == EMPTY)
		return;
	QScrollBar *vs = verticalScrollBar();
	QScrollBar *hs = horizontalScrollBar();
	const qreal yRat = (qreal)pos.y() / viewport()->height();
	const qreal xRat = (qreal)pos.x() / viewport()->width();
	vs->setValue(yRat * vs->maximum());
	hs->setValue(xRat * hs->maximum());
}

void ProfileWidget2::mouseMoveEvent(QMouseEvent *event)
{
	toolTipItem->refresh(mapToScene(event->pos()));
	QPoint toolTipPos = mapFromScene(toolTipItem->pos());
	if (zoomLevel == 0) {
		QGraphicsView::mouseMoveEvent(event);
	} else {
		scrollViewTo(event->pos());
		toolTipItem->setPos(mapToScene(toolTipPos));
	}

	QPointF pos = mapToScene(event->pos());
	qreal vValue = profileYAxis->valueAt(pos);
	qreal hValue = timeAxis->valueAt(pos);
	if ( profileYAxis->maximum() >= vValue
		&& profileYAxis->minimum() <= vValue){
		mouseFollowerHorizontal->setPos(timeAxis->pos().x(), pos.y());
	}
	if ( timeAxis->maximum() >= hValue
		&& timeAxis->minimum() <= hValue){
		mouseFollowerVertical->setPos(pos.x(), profileYAxis->line().y1());
	}


}

bool ProfileWidget2::eventFilter(QObject *object, QEvent *event)
{
	QGraphicsScene *s = qobject_cast<QGraphicsScene *>(object);
	if (s && event->type() == QEvent::GraphicsSceneHelp) {
		event->ignore();
		return true;
	}
	return QGraphicsView::eventFilter(object, event);
}

void ProfileWidget2::setEmptyState()
{
	// Then starting Empty State, move the background up.
	if (currentState == EMPTY)
		return;

	disconnectTemporaryConnections();
	setBackgroundBrush(getColor(::BACKGROUND, isGrayscale));
	dataModel->clear();
	currentState = EMPTY;
	MainWindow::instance()->setEnabledToolbar(false);

	fixBackgroundPos();
	background->setVisible(true);

	profileYAxis->setVisible(false);
	gasYAxis->setVisible(false);
	timeAxis->setVisible(false);
	temperatureAxis->setVisible(false);
	cylinderPressureAxis->setVisible(false);
	toolTipItem->setVisible(false);
	meanDepth->setVisible(false);
	diveComputerText->setVisible(false);
	diveCeiling->setVisible(false);
	reportedCeiling->setVisible(false);
	rulerItem->setVisible(false);
	tankItem->setVisible(false);
	pn2GasItem->setVisible(false);
	po2GasItem->setVisible(false);
	pheGasItem->setVisible(false);
	ambPressureItem->setVisible(false);
	gflineItem->setVisible(false);
	mouseFollowerHorizontal->setVisible(false);
	mouseFollowerVertical->setVisible(false);

	#define HIDE_ALL(TYPE, CONTAINER) \
	Q_FOREACH (TYPE *item, CONTAINER) item->setVisible(false);
	HIDE_ALL(DiveCalculatedTissue, allTissues);
	HIDE_ALL(DivePercentageItem, allPercentages);
	HIDE_ALL(DiveEventItem, eventItems);
	HIDE_ALL(DiveHandler, handles);
	HIDE_ALL(QGraphicsSimpleTextItem, gases);
	#undef HIDE_ALL
}

void ProfileWidget2::setProfileState()
{
	// Then starting Empty State, move the background up.
	if (currentState == PROFILE)
		return;

	disconnectTemporaryConnections();
	connect(DivePictureModel::instance(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(plotPictures()));
	connect(DivePictureModel::instance(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),this, SLOT(plotPictures()));
	connect(DivePictureModel::instance(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)),this, SLOT(plotPictures()));
	/* show the same stuff that the profile shows. */

	//TODO: Move the DC handling to another method.
	MainWindow::instance()->enableDcShortcuts();

	currentState = PROFILE;
	MainWindow::instance()->setEnabledToolbar(true);
	toolTipItem->readPos();
	setBackgroundBrush(getColor(::BACKGROUND, isGrayscale));

	background->setVisible(false);
	toolTipItem->setVisible(true);
	profileYAxis->setVisible(true);
	gasYAxis->setVisible(true);
	timeAxis->setVisible(true);
	temperatureAxis->setVisible(true);
	cylinderPressureAxis->setVisible(true);

	profileYAxis->setPos(itemPos.depth.pos.on);
	if (prefs.pp_graphs.phe || prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.hrgraph) {
		profileYAxis->setLine(itemPos.depth.shrinked);
		temperatureAxis->setLine(itemPos.temperature.shrinked);
		cylinderPressureAxis->setLine(itemPos.cylinder.shrinked);
	} else {
		profileYAxis->setLine(itemPos.depth.expanded);
		temperatureAxis->setLine(itemPos.temperature.expanded);
		cylinderPressureAxis->setLine(itemPos.cylinder.expanded);
	}
	pn2GasItem->setVisible(prefs.pp_graphs.pn2);
	po2GasItem->setVisible(prefs.pp_graphs.po2);
	pheGasItem->setVisible(prefs.pp_graphs.phe);

	if (prefs.tankbar) {
		gasYAxis->setPos(itemPos.partialPressureWithTankBar.pos.on);
		gasYAxis->setLine(itemPos.partialPressureWithTankBar.expanded);
	} else {
		gasYAxis->setPos(itemPos.partialPressure.pos.on);
		gasYAxis->setLine(itemPos.partialPressure.expanded);
	}
	timeAxis->setPos(itemPos.time.pos.on);
	timeAxis->setLine(itemPos.time.expanded);

	cylinderPressureAxis->setPos(itemPos.cylinder.pos.on);
	temperatureAxis->setPos(itemPos.temperature.pos.on);
	heartBeatAxis->setPos(itemPos.heartBeat.pos.on);
	heartBeatAxis->setLine(itemPos.heartBeat.expanded);
	heartBeatItem->setVisible(prefs.hrgraph);
	meanDepth->setVisible(true);

	diveComputerText->setVisible(true);
	diveComputerText->setPos(itemPos.dcLabel.on);

	diveCeiling->setVisible(prefs.calcceiling);
	reportedCeiling->setVisible(prefs.dcceiling);

	if (prefs.calcalltissues) {
		Q_FOREACH (DiveCalculatedTissue *tissue, allTissues) {
			tissue->setVisible(true);
		}
	}
	percentageAxis->setPos(itemPos.percentage.pos.on);
	percentageAxis->setLine(itemPos.percentage.expanded);
	if (prefs.percentagegraph) {
		Q_FOREACH (DivePercentageItem *percentage, allPercentages) {
			percentage->setVisible(true);
		}

		ambPressureItem->setVisible(true);
	}	gflineItem->setVisible(true);

	rulerItem->setVisible(prefs.rulergraph);
	tankItem->setVisible(prefs.tankbar);
	tankItem->setPos(itemPos.tankBar.on);

	#define HIDE_ALL(TYPE, CONTAINER) \
	Q_FOREACH (TYPE *item, CONTAINER) item->setVisible(false);
	HIDE_ALL(DiveHandler, handles);
	HIDE_ALL(QGraphicsSimpleTextItem, gases);
	#undef HIDE_ALL
	mouseFollowerHorizontal->setVisible(false);
	mouseFollowerVertical->setVisible(false);
}

void ProfileWidget2::clearHandlers()
{
	if (handles.count()) {
		foreach (DiveHandler *handle, handles) {
			scene()->removeItem(handle);
		}
		handles.clear();
	}
}

void ProfileWidget2::setAddState()
{
	if (currentState == ADD)
		return;

	setProfileState();
	mouseFollowerHorizontal->setVisible(true);
	mouseFollowerVertical->setVisible(true);
	mouseFollowerHorizontal->setLine(timeAxis->line());
	mouseFollowerVertical->setLine(QLineF(0, profileYAxis->pos().y(), 0, timeAxis->pos().y()));
	disconnectTemporaryConnections();
	//TODO: Move this method to another place, shouldn't be on mainwindow.
	MainWindow::instance()->disableDcShortcuts();
	actionsForKeys[Qt::Key_Left]->setShortcut(Qt::Key_Left);
	actionsForKeys[Qt::Key_Right]->setShortcut(Qt::Key_Right);
	actionsForKeys[Qt::Key_Up]->setShortcut(Qt::Key_Up);
	actionsForKeys[Qt::Key_Down]->setShortcut(Qt::Key_Down);
	actionsForKeys[Qt::Key_Escape]->setShortcut(Qt::Key_Escape);
	actionsForKeys[Qt::Key_Delete]->setShortcut(Qt::Key_Delete);

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	connect(plannerModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(replot()));
	connect(plannerModel, SIGNAL(cylinderModelEdited()), this, SLOT(replot()));
	connect(plannerModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
		this, SLOT(pointInserted(const QModelIndex &, int, int)));
	connect(plannerModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
		this, SLOT(pointsRemoved(const QModelIndex &, int, int)));
	/* show the same stuff that the profile shows. */
	currentState = ADD; /* enable the add state. */
	diveCeiling->setVisible(true);
	setBackgroundBrush(QColor("#A7DCFF"));
}

void ProfileWidget2::setPlanState()
{
	if (currentState == PLAN)
		return;

	setProfileState();
	mouseFollowerHorizontal->setVisible(true);
	mouseFollowerVertical->setVisible(true);
	mouseFollowerHorizontal->setLine(timeAxis->line());
	mouseFollowerVertical->setLine(QLineF(0, profileYAxis->pos().y(), 0, timeAxis->pos().y()));
	disconnectTemporaryConnections();
	//TODO: Move this method to another place, shouldn't be on mainwindow.
	MainWindow::instance()->disableDcShortcuts();
	actionsForKeys[Qt::Key_Left]->setShortcut(Qt::Key_Left);
	actionsForKeys[Qt::Key_Right]->setShortcut(Qt::Key_Right);
	actionsForKeys[Qt::Key_Up]->setShortcut(Qt::Key_Up);
	actionsForKeys[Qt::Key_Down]->setShortcut(Qt::Key_Down);
	actionsForKeys[Qt::Key_Escape]->setShortcut(Qt::Key_Escape);
	actionsForKeys[Qt::Key_Delete]->setShortcut(Qt::Key_Delete);

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	connect(plannerModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(replot()));
	connect(plannerModel, SIGNAL(cylinderModelEdited()), this, SLOT(replot()));
	connect(plannerModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
		this, SLOT(pointInserted(const QModelIndex &, int, int)));
	connect(plannerModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
		this, SLOT(pointsRemoved(const QModelIndex &, int, int)));
	/* show the same stuff that the profile shows. */
	currentState = PLAN; /* enable the add state. */
	diveCeiling->setVisible(true);
	setBackgroundBrush(QColor("#D7E3EF"));
}

extern struct ev_select *ev_namelist;
extern int evn_allocated;
extern int evn_used;

bool ProfileWidget2::isPlanner()
{
	return currentState == PLAN;
}

bool ProfileWidget2::isAddOrPlanner()
{
	return currentState == PLAN || currentState == ADD;
}

void ProfileWidget2::contextMenuEvent(QContextMenuEvent *event)
{
	if (currentState == ADD || currentState == PLAN) {
		QGraphicsView::contextMenuEvent(event);
		return;
	}
	QMenu m;
	bool isDCName = false;
	if (selected_dive == -1)
		return;
	// figure out if we are ontop of the dive computer name in the profile
	QGraphicsItem *sceneItem = itemAt(mapFromGlobal(event->globalPos()));
	if (sceneItem) {
		QGraphicsItem *parentItem = sceneItem;
		while (parentItem) {
			if (parentItem->data(SUBSURFACE_OBJ_DATA) == SUBSURFACE_OBJ_DC_TEXT) {
				isDCName = true;
				break;
			}
			parentItem = parentItem->parentItem();
		}
		if (isDCName) {
			if (dc_number == 0 && count_divecomputers() == 1)
				// nothing to do, can't delete or reorder
				return;
			// create menu to show when right clicking on dive computer name
			if (dc_number > 0)
				m.addAction(tr("Make first divecomputer"), this, SLOT(makeFirstDC()));
			if (count_divecomputers() > 1)
				m.addAction(tr("Delete this divecomputer"), this, SLOT(deleteCurrentDC()));
			m.exec(event->globalPos());
			// don't show the regular profile context menu
			return;
		}
	}
	// create the profile context menu
	QMenu *gasChange = m.addMenu(tr("Add gas change"));
	GasSelectionModel *model = GasSelectionModel::instance();
	model->repopulate();
	int rowCount = model->rowCount();
	for (int i = 0; i < rowCount; i++) {
		QAction *action = new QAction(&m);
		action->setText(model->data(model->index(i, 0), Qt::DisplayRole).toString());
		connect(action, SIGNAL(triggered(bool)), this, SLOT(changeGas()));
		action->setData(event->globalPos());
		gasChange->addAction(action);
	}
	QAction *action = m.addAction(tr("Add bookmark"), this, SLOT(addBookmark()));
	action->setData(event->globalPos());
	if (DiveEventItem *item = dynamic_cast<DiveEventItem *>(sceneItem)) {
		action = new QAction(&m);
		action->setText(tr("Remove event"));
		action->setData(QVariant::fromValue<void *>(item)); // so we know what to remove.
		connect(action, SIGNAL(triggered(bool)), this, SLOT(removeEvent()));
		m.addAction(action);
		action = new QAction(&m);
		action->setText(tr("Hide similar events"));
		action->setData(QVariant::fromValue<void *>(item));
		connect(action, SIGNAL(triggered(bool)), this, SLOT(hideEvents()));
		m.addAction(action);
		if (item->getEvent()->type == SAMPLE_EVENT_BOOKMARK) {
			action = new QAction(&m);
			action->setText(tr("Edit name"));
			action->setData(QVariant::fromValue<void *>(item));
			connect(action, SIGNAL(triggered(bool)), this, SLOT(editName()));
			m.addAction(action);
		}
	}
	bool some_hidden = false;
	for (int i = 0; i < evn_used; i++) {
		if (ev_namelist[i].plot_ev == false) {
			some_hidden = true;
			break;
		}
	}
	if (some_hidden) {
		action = m.addAction(tr("Unhide all events"), this, SLOT(unhideEvents()));
		action->setData(event->globalPos());
	}
	m.exec(event->globalPos());
}

void ProfileWidget2::deleteCurrentDC()
{
	delete_current_divecomputer();
	mark_divelist_changed(true);
	// we need to force it since it's likely the same dive and same dc_number - but that's a different dive computer now
	MainWindow::instance()->graphics()->plotDive(0, true);
	MainWindow::instance()->refreshDisplay();
}

void ProfileWidget2::makeFirstDC()
{
	make_first_dc();
	mark_divelist_changed(true);
	// this is now the first DC, so we need to redraw the profile and refresh the dive list
	// (and no, it's not just enough to rewrite the text - the first DC is special so values in the
	// dive list may change).
	// As a side benefit, this returns focus to the dive list.
	dc_number = 0;
	MainWindow::instance()->refreshDisplay();
}

void ProfileWidget2::hideEvents()
{
	QAction *action = qobject_cast<QAction *>(sender());
	DiveEventItem *item = static_cast<DiveEventItem *>(action->data().value<void *>());
	struct event *event = item->getEvent();

	if (QMessageBox::question(MainWindow::instance(),
				  TITLE_OR_TEXT(tr("Hide events"), tr("Hide all %1 events?").arg(event->name)),
				  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
		if (event->name) {
			for (int i = 0; i < evn_used; i++) {
				if (same_string(event->name, ev_namelist[i].ev_name)) {
					ev_namelist[i].plot_ev = false;
					break;
				}
			}
			Q_FOREACH (DiveEventItem *evItem, eventItems) {
				if (same_string(evItem->getEvent()->name, event->name))
					evItem->hide();
			}
		} else {
			item->hide();
		}
	}
}

void ProfileWidget2::unhideEvents()
{
	for (int i = 0; i < evn_used; i++) {
		ev_namelist[i].plot_ev = true;
	}
	Q_FOREACH (DiveEventItem *item, eventItems)
		item->show();
}

void ProfileWidget2::removeEvent()
{
	QAction *action = qobject_cast<QAction *>(sender());
	DiveEventItem *item = static_cast<DiveEventItem *>(action->data().value<void *>());
	struct event *event = item->getEvent();

	if (QMessageBox::question(MainWindow::instance(), TITLE_OR_TEXT(
								  tr("Remove the selected event?"),
								  tr("%1 @ %2:%3").arg(event->name).arg(event->time.seconds / 60).arg(event->time.seconds % 60, 2, 10, QChar('0'))),
				  QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok) {
		remove_event(event);
		mark_divelist_changed(true);
		replot();
	}
}

void ProfileWidget2::addBookmark()
{
	QAction *action = qobject_cast<QAction *>(sender());
	QPointF scenePos = mapToScene(mapFromGlobal(action->data().toPoint()));
	add_event(current_dc, timeAxis->valueAt(scenePos), SAMPLE_EVENT_BOOKMARK, 0, 0, "bookmark");
	mark_divelist_changed(true);
	replot();
}

void ProfileWidget2::changeGas()
{
	QAction *action = qobject_cast<QAction *>(sender());
	QPointF scenePos = mapToScene(mapFromGlobal(action->data().toPoint()));
	QString gas = action->text();
	// backup the things on the dataModel, since we will clear that out.
	struct gasmix gasmix;
	int seconds = timeAxis->valueAt(scenePos);

	validate_gas(gas.toUtf8().constData(), &gasmix);
	add_gas_switch_event(&displayed_dive, current_dc, seconds, get_gasidx(&displayed_dive, &gasmix));
	// this means we potentially have a new tank that is being used and needs to be shown
	fixup_dive(&displayed_dive);

	// FIXME - this no longer gets written to the dive list - so we need to enableEdition() here

	MainWindow::instance()->information()->updateDiveInfo();
	mark_divelist_changed(true);
	replot();
}

bool ProfileWidget2::getPrintMode()
{
	return printMode;
}

void ProfileWidget2::setPrintMode(bool mode, bool grayscale)
{
	printMode = mode;
	isGrayscale = mode ? grayscale : false;
	mouseFollowerHorizontal->setVisible( !mode );
	mouseFollowerVertical->setVisible( !mode );
}

void ProfileWidget2::setFontPrintScale(double scale)
{
	fontPrintScale = scale;
}

double ProfileWidget2::getFontPrintScale()
{
	if (printMode)
		return fontPrintScale;
	else
		return 1.0;
}

void ProfileWidget2::editName()
{
	QAction *action = qobject_cast<QAction *>(sender());
	DiveEventItem *item = static_cast<DiveEventItem *>(action->data().value<void *>());
	struct event *event = item->getEvent();
	bool ok;
	QString newName = QInputDialog::getText(MainWindow::instance(), tr("Edit name of bookmark"),
						tr("Custom name:"), QLineEdit::Normal,
						event->name, &ok);
	if (ok && !newName.isEmpty()) {
		if (newName.length() > 22) { //longer names will display as garbage.
			QMessageBox lengthWarning;
			lengthWarning.setText(tr("Name is too long!"));
			lengthWarning.exec();
			return;
		}
		// order is important! first update the current dive (by matching the unchanged event),
		// then update the displayed dive (as event is part of the events on displayed dive
		// and will be freed as part of changing the name!
		update_event_name(current_dive, event, newName.toUtf8().data());
		update_event_name(&displayed_dive, event, newName.toUtf8().data());
		mark_divelist_changed(true);
		replot();
	}
}

void ProfileWidget2::disconnectTemporaryConnections()
{
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	disconnect(plannerModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(replot()));
	disconnect(plannerModel, SIGNAL(cylinderModelEdited()), this, SLOT(replot()));

	disconnect(plannerModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
		   this, SLOT(pointInserted(const QModelIndex &, int, int)));
	disconnect(plannerModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
		   this, SLOT(pointsRemoved(const QModelIndex &, int, int)));

	Q_FOREACH (QAction *action, actionsForKeys.values()) {
		action->setShortcut(QKeySequence());
		action->setShortcutContext(Qt::WidgetShortcut);
	}
}

void ProfileWidget2::pointInserted(const QModelIndex &parent, int start, int end)
{
	DiveHandler *item = new DiveHandler();
	scene()->addItem(item);
	handles << item;

	connect(item, SIGNAL(moved()), this, SLOT(recreatePlannedDive()));
	connect(item, SIGNAL(clicked()), this, SLOT(divePlannerHandlerClicked()));
	connect(item, SIGNAL(released()), this, SLOT(divePlannerHandlerReleased()));
	QGraphicsSimpleTextItem *gasChooseBtn = new QGraphicsSimpleTextItem();
	scene()->addItem(gasChooseBtn);
	gasChooseBtn->setZValue(10);
	gasChooseBtn->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	gases << gasChooseBtn;
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	if (plannerModel->recalcQ())
		replot();
}

void ProfileWidget2::pointsRemoved(const QModelIndex &, int start, int end)
{ // start and end are inclusive.
	int num = (end - start) + 1;
	for (int i = num; i != 0; i--) {
		delete handles.back();
		handles.pop_back();
		delete gases.back();
		gases.pop_back();
	}
	scene()->clearSelection();
	replot();
}

void ProfileWidget2::repositionDiveHandlers()
{
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	// Re-position the user generated dive handlers
	int last = 0;
	for (int i = 0; i < plannerModel->rowCount(); i++) {
		struct divedatapoint datapoint = plannerModel->at(i);
		if (datapoint.time == 0) // those are the magic entries for tanks
			continue;
		DiveHandler *h = handles.at(i);
		h->setVisible(datapoint.entered);
		h->setPos(timeAxis->posAtValue(datapoint.time), profileYAxis->posAtValue(datapoint.depth));
		QPointF p1 = (last == i) ? QPointF(timeAxis->posAtValue(0), profileYAxis->posAtValue(0)) : handles[last]->pos();
		QPointF p2 = handles[i]->pos();
		QLineF line(p1, p2);
		QPointF pos = line.pointAt(0.5);
		gases[i]->setPos(pos);
		gases[i]->setVisible(datapoint.entered);
		gases[i]->setText(dpGasToStr(plannerModel->at(i)));
		last = i;
	}
}

int ProfileWidget2::fixHandlerIndex(DiveHandler *activeHandler)
{
	int index = handles.indexOf(activeHandler);
	if (index > 0 && index < handles.count() - 1) {
		DiveHandler *before = handles[index - 1];
		if (before->pos().x() > activeHandler->pos().x()) {
			handles.swap(index, index - 1);
			return index - 1;
		}
		DiveHandler *after = handles[index + 1];
		if (after->pos().x() < activeHandler->pos().x()) {
			handles.swap(index, index + 1);
			return index + 1;
		}
	}
	return index;
}

void ProfileWidget2::recreatePlannedDive()
{
	DiveHandler *activeHandler = qobject_cast<DiveHandler *>(sender());
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	int index = fixHandlerIndex(activeHandler);
	int mintime = 0, maxtime = (timeAxis->maximum() + 10) * 60;
	if (index > 0)
		mintime = plannerModel->at(index - 1).time;
	if (index < plannerModel->size() - 1)
		maxtime = plannerModel->at(index + 1).time;

	int minutes = rint(timeAxis->valueAt(activeHandler->pos()) / 60);
	if (minutes * 60 <= mintime || minutes * 60 >= maxtime)
		return;

	divedatapoint data = plannerModel->at(index);
	data.depth = rint(profileYAxis->valueAt(activeHandler->pos()) / M_OR_FT(1, 1)) * M_OR_FT(1, 1);
	data.time = rint(timeAxis->valueAt(activeHandler->pos()));

	plannerModel->editStop(index, data);
}

void ProfileWidget2::keyDownAction()
{
	if (currentState != ADD && currentState != PLAN)
		return;

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	Q_FOREACH (QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);
			if (dp.depth >= profileYAxis->maximum())
				continue;

			dp.depth += M_OR_FT(1, 5);
			plannerModel->editStop(row, dp);
		}
	}
}

void ProfileWidget2::keyUpAction()
{
	if (currentState != ADD && currentState != PLAN)
		return;

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	Q_FOREACH (QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);

			if (dp.depth <= 0)
				continue;

			dp.depth -= M_OR_FT(1, 5);
			plannerModel->editStop(row, dp);
		}
	}
}

void ProfileWidget2::keyLeftAction()
{
	if (currentState != ADD && currentState != PLAN)
		return;

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	Q_FOREACH (QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);

			if (dp.time / 60 <= 0)
				continue;

			// don't overlap positions.
			// maybe this is a good place for a 'goto'?
			double xpos = timeAxis->posAtValue((dp.time - 60) / 60);
			bool nextStep = false;
			Q_FOREACH (DiveHandler *h, handles) {
				if (IS_FP_SAME(h->pos().x(), xpos)) {
					nextStep = true;
					break;
				}
			}
			if (nextStep)
				continue;

			dp.time -= 60;
			plannerModel->editStop(row, dp);
		}
	}
}

void ProfileWidget2::keyRightAction()
{
	if (currentState != ADD && currentState != PLAN)
		return;

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	Q_FOREACH (QGraphicsItem *i, scene()->selectedItems()) {
		if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
			int row = handles.indexOf(handler);
			divedatapoint dp = plannerModel->at(row);
			if (dp.time / 60 >= timeAxis->maximum())
				continue;

			// don't overlap positions.
			// maybe this is a good place for a 'goto'?
			double xpos = timeAxis->posAtValue((dp.time + 60) / 60);
			bool nextStep = false;
			Q_FOREACH (DiveHandler *h, handles) {
				if (IS_FP_SAME(h->pos().x(), xpos)) {
					nextStep = true;
					break;
				}
			}
			if (nextStep)
				continue;

			dp.time += 60;
			plannerModel->editStop(row, dp);
		}
	}
}

void ProfileWidget2::keyDeleteAction()
{
	if (currentState != ADD && currentState != PLAN)
		return;

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	int selCount = scene()->selectedItems().count();
	if (selCount) {
		QVector<int> selectedIndexes;
		Q_FOREACH (QGraphicsItem *i, scene()->selectedItems()) {
			if (DiveHandler *handler = qgraphicsitem_cast<DiveHandler *>(i)) {
				selectedIndexes.push_back(handles.indexOf(handler));
				handler->hide();
			}
		}
		plannerModel->removeSelectedPoints(selectedIndexes);
	}
}

void ProfileWidget2::keyEscAction()
{
	if (currentState != ADD && currentState != PLAN)
		return;

	if (scene()->selectedItems().count()) {
		scene()->clearSelection();
		return;
	}

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	if (plannerModel->isPlanner())
		plannerModel->cancelPlan();
}

void ProfileWidget2::plotPictures()
{
	Q_FOREACH(DivePictureItem *item, pictures){
		item->hide();
		item->deleteLater();
	}
	pictures.clear();

	if (printMode)
		return;

	double x, y, lastX = -1.0, lastY = -1.0;
	DivePictureModel *m = DivePictureModel::instance();
	for (int i = 0; i < m->rowCount(); i++) {
		int offsetSeconds = m->index(i,1).data(Qt::UserRole).value<int>();
		// it's a correct picture, but doesn't have a timestamp: only show on the widget near the
		// information area.
		if (!offsetSeconds)
			continue;
		DivePictureItem *item = new DivePictureItem();
		item->setPixmap(m->index(i,0).data(Qt::DecorationRole).value<QPixmap>());
		item->setFileUrl(m->index(i,1).data().toString());
		// let's put the picture at the correct time, but at a fixed "depth" on the profile
		// not sure this is ideal, but it seems to look right.
		x = timeAxis->posAtValue(offsetSeconds);
		if (i == 0)
			y = 10;
		else if (fabs(x - lastX) < 4)
			y = lastY + 3;
		lastX = x;
		lastY = y;
		item->setPos(x, y);
		scene()->addItem(item);
		pictures.push_back(item);
	}
}
