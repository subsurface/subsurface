#ifndef PROFILEWIDGET2_H
#define PROFILEWIDGET2_H

#include <QGraphicsView>

// /* The idea of this widget is to display and edit the profile.
//  * It has:
//  * 1 - ToolTip / Legend item, displays every information of the current mouse position on it, plus the legends of the maps.
//  * 2 - ToolBox, displays the QActions that are used to do special stuff on the profile ( like activating the plugins. )
//  * 3 - Cartesian Axis for depth ( y )
//  * 4 - Cartesian Axis for Gases ( y )
//  * 5 - Cartesian Axis for Time  ( x )
//  *
//  * It needs to be dynamic, things should *flow* on it, not just appear / disappear.
//  */
#include "graphicsview-common.h"

class TemperatureAxis;
class DiveEventItem;
struct DivePlotDataModel;
struct DivePixmapItem;
struct DiveRectItem;
struct DepthAxis;
struct DiveCartesianAxis;
struct DiveProfileItem;
struct TimeAxis;
struct dive;
struct QStateMachine;
struct DiveCartesianPlane;
struct DiveTemperatureItem;
struct plot_info;

class ProfileWidget2 : public QGraphicsView {
	Q_OBJECT
    void fixBackgroundPos();
public:
	enum State{ EMPTY, PROFILE, EDIT, ADD, PLAN, INVALID };
	enum Items{BACKGROUND, PROFILE_Y_AXIS, GAS_Y_AXIS, TIME_AXIS, DEPTH_CONTROLLER, TIME_CONTROLLER, COLUMNS};

	ProfileWidget2(QWidget *parent);
	void plotDives(QList<dive*> dives);

public slots: // Necessary to call from QAction's signals.
	void settingsChanged();
protected:
	virtual void contextMenuEvent(QContextMenuEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

signals:
	void startProfileState();
	void startAddState();
	void startPlanState();
	void startEmptyState();
	void startEditState();
	void startHideGasState();
	void startShowGasState();
	void startShowTissueState();
	void startHideTissueState();
private:
	DivePlotDataModel *dataModel;
	State currentState;
	QStateMachine *stateMachine;

	DivePixmapItem *background ;
	// All those here should probably be merged into one structure,
	// So it's esyer to replicate for more dives later.
	// In the meantime, keep it here.
	struct plot_info *plotInfo;
	DepthAxis *profileYAxis ;
	DiveCartesianAxis *gasYAxis;
	TemperatureAxis *temperatureAxis;
	TimeAxis *timeAxis;
	DiveRectItem *depthController;
	DiveRectItem *timeController;
	DiveProfileItem *diveProfileItem;
	DiveCartesianPlane *cartesianPlane;
	DiveTemperatureItem *temperatureItem;
	QList<DiveEventItem*> eventItems;
};

#endif