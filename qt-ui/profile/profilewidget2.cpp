#include "profilewidget2.h"
#include "diveplotdatamodel.h"
#include "divepixmapitem.h"
#include "diverectitem.h"
#include "divecartesianaxis.h"
#include "diveprofileitem.h"
#include "helpers.h"
#include <QStateMachine>

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