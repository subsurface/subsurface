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
#include "divelineitem.h"

class ToolTipItem;
class MeanDepthLine;
class DiveReportedCeiling;
class DiveTextItem;
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
struct DiveGasPressureItem;
struct DiveCalculatedCeiling;
struct DiveReportedCeiling;
struct DiveCalculatedTissue;
struct PartialPressureGasItem;
struct PartialGasPressureAxis;
struct AbstractProfilePolygonItem;

class ProfileWidget2 : public QGraphicsView {
	Q_OBJECT
	void fixBackgroundPos();
	void scrollViewTo(const QPoint& pos);
    void setupSceneAndFlags();
    void setupItemSizes();
    void addItemsToScene();
    void setupItemOnScene();
public:
	enum State{ EMPTY, PROFILE, EDIT, ADD, PLAN, INVALID };
	enum Items{BACKGROUND, PROFILE_Y_AXIS, GAS_Y_AXIS, TIME_AXIS, DEPTH_CONTROLLER, TIME_CONTROLLER, COLUMNS};

	ProfileWidget2(QWidget *parent);
	void plotDives(QList<dive*> dives);
	virtual bool eventFilter(QObject*, QEvent*);
	void setupItem( AbstractProfilePolygonItem *item, DiveCartesianAxis *hAxis, DiveCartesianAxis *vAxis, DivePlotDataModel *model, int vData, int hData, int zValue);

public slots: // Necessary to call from QAction's signals.
	void settingsChanged();
protected:
	virtual void resizeEvent(QResizeEvent* event);
	virtual void wheelEvent(QWheelEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);

signals:
	void startProfileState();
	void startEmptyState();
private:
	DivePlotDataModel *dataModel;
	State currentState;
	int zoomLevel;
	DivePixmapItem *background ;
	ToolTipItem *toolTipItem;
	// All those here should probably be merged into one structure,
	// So it's esyer to replicate for more dives later.
	// In the meantime, keep it here.
	struct plot_info *plotInfo;
	DepthAxis *profileYAxis ;
	PartialGasPressureAxis *gasYAxis;
	TemperatureAxis *temperatureAxis;
	TimeAxis *timeAxis;
	DiveRectItem *depthController;
	DiveRectItem *timeController;
	DiveProfileItem *diveProfileItem;
	DiveCartesianPlane *cartesianPlane;
	DiveTemperatureItem *temperatureItem;
	DiveCartesianAxis *cylinderPressureAxis;
	DiveGasPressureItem *gasPressureItem;
	MeanDepthLine *meanDepth;
	QList<DiveEventItem*> eventItems;
	DiveTextItem *diveComputerText;
	DiveCalculatedCeiling *diveCeiling;
	QList<DiveCalculatedTissue*> allTissues;
	DiveReportedCeiling *reportedCeiling;
	PartialPressureGasItem *pn2GasItem;
	PartialPressureGasItem *pheGasItem;
	PartialPressureGasItem *po2GasItem;
};

#endif
