#ifndef PROFILEWIDGET2_H
#define PROFILEWIDGET2_H

#include <QGraphicsView>
#include <QStateMachine>

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

struct dive;

class ProfileWidget2 : public QGraphicsView {
	Q_OBJECT
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
	virtual void showEvent(QShowEvent* event);

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
};

#endif