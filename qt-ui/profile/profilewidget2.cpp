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
#include <QSignalTransition>
#include <QPropertyAnimation>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDebug>
#include <QSettings>
#include <QScrollBar>

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
static struct _ItemPos{
	struct _Pos{
		QPointF on;
		QPointF off;
	};
	struct _Axis{
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
} itemPos;

ProfileWidget2::ProfileWidget2(QWidget *parent) :
	QGraphicsView(parent),
	dataModel(new DivePlotDataModel(this)),
	currentState(INVALID),
	zoomLevel(0),
	background (new DivePixmapItem()),
	toolTipItem(new ToolTipItem()),
	profileYAxis(new DepthAxis()),
	gasYAxis(new PartialGasPressureAxis()),
	temperatureAxis(new TemperatureAxis()),
	timeAxis(new TimeAxis()),
	diveProfileItem(new DiveProfileItem()),
	cartesianPlane(new DiveCartesianPlane()),
	temperatureItem(new DiveTemperatureItem()),
	cylinderPressureAxis(new DiveCartesianAxis()),
	gasPressureItem(new DiveGasPressureItem()),
	meanDepth(new MeanDepthLine()),
	diveComputerText(new DiveTextItem()),
	diveCeiling(new DiveCalculatedCeiling()),
	reportedCeiling(new DiveReportedCeiling()),
	pn2GasItem( new PartialPressureGasItem()),
	pheGasItem( new PartialPressureGasItem()),
	po2GasItem( new PartialPressureGasItem())
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
	mainWindow()->tabWidget()->addTab(diveDepthTableView, "Depth Model");
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
	scene()->addItem(cartesianPlane);
	scene()->addItem(meanDepth);
	scene()->addItem(diveComputerText);
	scene()->addItem(diveCeiling);
	scene()->addItem(reportedCeiling);
	scene()->addItem(pn2GasItem);
	scene()->addItem(pheGasItem);
	scene()->addItem(po2GasItem);
	Q_FOREACH(DiveCalculatedTissue *tissue, allTissues){
		scene()->addItem(tissue);
	}
}

void ProfileWidget2::setupItemOnScene()
{
	background->setZValue(9999);
	toolTipItem->setTimeAxis(timeAxis);

	profileYAxis->setOrientation(DiveCartesianAxis::TopToBottom);
	profileYAxis->setMinimum(0);
	profileYAxis->setTickInterval(M_OR_FT(10,30));
	profileYAxis->setTickSize(1);

	gasYAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	gasYAxis->setTickInterval(1);
	gasYAxis->setTickSize(1);
	gasYAxis->setMinimum(0);
	gasYAxis->setModel(dataModel);

	temperatureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	temperatureAxis->setTickSize(2);
	temperatureAxis->setTickInterval(300);

	cylinderPressureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	cylinderPressureAxis->setTickSize(2);
	cylinderPressureAxis->setTickInterval(30000);

	meanDepth->setLine(0,0,96,0);
	meanDepth->setX(3);
	meanDepth->setPen(QPen(QBrush(Qt::red), 0, Qt::SolidLine));
	meanDepth->setZValue(1);

	cartesianPlane->setBottomAxis(timeAxis);
	cartesianPlane->setLeftAxis(profileYAxis);
	cartesianPlane->setZValue(-1);

	diveComputerText->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	diveComputerText->setBrush(getColor(TIME_TEXT));

	setupItem(reportedCeiling, timeAxis, profileYAxis, dataModel, DivePlotDataModel::CEILING, DivePlotDataModel::TIME, 1);
	setupItem(diveCeiling, timeAxis, profileYAxis, dataModel, DivePlotDataModel::CEILING, DivePlotDataModel::TIME, 1);
	for(int i = 0; i < 16; i++){
		DiveCalculatedTissue *tissueItem = new DiveCalculatedTissue();
		setupItem(tissueItem, timeAxis, profileYAxis, dataModel, DivePlotDataModel::TISSUE_1 + i, DivePlotDataModel::TIME, 1+i);
		allTissues.append(tissueItem);
	}
	setupItem(gasPressureItem, timeAxis, cylinderPressureAxis, dataModel, DivePlotDataModel::TEMPERATURE, DivePlotDataModel::TIME, 1);
	setupItem(temperatureItem, timeAxis, temperatureAxis, dataModel, DivePlotDataModel::TEMPERATURE, DivePlotDataModel::TIME, 1);
	setupItem(diveProfileItem, timeAxis, profileYAxis, dataModel, DivePlotDataModel::DEPTH, DivePlotDataModel::TIME, 0);

#define CREATE_PP_GAS( ITEM, VERTICAL_COLUMN, COLOR, COLOR_ALERT, THRESHOULD_SETTINGS, VISIBILITY_SETTINGS ) \
	setupItem(ITEM, timeAxis, gasYAxis, dataModel, DivePlotDataModel::VERTICAL_COLUMN, DivePlotDataModel::TIME, 0); \
	ITEM->setThreshouldSettingsKey(THRESHOULD_SETTINGS); \
	ITEM->setVisibilitySettingsKey(VISIBILITY_SETTINGS); \
	ITEM->setColors(getColor(COLOR), getColor(COLOR_ALERT)); \
	ITEM->preferencesChanged();

	CREATE_PP_GAS( pn2GasItem, PN2, PN2, PN2_ALERT, "pn2threshold", "pn2graph");
	CREATE_PP_GAS( pheGasItem, PHE, PHE, PHE_ALERT, "phethreshold", "phegraph");
	CREATE_PP_GAS( po2GasItem, PO2, PO2, PO2_ALERT, "po2threshold", "po2graph");
#undef CREATE_PP_GAS

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
	itemPos.depth.expanded.setP1(QPointF(0,0));
	itemPos.depth.expanded.setP2(QPointF(0,94));
	itemPos.depth.shrinked.setP1(QPointF(0,0));
	itemPos.depth.shrinked.setP2(QPointF(0,60));

	// Time Axis Config
	itemPos.time.pos.on.setX(3);
	itemPos.time.pos.on.setY(97);
	itemPos.time.pos.off.setX(3);
	itemPos.time.pos.off.setY(110);
	itemPos.time.expanded.setP1(QPointF(0,0));
	itemPos.time.expanded.setP2(QPointF(94,0));

	// Partial Gas Axis Config
	itemPos.partialPressure.pos.on.setX(97);
	itemPos.partialPressure.pos.on.setY(60);
	itemPos.partialPressure.pos.off.setX(110);
	itemPos.partialPressure.pos.off.setY(60);
	itemPos.partialPressure.expanded.setP1(QPointF(0,0));
	itemPos.partialPressure.expanded.setP2(QPointF(0,30));

	// cylinder axis config
	itemPos.cylinder.pos.on.setX(3);
	itemPos.cylinder.pos.on.setY(20);
	itemPos.cylinder.pos.off.setX(-10);
	itemPos.cylinder.pos.off.setY(20);
	itemPos.cylinder.expanded.setP1(QPointF(0,0));
	itemPos.cylinder.expanded.setP2(QPointF(0,20));

		// Temperature axis config
	itemPos.temperature.pos.on.setX(3);
	itemPos.temperature.pos.on.setY(40);
	itemPos.temperature.pos.off.setX(-10);
	itemPos.temperature.pos.off.setY(40);
	itemPos.temperature.expanded.setP1(QPointF(0,0));
	itemPos.temperature.expanded.setP2(QPointF(0,20));

	itemPos.dcLabel.on.setX(3);
	itemPos.dcLabel.on.setY(97);
	itemPos.dcLabel.off.setX(-10);
	itemPos.dcLabel.off.setY(97);
}

void ProfileWidget2::setupItem(AbstractProfilePolygonItem* item, DiveCartesianAxis* hAxis, DiveCartesianAxis* vAxis, DivePlotDataModel* model, int vData, int hData, int zValue)
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
void ProfileWidget2::plotDives(QList<dive*> dives)
{
	// I Know that it's a list, but currently we are
	// using just the first.
	struct dive *d = dives.first();
	if (!d)
		return;

	// No need to do this again if we are already showing the same dive
	// computer of the same dive, so we check the unique id of the dive
	// and the selected dive computer number against the ones we are
	// showing (can't compare the dive pointers as those might change).
	// I'm unclear what the semantics are supposed to be if we actually
	// use more than one 'dives' as argument - so ignoring that right now :-)
	if (d->id == dataModel->id() && dc_number == dataModel->dcShown())
		return;

	setProfileState();
	// Here we need to probe for the limits of the dive.
	// There's already a function that does exactly that,
	// but it's using the graphics context, and I need to
	// replace that.
	struct divecomputer *currentdc = select_dc(&d->dc);
	Q_ASSERT(currentdc);

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
	timeAxis->setMaximum(maxtime);

	int i, incr;
	static int increments[8] = { 10, 20, 30, 60, 5*60, 10*60, 15*60, 30*60 };
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
	meanDepth->setMeanDepth(pInfo.meandepth);
	meanDepth->animateMoveTo(3, profileYAxis->posAtValue(pInfo.meandepth));

	dataModel->emitDataChanged();
	cartesianPlane->setup();
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

	diveComputerText->setText(currentdc->model);
}

void ProfileWidget2::settingsChanged()
{
	QSettings s;
	s.beginGroup("TecDetails");
	if(s.value("phegraph").toBool()|| s.value("po2graph").toBool()|| s.value("pn2graph").toBool()){
		profileYAxis->animateChangeLine(itemPos.depth.shrinked);
	}else{
		profileYAxis->animateChangeLine(itemPos.depth.expanded);
	}
}

void ProfileWidget2::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);
	fixBackgroundPos();
}

void ProfileWidget2::fixBackgroundPos()
{
	if(currentState != EMPTY)
		return;
	QPixmap toBeScaled;
	if (!backgrounds.keys().contains(backgroundFile)){
		backgrounds[backgroundFile] = QPixmap(backgroundFile);
	}
	toBeScaled = backgrounds[backgroundFile];
	QPixmap p = toBeScaled.scaledToHeight(viewport()->height());
	int x = viewport()->width() / 2 - p.width() / 2;
	background->setPixmap(p);
	background->setX(mapToScene(x, 0).x());
}

void ProfileWidget2::wheelEvent(QWheelEvent* event)
{
	QPoint toolTipPos = mapFromScene(toolTipItem->pos());
	double scaleFactor = 1.15;
	if (event->delta() > 0 && zoomLevel < 20) {
		scale(scaleFactor, scaleFactor);
		zoomLevel++;
	} else if (event->delta() < 0 && zoomLevel > 0) {
		// Zooming out
		scale(1.0 / scaleFactor, 1.0 / scaleFactor);
		zoomLevel--;
	}
	scrollViewTo(event->pos());
	toolTipItem->setPos(mapToScene(toolTipPos));
}

void ProfileWidget2::scrollViewTo(const QPoint& pos)
{
/* since we cannot use translate() directly on the scene we hack on
 * the scroll bars (hidden) functionality */
	if (!zoomLevel)
		return;
	QScrollBar *vs = verticalScrollBar();
	QScrollBar *hs = horizontalScrollBar();
	const qreal yRat = (qreal)pos.y() / viewport()->height();
	const qreal xRat = (qreal)pos.x() / viewport()->width();
	vs->setValue(yRat * vs->maximum());
	hs->setValue(xRat * hs->maximum());
}

void ProfileWidget2::mouseMoveEvent(QMouseEvent* event)
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
	QGraphicsScene *s = qobject_cast<QGraphicsScene*>(object);
	if (s && event->type() == QEvent::GraphicsSceneHelp){
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
	backgroundFile = QString(":poster%1").arg( rand()%3 +1);
	currentState = EMPTY;
	fixBackgroundPos();
	profileYAxis->setPos(itemPos.depth.pos.off);
	gasYAxis->setPos(itemPos.partialPressure.pos.off);
	timeAxis->setPos(itemPos.time.pos.off);
	background->setY( itemPos.background.on.y());
	background->setVisible(true);
	toolTipItem->setVisible(false);
	temperatureAxis->setPos(itemPos.temperature.pos.off);
	cylinderPressureAxis->setPos(itemPos.cylinder.pos.off);
	meanDepth->setVisible(false);
	diveComputerText->setVisible(false);
	diveCeiling->setVisible(false);
	reportedCeiling->setVisible(false);
	cartesianPlane->setVisible(false);
	Q_FOREACH(DiveCalculatedTissue *tissue, allTissues){
		tissue->setVisible(false);
	}
	Q_FOREACH(DiveEventItem *event, eventItems){
		event->setVisible(false);
	}
}

void ProfileWidget2::setProfileState()
{
	// Then starting Empty State, move the background up.
	if (currentState == PROFILE)
		return;

	currentState = PROFILE;
	setBackgroundBrush(getColor(::BACKGROUND));

	background->setVisible(false);
	toolTipItem->setVisible(true);

	profileYAxis->setPos(itemPos.depth.pos.on);
	QSettings s;
	s.beginGroup("TecDetails");
	if(s.value("phegraph").toBool()|| s.value("po2graph").toBool()|| s.value("pn2graph").toBool()){
		profileYAxis->setLine(itemPos.depth.shrinked);
	}else{
		profileYAxis->setLine(itemPos.depth.expanded);
	}

	gasYAxis->setPos(itemPos.partialPressure.pos.on);
	gasYAxis->setLine(itemPos.partialPressure.expanded);

	timeAxis->setPos(itemPos.time.pos.on);
	timeAxis->setLine(itemPos.time.expanded);

	cylinderPressureAxis->setPos(itemPos.cylinder.pos.on);
	cylinderPressureAxis->setLine(itemPos.cylinder.expanded);

	temperatureAxis->setPos(itemPos.temperature.pos.on);
	temperatureAxis->setLine(itemPos.temperature.expanded);

	cartesianPlane->setVisible(true);
	meanDepth->setVisible(true);

	diveComputerText->setVisible(true);
	diveComputerText->setPos(itemPos.dcLabel.on);

	cartesianPlane->setHorizontalLine( itemPos.time.expanded );
	cartesianPlane->setVerticalLine( itemPos.depth.expanded );

	diveCeiling->setVisible(s.value("calcceiling").toBool());
	reportedCeiling->setVisible(s.value("dcceiling").toBool());

	if(s.value("calcalltissues").toBool()){
		Q_FOREACH(DiveCalculatedTissue *tissue, allTissues){
			tissue->setVisible(true);
		}
	}
	// Only set visible the ones that should be visible, but how?
	Q_FOREACH(DiveEventItem *event, eventItems){
		event->setVisible(true);
	}
}
