#include "profilewidget2.h"
#include "diveplotdatamodel.h"
#include "divepixmapitem.h"
#include "diverectitem.h"
#include "divecartesianaxis.h"
#include "diveprofileitem.h"
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