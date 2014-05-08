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
#include "dive.h"
#include "pref.h"
#include <libdivecomputer/parser.h>
#include <QSignalTransition>
#include <QPropertyAnimation>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDebug>
#include <QSettings>
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
	_Axis depth;
	_Axis partialPressure;
	_Axis time;
	_Axis cylinder;
	_Axis temperature;
	_Axis heartBeat;
} itemPos;

ProfileWidget2::ProfileWidget2(QWidget *parent) : QGraphicsView(parent),
	dataModel(new DivePlotDataModel(this)),
	currentState(INVALID),
	zoomLevel(0),
	zoomFactor(1.15),
	background(new DivePixmapItem()),
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
	rulerItem(new RulerItem2()),
	isGrayscale(false),
	printMode(false)
{
	memset(&plotInfo, 0, sizeof(plotInfo));

	setupSceneAndFlags();
	setupItemSizes();
	setupItemOnScene();
	addItemsToScene();
	scene()->installEventFilter(this);
	setEmptyState();
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

#ifndef QT_NO_DEBUG
	QTableView *diveDepthTableView = new QTableView();
	diveDepthTableView->setModel(dataModel);
	diveDepthTableView->show();
#endif
}

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
	scene()->addItem(diveComputerText);
	scene()->addItem(diveCeiling);
	scene()->addItem(reportedCeiling);
	scene()->addItem(pn2GasItem);
	scene()->addItem(pheGasItem);
	scene()->addItem(po2GasItem);
	scene()->addItem(heartBeatAxis);
	scene()->addItem(heartBeatItem);
	scene()->addItem(rulerItem);
	scene()->addItem(rulerItem->sourceNode());
	scene()->addItem(rulerItem->destNode());
	Q_FOREACH(DiveCalculatedTissue * tissue, allTissues) {
		scene()->addItem(tissue);
	}
}

void ProfileWidget2::setupItemOnScene()
{
	background->setZValue(9999);
	toolTipItem->setZValue(9998);
	toolTipItem->setTimeAxis(timeAxis);
	rulerItem->setZValue(9997);

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

	setupItem(reportedCeiling, timeAxis, profileYAxis, dataModel, DivePlotDataModel::CEILING, DivePlotDataModel::TIME, 1);
	setupItem(diveCeiling, timeAxis, profileYAxis, dataModel, DivePlotDataModel::CEILING, DivePlotDataModel::TIME, 1);
	for (int i = 0; i < 16; i++) {
		DiveCalculatedTissue *tissueItem = new DiveCalculatedTissue();
		setupItem(tissueItem, timeAxis, profileYAxis, dataModel, DivePlotDataModel::TISSUE_1 + i, DivePlotDataModel::TIME, 1 + i);
		allTissues.append(tissueItem);
	}
	setupItem(gasPressureItem, timeAxis, cylinderPressureAxis, dataModel, DivePlotDataModel::TEMPERATURE, DivePlotDataModel::TIME, 1);
	setupItem(temperatureItem, timeAxis, temperatureAxis, dataModel, DivePlotDataModel::TEMPERATURE, DivePlotDataModel::TIME, 1);
	setupItem(heartBeatItem, timeAxis, heartBeatAxis, dataModel, DivePlotDataModel::HEARTBEAT, DivePlotDataModel::TIME, 1);
	heartBeatItem->setVisibilitySettingsKey("hrgraph");
	heartBeatItem->preferencesChanged();
	setupItem(diveProfileItem, timeAxis, profileYAxis, dataModel, DivePlotDataModel::DEPTH, DivePlotDataModel::TIME, 0);

#define CREATE_PP_GAS(ITEM, VERTICAL_COLUMN, COLOR, COLOR_ALERT, THRESHOULD_SETTINGS, VISIBILITY_SETTINGS)              \
	setupItem(ITEM, timeAxis, gasYAxis, dataModel, DivePlotDataModel::VERTICAL_COLUMN, DivePlotDataModel::TIME, 0); \
	ITEM->setThreshouldSettingsKey(THRESHOULD_SETTINGS);                                                            \
	ITEM->setVisibilitySettingsKey(VISIBILITY_SETTINGS);                                                            \
	ITEM->setColors(getColor(COLOR, isGrayscale), getColor(COLOR_ALERT, isGrayscale));                              \
	ITEM->preferencesChanged();                                                                                     \
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
}

void ProfileWidget2::replot()
{
	int diveId = dataModel->id();
	dataModel->clear();
	plotDives(QList<dive *>() << getDiveById(diveId));
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
	itemPos.partialPressure.expanded.setP2(QPointF(0, 30));

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
	itemPos.heartBeat.expanded.setP2(QPointF(0, 20));

	itemPos.dcLabel.on.setX(3);
	itemPos.dcLabel.on.setY(100);
	itemPos.dcLabel.off.setX(-10);
	itemPos.dcLabel.off.setY(100);
}

void ProfileWidget2::setupItem(AbstractProfilePolygonItem *item, DiveCartesianAxis *hAxis, DiveCartesianAxis *vAxis, DivePlotDataModel *model, int vData, int hData, int zValue)
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
void ProfileWidget2::plotDives(QList<dive *> dives)
{
	static bool firstCall = true;

	// I Know that it's a list, but currently we are
	// using just the first.
	struct dive *d = dives.first();
	if (!d)
		return;

	int animSpeedBackup = -1;
	if (firstCall && MainWindow::instance()->filesFromCommandLine()) {
		animSpeedBackup = prefs.animation;
		prefs.animation = 0;
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
	QSettings s;
	s.beginGroup("TecDetails");
	const bool rulerVisible = s.value("rulergraph", false).toBool() && !printMode;
	rulerItem->setVisible(rulerVisible);
	rulerItem->sourceNode()->setVisible(rulerVisible);
	rulerItem->destNode()->setVisible(rulerVisible);

	// No need to do this again if we are already showing the same dive
	// computer of the same dive, so we check the unique id of the dive
	// and the selected dive computer number against the ones we are
	// showing (can't compare the dive pointers as those might change).
	// I'm unclear what the semantics are supposed to be if we actually
	// use more than one 'dives' as argument - so ignoring that right now :-)
	if (d->id == dataModel->id() && dc_number == dataModel->dcShown())
		return;

	setProfileState();

	// next get the dive computer structure - if there are no samples
	// let's create a fake profile that's somewhat reasonable for the
	// data that we have
	struct divecomputer *currentdc = select_dc(d);
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
	struct plot_info pInfo = calculate_max_limits_new(d, currentdc);
	create_plot_info_new(d, currentdc, &pInfo);
	int maxtime = get_maxtime(&pInfo);
	int maxdepth = get_maxdepth(&pInfo);

	dataModel->setDive(d, pInfo);
	toolTipItem->setPlotInfo(pInfo);

	// It seems that I'll have a lot of boilerplate setting the model / axis for
	// each item, I'll mostly like to fix this in the future, but I'll keep at this for now.
	profileYAxis->setMaximum(maxdepth);
	profileYAxis->updateTicks();

	temperatureAxis->setMinimum(pInfo.mintemp);
	temperatureAxis->setMaximum(pInfo.maxtemp);

	if (heartBeatItem->isVisible()) {
		if (pInfo.maxhr) {
			heartBeatAxis->setMinimum(pInfo.minhr);
			heartBeatAxis->setMaximum(pInfo.maxhr);
			heartBeatAxis->updateTicks(HR_AXIS); // this shows the ticks
			heartBeatAxis->setVisible(true);
		} else {
			heartBeatAxis->setVisible(false);
		}
	} else {
		heartBeatAxis->setVisible(false);
	}
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
	if (prefs.show_average_depth)
		meanDepth->setVisible(true);
	else
		meanDepth->setVisible(false);
	meanDepth->setMeanDepth(pInfo.meandepth);
	meanDepth->setLine(0, 0, timeAxis->posAtValue(d->duration.seconds), 0);
	meanDepth->animateMoveTo(3, profileYAxis->posAtValue(pInfo.meandepth));

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
	Q_FOREACH(DiveEventItem * event, eventItems) {
		event->setVisible(!event->shouldBeHidden());
		// qDebug() << event->getEvent()->name << "@" << event->getEvent()->time.seconds << "is hidden:" << event->isHidden();
	}
	diveComputerText->setText(currentdc->model);
	if (MainWindow::instance()->filesFromCommandLine() && animSpeedBackup != -1) {
		prefs.animation = animSpeedBackup;
	}
}

void ProfileWidget2::settingsChanged()
{
	// if we are showing calculated ceilings then we have to replot()
	// because the GF could have changed; otherwise we try to avoid replot()
	bool needReplot = prefs.calcceiling;
	if (PP_GRAPHS_ENABLED) {
		profileYAxis->animateChangeLine(itemPos.depth.shrinked);
		temperatureAxis->animateChangeLine(itemPos.temperature.shrinked);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.shrinked);
	} else {
		profileYAxis->animateChangeLine(itemPos.depth.expanded);
		temperatureAxis->animateChangeLine(itemPos.temperature.expanded);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.expanded);
	}
	if (prefs.zoomed_plot != isPlotZoomed) {
		isPlotZoomed = prefs.zoomed_plot;
		needReplot = true;
	}

	if (currentState == PROFILE) {
		rulerItem->setVisible(prefs.rulergraph);
		rulerItem->destNode()->setVisible(prefs.rulergraph);
		rulerItem->sourceNode()->setVisible(prefs.rulergraph);
		needReplot = true;
	} else {
		rulerItem->setVisible(false);
		rulerItem->destNode()->setVisible(false);
		rulerItem->sourceNode()->setVisible(false);
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

void ProfileWidget2::fixBackgroundPos()
{
	if (currentState != EMPTY)
		return;
	QPixmap toBeScaled = QPixmap(backgroundFile);
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

	dataModel->clear();
	currentState = EMPTY;
	MainWindow::instance()->setToolButtonsEnabled(false);

	backgroundFile = QString(":poster");
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
	rulerItem->destNode()->setVisible(false);
	rulerItem->sourceNode()->setVisible(false);
	pn2GasItem->setVisible(false);
	po2GasItem->setVisible(false);
	pheGasItem->setVisible(false);
	Q_FOREACH(DiveCalculatedTissue * tissue, allTissues) {
		tissue->setVisible(false);
	}
	Q_FOREACH(DiveEventItem * event, eventItems) {
		event->setVisible(false);
	}
}

void ProfileWidget2::setProfileState()
{
	// Then starting Empty State, move the background up.
	if (currentState == PROFILE)
		return;

	currentState = PROFILE;
	MainWindow::instance()->setToolButtonsEnabled(true);
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
	if (prefs.pp_graphs.phe || prefs.pp_graphs.po2 || prefs.pp_graphs.pn2) {
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

	gasYAxis->setPos(itemPos.partialPressure.pos.on);
	gasYAxis->setLine(itemPos.partialPressure.expanded);

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
		Q_FOREACH(DiveCalculatedTissue * tissue, allTissues) {
			tissue->setVisible(true);
		}
	}
	QSettings s;
	s.beginGroup("TecDetails");
	bool rulerVisible = s.value("rulergraph", false).toBool();
	rulerItem->setVisible(rulerVisible);
	rulerItem->destNode()->setVisible(rulerVisible);
	rulerItem->sourceNode()->setVisible(rulerVisible);
}

extern struct ev_select *ev_namelist;
extern int evn_allocated;
extern int evn_used;

void ProfileWidget2::contextMenuEvent(QContextMenuEvent *event)
{
	if (selected_dive == -1)
		return;
	QMenu m;
	QMenu *gasChange = m.addMenu(tr("Add Gas Change"));
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
	QAction *action = m.addAction(tr("Add Bookmark"), this, SLOT(addBookmark()));
	action->setData(event->globalPos());
	QGraphicsItem *sceneItem = itemAt(mapFromGlobal(event->globalPos()));
	if (DiveEventItem *item = dynamic_cast<DiveEventItem *>(sceneItem)) {
		action = new QAction(&m);
		action->setText(tr("Remove Event"));
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
				if (!strcmp(event->name, ev_namelist[i].ev_name)) {
					ev_namelist[i].plot_ev = false;
					break;
				}
			}
		}
		item->hide();
		replot();
	}
}

void ProfileWidget2::unhideEvents()
{
	for (int i = 0; i < evn_used; i++) {
		ev_namelist[i].plot_ev = true;
	}
	replot();
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
		struct event **ep = &current_dc->events;
		while (ep && *ep != event)
			ep = &(*ep)->next;
		if (ep) {
			*ep = event->next;
			free(event);
		}
		mark_divelist_changed(true);
		replot();
	}
}

void ProfileWidget2::addBookmark()
{
	QAction *action = qobject_cast<QAction *>(sender());
	QPointF scenePos = mapToScene(mapFromGlobal(action->data().toPoint()));
	add_event(current_dc, timeAxis->valueAt(scenePos), SAMPLE_EVENT_BOOKMARK, 0, 0, "bookmark");
	replot();
}

void ProfileWidget2::changeGas()
{
	QAction *action = qobject_cast<QAction *>(sender());
	QPointF scenePos = mapToScene(mapFromGlobal(action->data().toPoint()));
	QString gas = action->text();
	// backup the things on the dataModel, since we will clear that out.
	unsigned int diveComputer = dataModel->dcShown();
	int diveId = dataModel->id();
	int o2, he;
	int seconds = timeAxis->valueAt(scenePos);
	struct dive *d = getDiveById(diveId);

	validate_gas(gas.toUtf8().constData(), &o2, &he);
	add_gas_switch_event(d, get_dive_dc(d, diveComputer), seconds, get_gasidx(d, o2, he));
	// this means we potentially have a new tank that is being used and needs to be shown
	fixup_dive(d);
	MainWindow::instance()->information()->updateDiveInfo(selected_dive);
	mark_divelist_changed(true);
	replot();
}

void ProfileWidget2::setPrintMode(bool mode, bool grayscale)
{
	printMode = mode;
	isGrayscale = mode ? grayscale : false;
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
			lengthWarning.setText("Name is too long!");
			lengthWarning.exec();
			return;
		}
		strcpy(event->name, newName.toUtf8());
		remember_event(event->name);
	}
	replot();
}
