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
#include "diveprofileitem.h"
#include "display.h"

class RulerItem2;
struct dive;
struct plot_info;
class ToolTipItem;
class MeanDepthLine;
class DiveReportedCeiling;
class DiveTextItem;
class TemperatureAxis;
class DiveEventItem;
class DivePlotDataModel;
class DivePixmapItem;
class DiveRectItem;
class DepthAxis;
class DiveCartesianAxis;
class DiveProfileItem;
class TimeAxis;
class DiveTemperatureItem;
class DiveHeartrateItem;
class PercentageItem;
class DiveGasPressureItem;
class DiveCalculatedCeiling;
class DiveCalculatedTissue;
class PartialPressureGasItem;
class PartialGasPressureAxis;
class AbstractProfilePolygonItem;
class TankItem;
class DiveHandler;
class QGraphicsSimpleTextItem;
class QModelIndex;
class DivePictureItem;

class ProfileWidget2 : public QGraphicsView {
	Q_OBJECT
public:
	enum State {
		EMPTY,
		PROFILE,
		EDIT,
		ADD,
		PLAN,
		INVALID
	};
	enum Items {
		BACKGROUND,
		PROFILE_Y_AXIS,
		GAS_Y_AXIS,
		TIME_AXIS,
		DEPTH_CONTROLLER,
		TIME_CONTROLLER,
		COLUMNS
	};

	ProfileWidget2(QWidget *parent = 0);
	void plotDive(struct dive *d = 0, bool force = false);
	virtual bool eventFilter(QObject *, QEvent *);
	void setupItem(AbstractProfilePolygonItem *item, DiveCartesianAxis *hAxis, DiveCartesianAxis *vAxis, DivePlotDataModel *model, int vData, int hData, int zValue);
	void setPrintMode(bool mode, bool grayscale = false);
	bool getPrintMode();
	bool isPointOutOfBoundaries(const QPointF &point) const;
	bool isPlanner();
	bool isAddOrPlanner();
	double getFontPrintScale();
	void setFontPrintScale(double scale);
	void clearHandlers();
	State currentState;

public
slots: // Necessary to call from QAction's signals.
	void settingsChanged();
	void setEmptyState();
	void setProfileState();
	void setPlanState();
	void setAddState();
	void changeGas();
	void addSetpointChange();
	void addBookmark();
	void hideEvents();
	void unhideEvents();
	void removeEvent();
	void editName();
	void makeFirstDC();
	void deleteCurrentDC();
	void pointInserted(const QModelIndex &parent, int start, int end);
	void pointsRemoved(const QModelIndex &, int start, int end);
	void plotPictures();
	void replot();

	/* this is called for every move on the handlers. maybe we can speed up this a bit? */
	void recreatePlannedDive();

	/* key press handlers */
	void keyEscAction();
	void keyDeleteAction();
	void keyUpAction();
	void keyDownAction();
	void keyLeftAction();
	void keyRightAction();

	void divePlannerHandlerClicked();
	void divePlannerHandlerReleased();
protected:
	virtual void resizeEvent(QResizeEvent *event);
	virtual void wheelEvent(QWheelEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);

private: /*methods*/
	void fixBackgroundPos();
	void scrollViewTo(const QPoint &pos);
	void setupSceneAndFlags();
	void setupItemSizes();
	void addItemsToScene();
	void setupItemOnScene();
	void disconnectTemporaryConnections();
	struct plot_data *getEntryFromPos(QPointF pos);

private:
	DivePlotDataModel *dataModel;
	int zoomLevel;
	qreal zoomFactor;
	DivePixmapItem *background;
	QString backgroundFile;
	ToolTipItem *toolTipItem;
	bool isPlotZoomed;
	// All those here should probably be merged into one structure,
	// So it's esyer to replicate for more dives later.
	// In the meantime, keep it here.
	struct plot_info plotInfo;
	DepthAxis *profileYAxis;
	PartialGasPressureAxis *gasYAxis;
	TemperatureAxis *temperatureAxis;
	TimeAxis *timeAxis;
	DiveProfileItem *diveProfileItem;
	DiveTemperatureItem *temperatureItem;
	DiveCartesianAxis *cylinderPressureAxis;
	DiveGasPressureItem *gasPressureItem;
	MeanDepthLine *meanDepth;
	QList<DiveEventItem *> eventItems;
	DiveTextItem *diveComputerText;
	DiveCalculatedCeiling *diveCeiling;
	QList<DiveCalculatedTissue *> allTissues;
	DiveReportedCeiling *reportedCeiling;
	PartialPressureGasItem *pn2GasItem;
	PartialPressureGasItem *pheGasItem;
	PartialPressureGasItem *po2GasItem;
	DiveCartesianAxis *heartBeatAxis;
	DiveHeartrateItem *heartBeatItem;
	DiveCartesianAxis *percentageAxis;
	QList<DivePercentageItem *> allPercentages;
	DiveAmbPressureItem *ambPressureItem;
	DiveGFLineItem *gflineItem;
	DiveLineItem *mouseFollowerVertical;
	DiveLineItem *mouseFollowerHorizontal;
	RulerItem2 *rulerItem;
	TankItem *tankItem;
	bool isGrayscale;
	bool printMode;

	//specifics for ADD and PLAN
	QList<DiveHandler *> handles;
	QList<QGraphicsSimpleTextItem *> gases;
	QList<DivePictureItem*> pictures;
	void repositionDiveHandlers();
	int fixHandlerIndex(DiveHandler *activeHandler);
	friend class DiveHandler;
	QHash<Qt::Key, QAction *> actionsForKeys;
	bool shouldCalculateMaxTime;
	bool shouldCalculateMaxDepth;
	int maxtime;
	int maxdepth;
	double fontPrintScale;
};

#endif // PROFILEWIDGET2_H
