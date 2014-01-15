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

}

void ProfileWidget2::showEvent(QShowEvent* event)
{

}