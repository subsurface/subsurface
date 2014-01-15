#include "profilewidget2.h"
#include "diveplotdatamodel.h"
#include "divepixmapitem.h"
#include "diverectitem.h"
#include "divecartesianaxis.h"
#include "diveprofileitem.h"
#include "helpers.h"

#include <QStateMachine>
#include <QSignalTransition>

ProfileWidget2::ProfileWidget2(QWidget *parent) :
	QGraphicsView(parent),
	dataModel(new DivePlotDataModel(this)),
	currentState(INVALID),
	stateMachine(new QStateMachine(this)),
	background (new DivePixmapItem()),
	profileYAxis(new DepthAxis()),
	gasYAxis(new DiveCartesianAxis()),
	timeAxis(new TimeAxis()),
	depthController(new DiveRectItem()),
	timeController(new DiveRectItem()),
	diveProfileItem(new DiveProfileItem())
{
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0, 0, 100, 100);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scene()->setItemIndexMethod(QGraphicsScene::NoIndex);
	setOptimizationFlags(QGraphicsView::DontSavePainterState);
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	// Creating the needed items.
	// ORDER: {BACKGROUND, PROFILE_Y_AXIS, GAS_Y_AXIS, TIME_AXIS, DEPTH_CONTROLLER, TIME_CONTROLLER, COLUMNS};
	profileYAxis->setOrientation(Qt::Vertical);
	gasYAxis->setOrientation(Qt::Vertical);
	timeAxis->setOrientation(Qt::Horizontal);

		// Defaults of the Axis Coordinates:
	profileYAxis->setMinimum(0);
	profileYAxis->setTickInterval(M_OR_FT(10,30)); //TODO: This one should be also hooked up on the Settings change.
	timeAxis->setMinimum(0);
	timeAxis->setTickInterval(600); // 10 to 10 minutes?

	// Default Sizes of the Items.
	profileYAxis->setLine(0, 0, 0, 90);
	profileYAxis->setX(2);
	profileYAxis->setTickSize(1);
	gasYAxis->setLine(0, 0, 0, 20);
	timeAxis->setLine(0,0,96,0);
	timeAxis->setX(3);
	timeAxis->setTickSize(1);
	depthController->setRect(0, 0, 10, 5);
	timeController->setRect(0, 0, 10, 5);
	timeController->setX(sceneRect().width() - timeController->boundingRect().width()); // Position it on the right spot.

	// insert in the same way it's declared on the Enum. This is needed so we don't use an map.
	QList<QGraphicsItem*> stateItems; stateItems << background << profileYAxis << gasYAxis
		<< timeAxis << depthController << timeController;
	Q_FOREACH(QGraphicsItem *item, stateItems){
		scene()->addItem(item);
	}

	background->setFlag(QGraphicsItem::ItemIgnoresTransformations);

		//enum State{ EMPTY, PROFILE, EDIT, ADD, PLAN, INVALID };
	stateMachine = new QStateMachine(this);

	// TopLevel States
	QState *emptyState = new QState();
	QState *profileState = new QState();
	QState *editState = new QState();
	QState *addState = new QState();
	QState *planState = new QState();

	// Conections:
	stateMachine->addState(emptyState);
	stateMachine->addState(profileState);
	stateMachine->addState(editState);
	stateMachine->addState(addState);
	stateMachine->addState(planState);
	stateMachine->setInitialState(emptyState);

	// All Empty State Connections.
	QSignalTransition *tEmptyToProfile = emptyState->addTransition(this, SIGNAL(startProfileState()), profileState);
	QSignalTransition *tEmptyToAdd = emptyState->addTransition(this, SIGNAL(startAddState()), addState);
	QSignalTransition *tEmptyToPlan = emptyState->addTransition(this, SIGNAL(startPlanState()), planState);

	// All Plan Connections
	QSignalTransition *tPlanToEmpty = planState->addTransition(this, SIGNAL(startEmptyState()), emptyState);
	QSignalTransition *tPlanToProfile = planState->addTransition(this, SIGNAL(startProfileState()), profileState);
	QSignalTransition *tPlanToAdd = planState->addTransition(this, SIGNAL(startAddState()), addState);

	// All Add Dive Connections
	QSignalTransition *tAddToEmpty = addState->addTransition(this, SIGNAL(startEmptyState()), emptyState);
	QSignalTransition *tAddToPlan = addState->addTransition(this, SIGNAL(startPlanState()), planState);
	QSignalTransition *tAddToProfile = addState->addTransition(this, SIGNAL(startProfileState()), profileState);

	// All Profile State Connections
	QSignalTransition *tProfileToEdit = profileState->addTransition(this, SIGNAL(startEditState()), editState);
	QSignalTransition *tProfileToEmpty = profileState->addTransition(this, SIGNAL(startEmptyState()), emptyState);
	QSignalTransition *tProfileToPlan = profileState->addTransition(this, SIGNAL(startPlanState()), planState);
	QSignalTransition *tProfileToAdd = profileState->addTransition(this, SIGNAL(startAddState()), addState);

	// All "Edit" state connections
	QSignalTransition *tEditToEmpty = editState->addTransition(this, SIGNAL(startEmptyState()), emptyState);
	QSignalTransition *tEditToPlan = editState->addTransition(this, SIGNAL(startPlanState()), planState);
	QSignalTransition *tEditToProfile = editState->addTransition(this, SIGNAL(startProfileState()), profileState);
	QSignalTransition *tEditToAdd = editState->addTransition(this, SIGNAL(startAddState()), addState);

		// Constants:
	const int backgroundOnCanvas = 0;
	const int backgroundOffCanvas = 110;
	const int profileYAxisOnCanvas = 3;
	const int profileYAxisOffCanvas = profileYAxis->boundingRect().width() - 10;
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
	const QLineF timeAxisLine = QLineF(0, 0, 96, 0);

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
	profileState->assignProperty(gasYAxis, "x", 0);
	profileState->assignProperty(timeAxis, "y", timeAxisOnCanvas);
	profileState->assignProperty(depthController, "y", depthControllerOffCanvas);
	profileState->assignProperty(timeController, "y", timeControllerOffCanvas);

	// Edit, everything but the background and gasYAxis are shown.
	editState->assignProperty(this, "backgroundBrush", QBrush(Qt::darkGray));
	editState->assignProperty(background, "y",  backgroundOffCanvas);
	editState->assignProperty(profileYAxis, "x", profileYAxisOnCanvas);
	editState->assignProperty(profileYAxis, "line", profileYAxisExpanded);
	editState->assignProperty(gasYAxis, "x", gasYAxisOffCanvas);
	editState->assignProperty(timeAxis, "y", timeAxisEditMode);
	editState->assignProperty(depthController, "y", depthControllerOnCanvas);
	editState->assignProperty(timeController, "y", timeControllerOnCanvas);

	// Add, everything but the background and gasYAxis are shown.
	addState->assignProperty(this, "backgroundBrush", QBrush(Qt::darkGray));
	addState->assignProperty(background, "y",  backgroundOffCanvas);
	addState->assignProperty(profileYAxis, "x", profileYAxisOnCanvas);
	addState->assignProperty(profileYAxis, "rect", profileYAxisExpanded);
	addState->assignProperty(gasYAxis, "x", gasYAxisOffCanvas);
	addState->assignProperty(timeAxis, "y", timeAxisEditMode);
	addState->assignProperty(depthController, "y", depthControllerOnCanvas);
	addState->assignProperty(timeController, "y", timeControllerOnCanvas);

	// Plan, everything but the background and gasYAxis are shown.
	planState->assignProperty(this, "backgroundBrush", QBrush(Qt::darkGray));
	planState->assignProperty(background, "y",  backgroundOffCanvas);
	planState->assignProperty(profileYAxis, "x", profileYAxisOnCanvas);
	planState->assignProperty(profileYAxis, "line", profileYAxisExpanded);
	planState->assignProperty(gasYAxis, "x", gasYAxisOffCanvas);
	planState->assignProperty(timeAxis, "y", timeAxisEditMode);
	planState->assignProperty(depthController, "y", depthControllerOnCanvas);
	planState->assignProperty(timeController, "y", timeControllerOnCanvas);

}

// Currently just one dive, but the plan is to enable All of the selected dives.
void ProfileWidget2::plotDives(QList<dive*> dives)
{

}

void ProfileWidget2::settingsChanged()
{

}

void ProfileWidget2::contextMenuEvent(QContextMenuEvent* event)
{

}

void ProfileWidget2::resizeEvent(QResizeEvent* event)
{
	QGraphicsView::resizeEvent(event);
	fitInView(sceneRect(), Qt::IgnoreAspectRatio);

	if(!stateMachine->configuration().count())
		return;

	if ((*stateMachine->configuration().begin())->objectName() == "Empty State"){
		fixBackgroundPos();
	}
}

void ProfileWidget2::fixBackgroundPos()
{
	QPixmap p = QPixmap(":background").scaledToHeight(viewport()->height());
	int x = viewport()->width()/2 - p.width()/2;
	DivePixmapItem *bg = background;
	bg->setPixmap(p);
	bg->setX(mapToScene(x, 0).x());
}

void ProfileWidget2::showEvent(QShowEvent* event)
{

}