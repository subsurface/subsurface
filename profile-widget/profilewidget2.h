// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILEWIDGET2_H
#define PROFILEWIDGET2_H

#include <QGraphicsView>
#include <vector>
#include <memory>

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
#include "profile-widget/divelineitem.h"
#include "profile-widget/diveprofileitem.h"
#include "core/display.h"
#include "core/color.h"
#include "core/units.h"

class RulerItem2;
struct dive;
struct plot_info;
class ToolTipItem;
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
	void resetZoom();
	void scale(qreal sx, qreal sy);
	void plotDive(const struct dive *d = 0, bool force = false, bool clearPictures = false, bool instant = false);
	void setupItem(AbstractProfilePolygonItem *item, DiveCartesianAxis *vAxis, int vData, int hData, int zValue);
	void setPrintMode(bool mode, bool grayscale = false);
	bool getPrintMode();
	bool isPointOutOfBoundaries(const QPointF &point) const;
	bool isPlanner();
	double getFontPrintScale();
	void setFontPrintScale(double scale);
#ifndef SUBSURFACE_MOBILE
	bool eventFilter(QObject *, QEvent *) override;
	void clearHandlers();
#endif
	void recalcCeiling();
	void setToolTipVisibile(bool visible);
	State currentState;
	int animSpeed;

signals:
	void fontPrintScaleChanged(double scale);
	void enableToolbar(bool enable);
	void enableShortcuts();
	void disableShortcuts(bool paste);
	void refreshDisplay(bool recreateDivelist);
	void updateDiveInfo();
	void editCurrentDive();
	void dateTimeChangedItems();

public
slots: // Necessary to call from QAction's signals.
	void dateTimeChanged();
	void settingsChanged();
	void actionRequestedReplot(bool triggered);
	void setEmptyState();
	void setProfileState();
	void setReplot(bool state);
	void replot(dive *d = 0);
#ifndef SUBSURFACE_MOBILE
	void plotPictures();
	void removePictures(const QVector<QString> &fileUrls);
	void setPlanState();
	void setAddState();
	void changeGas();
	void addSetpointChange();
	void splitDive();
	void addBookmark();
	void addDivemodeSwitch();
	void hideEvents();
	void unhideEvents();
	void removeEvent();
	void editName();
	void makeFirstDC();
	void deleteCurrentDC();
	void splitCurrentDC();
	void pointInserted(const QModelIndex &parent, int start, int end);
	void pointsRemoved(const QModelIndex &, int start, int end);
	void updateThumbnail(QString filename, QImage thumbnail, duration_t duration);

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
#endif

protected:
	void resizeEvent(QResizeEvent *event) override;
#ifndef SUBSURFACE_MOBILE
	void wheelEvent(QWheelEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
#endif
	void dropEvent(QDropEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;


private: /*methods*/
	void fixBackgroundPos();
	void scrollViewTo(const QPoint &pos);
	void setupSceneAndFlags();
	void setupItemSizes();
	void addItemsToScene();
	void setupItemOnScene();
	void disconnectTemporaryConnections();
	struct plot_data *getEntryFromPos(QPointF pos);
	void addActionShortcut(const Qt::Key shortcut, void (ProfileWidget2::*slot)());
	void createPPGas(PartialPressureGasItem *item, int verticalColumn, color_index_t color, color_index_t colorAlert,
			 const double *thresholdSettingsMin, const double *thresholdSettingsMax);
	void clearPictures();
	void plotPicturesInternal(const struct dive *d, bool synchronous);
private:
	DivePlotDataModel *dataModel;
	int zoomLevel;
	qreal zoomFactor;
	DivePixmapItem *background;
	QString backgroundFile;
#ifndef SUBSURFACE_MOBILE
	ToolTipItem *toolTipItem;
#endif
	bool isPlotZoomed;
	bool replotEnabled;
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
	DiveMeanDepthItem *meanDepthItem;
	DiveCartesianAxis *cylinderPressureAxis;
	DiveGasPressureItem *gasPressureItem;
	QList<DiveEventItem *> eventItems;
	DiveTextItem *diveComputerText;
	DiveReportedCeiling *reportedCeiling;
	PartialPressureGasItem *pn2GasItem;
	PartialPressureGasItem *pheGasItem;
	PartialPressureGasItem *po2GasItem;
	PartialPressureGasItem *o2SetpointGasItem;
	PartialPressureGasItem *ccrsensor1GasItem;
	PartialPressureGasItem *ccrsensor2GasItem;
	PartialPressureGasItem *ccrsensor3GasItem;
	PartialPressureGasItem *ocpo2GasItem;
#ifndef SUBSURFACE_MOBILE
	DiveCalculatedCeiling *diveCeiling;
	DiveTextItem *decoModelParameters;
	QList<DiveCalculatedTissue *> allTissues;
	DiveCartesianAxis *heartBeatAxis;
	DiveHeartrateItem *heartBeatItem;
	DiveCartesianAxis *percentageAxis;
	QList<DivePercentageItem *> allPercentages;
	DiveAmbPressureItem *ambPressureItem;
	DiveGFLineItem *gflineItem;
	DiveLineItem *mouseFollowerVertical;
	DiveLineItem *mouseFollowerHorizontal;
	RulerItem2 *rulerItem;
#endif
	TankItem *tankItem;
	bool isGrayscale;
	bool printMode;

	QList<QGraphicsSimpleTextItem *> gases;

	//specifics for ADD and PLAN
#ifndef SUBSURFACE_MOBILE
	// The list of pictures in this plot. The pictures are sorted by offset in seconds.
	// For the same offset, sort by filename.
	// Pictures that are outside of the dive time are not shown.
	struct PictureEntry {
		offset_t offset;
		duration_t duration;
		QString filename;
		std::unique_ptr<DivePictureItem> thumbnail;
		// For videos with known duration, we represent the duration of the video by a line
		std::unique_ptr<QGraphicsRectItem> durationLine;
		PictureEntry (offset_t offsetIn, const QString &filenameIn, QGraphicsScene *scene, bool synchronous);
		bool operator< (const PictureEntry &e) const;
	};
	void updateThumbnailXPos(PictureEntry &e);
	std::vector<PictureEntry> pictures;
	void calculatePictureYPositions();
	void updateDurationLine(PictureEntry &e);
	void updateThumbnailPaintOrder();

	QList<DiveHandler *> handles;
	void repositionDiveHandlers();
	int fixHandlerIndex(DiveHandler *activeHandler);
	friend class DiveHandler;
#endif
	QHash<Qt::Key, QAction *> actionsForKeys;
	bool shouldCalculateMaxTime;
	bool shouldCalculateMaxDepth;
	int maxtime;
	int maxdepth;
	double fontPrintScale;
};

#endif // PROFILEWIDGET2_H
