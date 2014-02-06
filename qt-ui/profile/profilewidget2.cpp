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
#include <QStateMachine>
#include <QSignalTransition>
#include <QPropertyAnimation>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDebug>
#include <QScrollBar>

#ifndef QT_NO_DEBUG
#include <QTableView>
#endif
#include "mainwindow.h"

ProfileWidget2::ProfileWidget2(QWidget *parent) :
	QGraphicsView(parent),
	dataModel(new DivePlotDataModel(this)),
	currentState(INVALID),
	zoomLevel(0),
	stateMachine(new QStateMachine(this)),
	background (new DivePixmapItem()),
	toolTipItem(new ToolTipItem()),
	profileYAxis(new DepthAxis()),
	gasYAxis(new PartialGasPressureAxis()),
	temperatureAxis(new TemperatureAxis()),
	timeAxis(new TimeAxis()),
	depthController(new DiveRectItem()),
	timeController(new DiveRectItem()),
	diveProfileItem(NULL),
	cylinderPressureAxis(new DiveCartesianAxis()),
	temperatureItem(NULL),
	gasPressureItem(NULL),
	cartesianPlane(new DiveCartesianPlane()),
	meanDepth(new MeanDepthLine()),
	diveComputerText(new DiveTextItem()),
	diveCeiling(NULL),
	reportedCeiling(NULL)
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

	toolTipItem->setTimeAxis(timeAxis);
	scene()->addItem(toolTipItem);

	// Creating the needed items.
	// ORDER: {BACKGROUND, PROFILE_Y_AXIS, GAS_Y_AXIS, TIME_AXIS, DEPTH_CONTROLLER, TIME_CONTROLLER, COLUMNS};
	profileYAxis->setOrientation(DiveCartesianAxis::TopToBottom);
	timeAxis->setOrientation(DiveCartesianAxis::LeftToRight);

	// Defaults of the Axis Coordinates:
	profileYAxis->setMinimum(0);
	profileYAxis->setTickInterval(M_OR_FT(10,30)); //TODO: This one should be also hooked up on the Settings change.
	timeAxis->setMinimum(0);
	timeAxis->setTickInterval(600); // 10 to 10 minutes?

	// Default Sizes of the Items.
	profileYAxis->setX(2);
	profileYAxis->setTickSize(1);

	gasYAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	gasYAxis->setX(3);
	gasYAxis->setLine(0, 0, 0, 20);
	gasYAxis->setTickInterval(1);
	gasYAxis->setTickSize(2);
	gasYAxis->setY(70);
	gasYAxis->setMinimum(0);
	gasYAxis->setModel(dataModel);
	scene()->addItem(gasYAxis);

	temperatureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	temperatureAxis->setLine(0, 60, 0, 90);
	temperatureAxis->setX(3);
	temperatureAxis->setTickSize(2);
	temperatureAxis->setTickInterval(300);

	cylinderPressureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	cylinderPressureAxis->setLine(0,20,0,60);
	cylinderPressureAxis->setX(3);
	cylinderPressureAxis->setTickSize(2);
	cylinderPressureAxis->setTickInterval(30000);

	timeAxis->setLine(0,0,96,0);
	timeAxis->setX(3);
	timeAxis->setTickSize(1);

	depthController->setRect(0, 0, 10, 5);
	timeController->setRect(0, 0, 10, 5);
	timeController->setX(sceneRect().width() - timeController->boundingRect().width()); // Position it on the right spot.
	meanDepth->setLine(0,0,96,0);
	meanDepth->setX(3);
	meanDepth->setPen(QPen(QBrush(Qt::red), 0, Qt::SolidLine));
	meanDepth->setZValue(1);

	cartesianPlane->setBottomAxis(timeAxis);
	cartesianPlane->setLeftAxis(profileYAxis);
	scene()->addItem(cartesianPlane);

	diveComputerText->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	diveComputerText->setBrush(getColor(TIME_TEXT));

	// insert in the same way it's declared on the Enum. This is needed so we don't use an map.
	QList<QGraphicsItem*> stateItems; stateItems << background << profileYAxis <<
							timeAxis << depthController << timeController <<
							temperatureAxis << cylinderPressureAxis << diveComputerText <<
							meanDepth;
	Q_FOREACH(QGraphicsItem *item, stateItems) {
		scene()->addItem(item);
	}

	reportedCeiling = new DiveReportedCeiling();
	reportedCeiling->setHorizontalAxis(timeAxis);
	reportedCeiling->setVerticalAxis(profileYAxis);
	reportedCeiling->setModel(dataModel);
	reportedCeiling->setVerticalDataColumn(DivePlotDataModel::CEILING);
	reportedCeiling->setHorizontalDataColumn(DivePlotDataModel::TIME);
	reportedCeiling->setZValue(1);
	scene()->addItem(reportedCeiling);

	diveCeiling = new DiveCalculatedCeiling();
	diveCeiling->setHorizontalAxis(timeAxis);
	diveCeiling->setVerticalAxis(profileYAxis);
	diveCeiling->setModel(dataModel);
	diveCeiling->setVerticalDataColumn(DivePlotDataModel::CEILING);
	diveCeiling->setHorizontalDataColumn(DivePlotDataModel::TIME);
	diveCeiling->setZValue(1);
	scene()->addItem(diveCeiling);

	for(int i = 0; i < 16; i++){
		DiveCalculatedTissue *tissueItem = new DiveCalculatedTissue();
		tissueItem->setHorizontalAxis(timeAxis);
		tissueItem->setVerticalAxis(profileYAxis);
		tissueItem->setModel(dataModel);
		tissueItem->setVerticalDataColumn(DivePlotDataModel::TISSUE_1 + i);
		tissueItem->setHorizontalDataColumn(DivePlotDataModel::TIME);
		tissueItem->setZValue(1);
		allTissues.append(tissueItem);
		scene()->addItem(tissueItem);
	}

	gasPressureItem = new DiveGasPressureItem();
	gasPressureItem->setHorizontalAxis(timeAxis);
	gasPressureItem->setVerticalAxis(cylinderPressureAxis);
	gasPressureItem->setModel(dataModel);
	gasPressureItem->setVerticalDataColumn(DivePlotDataModel::TEMPERATURE);
	gasPressureItem->setHorizontalDataColumn(DivePlotDataModel::TIME);
	gasPressureItem->setZValue(1);
	scene()->addItem(gasPressureItem);

	temperatureItem = new DiveTemperatureItem();
	temperatureItem->setHorizontalAxis(timeAxis);
	temperatureItem->setVerticalAxis(temperatureAxis);
	temperatureItem->setModel(dataModel);
	temperatureItem->setVerticalDataColumn(DivePlotDataModel::TEMPERATURE);
	temperatureItem->setHorizontalDataColumn(DivePlotDataModel::TIME);
	temperatureItem->setZValue(1);
	scene()->addItem(temperatureItem);

	diveProfileItem = new DiveProfileItem();
	diveProfileItem->setHorizontalAxis(timeAxis);
	diveProfileItem->setVerticalAxis(profileYAxis);
	diveProfileItem->setModel(dataModel);
	diveProfileItem->setVerticalDataColumn(DivePlotDataModel::DEPTH);
	diveProfileItem->setHorizontalDataColumn(DivePlotDataModel::TIME);
	diveProfileItem->setZValue(0);
	scene()->addItem(diveProfileItem);

#define CREATE_PP_GAS( ITEM, VERTICAL_COLUMN, COLOR, COLOR_ALERT, THRESHOULD_SETTINGS, VISIBILITY_SETTINGS ) \
	ITEM = new PartialPressureGasItem(); \
	ITEM->setHorizontalAxis(timeAxis); \
	ITEM->setVerticalAxis(gasYAxis); \
	ITEM->setModel(dataModel); \
	ITEM->setVerticalDataColumn(DivePlotDataModel::VERTICAL_COLUMN); \
	ITEM->setHorizontalDataColumn(DivePlotDataModel::TIME); \
	ITEM->setZValue(0); \
	ITEM->setThreshouldSettingsKey(THRESHOULD_SETTINGS); \
	ITEM->setVisibilitySettingsKey(VISIBILITY_SETTINGS); \
	ITEM->setColors(getColor(COLOR), getColor(COLOR_ALERT)); \
	ITEM->preferencesChanged(); \
	scene()->addItem(ITEM);

	CREATE_PP_GAS( pn2GasItem, PN2, PN2, PN2_ALERT, "pn2threshold", "pn2graph");
	CREATE_PP_GAS( pheGasItem, PHE, PHE, PHE_ALERT, "phethreshold", "phegraph");
	CREATE_PP_GAS( po2GasItem, PO2, PO2, PO2_ALERT, "po2threshold", "po2graph");
#undef CREATE_PP_GAS

	background->setFlag(QGraphicsItem::ItemIgnoresTransformations);

	//enum State{ EMPTY, PROFILE, EDIT, ADD, PLAN, INVALID };
	stateMachine = new QStateMachine(this);

	// TopLevel States
	QState *emptyState = new QState();
	QState *profileState = new QState();

	// Conections:
	stateMachine->addState(emptyState);
	stateMachine->addState(profileState);
	stateMachine->setInitialState(emptyState);

	// All Empty State Connections.
	QSignalTransition *tEmptyToProfile = emptyState->addTransition(this, SIGNAL(startProfileState()), profileState);

	// All Profile State Connections
	QSignalTransition *tProfileToEmpty = profileState->addTransition(this, SIGNAL(startEmptyState()), emptyState);

	// Constants:
	const int backgroundOnCanvas = 0;
	const int backgroundOffCanvas = 110;
	const int profileYAxisOnCanvas = 3;
	const int profileYAxisOffCanvas = profileYAxis->boundingRect().width() - 10;
	// unused so far:
	const int gasYAxisOnCanvas = gasYAxis->boundingRect().width();
	const int depthControllerOnCanvas = sceneRect().height() - depthController->boundingRect().height();
	const int timeControllerOnCanvas = sceneRect().height() - timeController->boundingRect().height();
	const int gasYAxisOffCanvas = gasYAxis->boundingRect().width() - 10;
	const int timeAxisOnCanvas = sceneRect().height() - timeAxis->boundingRect().height() - 4;
	const int timeAxisOffCanvas = sceneRect().height() + timeAxis->boundingRect().height();
	const int timeAxisEditMode = sceneRect().height() - timeAxis->boundingRect().height() - depthController->boundingRect().height();
	const int depthControllerOffCanvas = sceneRect().height() + depthController->boundingRect().height();
	const int timeControllerOffCanvas = sceneRect().height() + timeController->boundingRect().height();
	const QLineF profileYAxisExpanded = QLineF(0,0,0,timeAxisOnCanvas);
	// unused so far:
	// const QLineF timeAxisLine = QLineF(0, 0, 96, 0);

	// State Defaults:
	// Empty State, everything but the background is hidden.
	emptyState->assignProperty(this, "backgroundBrush", QBrush(Qt::white));
	emptyState->assignProperty(background, "y",  backgroundOnCanvas);
	emptyState->assignProperty(profileYAxis, "x", profileYAxisOffCanvas);
	emptyState->assignProperty(gasYAxis, "x", gasYAxisOffCanvas);
	emptyState->assignProperty(timeAxis, "y", timeAxisOffCanvas);
	emptyState->assignProperty(depthController, "y", depthControllerOffCanvas);
	emptyState->assignProperty(timeController, "y", timeControllerOffCanvas);

	// Profile, everything but the background, depthController and timeController are shown.
	profileState->assignProperty(this, "backgroundBrush", getColor(::BACKGROUND));
	profileState->assignProperty(background, "y",  backgroundOffCanvas);
	profileState->assignProperty(profileYAxis, "x", profileYAxisOnCanvas);
	profileState->assignProperty(profileYAxis, "line", profileYAxisExpanded);
	profileState->assignProperty(gasYAxis, "x", profileYAxisOnCanvas);
	profileState->assignProperty(timeAxis, "y", timeAxisOnCanvas);
	profileState->assignProperty(depthController, "y", depthControllerOffCanvas);
	profileState->assignProperty(timeController, "y", timeControllerOffCanvas);
	profileState->assignProperty(cartesianPlane, "verticalLine", profileYAxisExpanded);
	profileState->assignProperty(cartesianPlane, "horizontalLine", timeAxis->line());

	// All animations for the State Transitions.
	QPropertyAnimation *backgroundYAnim = new QPropertyAnimation(background, "y");
	QPropertyAnimation *depthAxisAnim = new QPropertyAnimation(profileYAxis, "x");
	QPropertyAnimation *gasAxisanim = new QPropertyAnimation(gasYAxis, "x");
	QPropertyAnimation *timeAxisAnim = new QPropertyAnimation(timeAxis, "y");
	QPropertyAnimation *depthControlAnim = new QPropertyAnimation(depthController, "y");
	QPropertyAnimation *timeControlAnim = new QPropertyAnimation(timeController, "y");
	QPropertyAnimation *profileAxisAnim = new QPropertyAnimation(profileYAxis, "line");

// Animations
	QList<QSignalTransition*> transitions;
	transitions
		<< tEmptyToProfile
		<< tProfileToEmpty;

	Q_FOREACH(QSignalTransition *s, transitions) {
		s->addAnimation(backgroundYAnim);
		s->addAnimation(depthAxisAnim);
		s->addAnimation(gasAxisanim);
		s->addAnimation(timeAxisAnim);
		s->addAnimation(depthControlAnim);
		s->addAnimation(timeControlAnim);
		s->addAnimation(profileAxisAnim);
	}

		// Configuration so we can search for the States later, and it helps debugging.
	emptyState->setObjectName("Empty State");
	profileState->setObjectName("Profile State");

	// Starting the transitions:
	stateMachine->start();
	scene()->installEventFilter(this);
#ifndef QT_NO_DEBUG
	QTableView *diveDepthTableView = new QTableView();
	diveDepthTableView->setModel(dataModel);
	mainWindow()->tabWidget()->addTab(diveDepthTableView, "Depth Model");
#endif
}

// Currently just one dive, but the plan is to enable All of the selected dives.
void ProfileWidget2::plotDives(QList<dive*> dives)
{
	// I Know that it's a list, but currently we are
	// using just the first.
	struct dive *d = dives.first();
	if (!d)
		return;

		emit startProfileState();

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

	dataModel->setDive(current_dive, pInfo);
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
	diveComputerText->animateMoveTo(1 , sceneRect().height());
}

void ProfileWidget2::settingsChanged()
{

}

void ProfileWidget2::contextMenuEvent(QContextMenuEvent* event)
{
	// this menu should be completely replaced when things are working.
	QMenu m;
	m.addAction("Set Empty", this, SIGNAL(startEmptyState()));
	m.addAction("Set Profile", this, SIGNAL(startProfileState()));
	m.exec(event->globalPos());
}

void ProfileWidget2::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);

	if (!stateMachine->configuration().count())
		return;

	if ((*stateMachine->configuration().begin())->objectName() == "Empty State") {
		fixBackgroundPos();
	}
}

void ProfileWidget2::fixBackgroundPos()
{
	QPixmap toBeScaled(":background");
	if (toBeScaled.isNull())
		return;
	QPixmap p = toBeScaled.scaledToHeight(viewport()->height());

	int x = viewport()->width() / 2 - p.width() / 2;
	DivePixmapItem *bg = background;
	bg->setPixmap(p);
	bg->setX(mapToScene(x, 0).x());
}

void ProfileWidget2::wheelEvent(QWheelEvent* event)
{
	// doesn't seem to work for Qt 4.8.1
	// setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

	// Scale the view / do the zoom
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
		toolTipItem->setPos(mapToScene(toolTipPos));
		scrollViewTo(event->pos());
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

