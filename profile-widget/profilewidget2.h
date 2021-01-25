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
#include "core/pictureobj.h"
#include "core/units.h"
#include "core/subsurface-qt/divelistnotifier.h"

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
class DivePlannerPointsModel;
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

	// Pass null as plannerModel if no support for planning required
	ProfileWidget2(DivePlannerPointsModel *plannerModel, QWidget *parent = 0);
	~ProfileWidget2();
	void resetZoom();
	void scale(qreal sx, qreal sy);
	void plotDive(const struct dive *d, bool force = false, bool clearPictures = false, bool instant = false);
	void setPrintMode(bool mode, bool grayscale = false);
	bool getPrintMode() const;
	bool isPointOutOfBoundaries(const QPointF &point) const;
	bool isPlanner() const;
	double getFontPrintScale() const;
	void setFontPrintScale(double scale);
#ifndef SUBSURFACE_MOBILE
	bool eventFilter(QObject *, QEvent *) override;
#endif
	void setToolTipVisibile(bool visible);
	State currentState;
	int animSpeed;

signals:
	void fontPrintScaleChanged(double scale);
	void enableToolbar(bool enable);
	void enableShortcuts();
	void disableShortcuts(bool paste);
	void editCurrentDive();

public
slots: // Necessary to call from QAction's signals.
	void settingsChanged();
	void actionRequestedReplot(bool triggered);
	void divesChanged(const QVector<dive *> &dives, DiveField field);
	void setEmptyState();
	void setProfileState();
#ifndef SUBSURFACE_MOBILE
	void plotPictures();
	void picturesRemoved(dive *d, QVector<QString> filenames);
	void picturesAdded(dive *d, QVector<PictureObj> pics);
	void setPlanState();
	void setAddState();
	void pointsReset();
	void pointInserted(const QModelIndex &parent, int start, int end);
	void pointsRemoved(const QModelIndex &, int start, int end);
	void pointsMoved(const QModelIndex &, int start, int end, const QModelIndex &destination, int row);
	void updateThumbnail(QString filename, QImage thumbnail, duration_t duration);
	void profileChanged(dive *d);
	void pictureOffsetChanged(dive *d, QString filename, offset_t offset);
	void removePicture(const QString &fileUrl);

	/* this is called for every move on the handlers. maybe we can speed up this a bit? */
	void divePlannerHandlerMoved();

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

private:
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

	void replot();
	void changeGas(int tank, int seconds);
	void fixBackgroundPos();
	void scrollViewTo(const QPoint &pos);
	void setupSceneAndFlags();
	template<typename T, class... Args> T *createItem(const DiveCartesianAxis &vAxis, int vColumn, int z, Args&&... args);
	void setupItemSizes();
	void addItemsToScene();
	void setupItemOnScene();
	void disconnectTemporaryConnections();
	struct plot_data *getEntryFromPos(QPointF pos);
	void addActionShortcut(const Qt::Key shortcut, void (ProfileWidget2::*slot)());
	PartialPressureGasItem *createPPGas(int column, color_index_t color, color_index_t colorAlert,
					    const double *thresholdSettingsMin, const double *thresholdSettingsMax);
	void clearPictures();
	void plotPicturesInternal(const struct dive *d, bool synchronous);
	void addDivemodeSwitch(int seconds, int divemode);
	void addBookmark(int seconds);
	void splitDive(int seconds);
	void addSetpointChange(int seconds);
	void removeEvent(DiveEventItem *item);
	void hideEvents(DiveEventItem *item);
	void editName(DiveEventItem *item);
	void unhideEvents();
	void makeFirstDC();
	void deleteCurrentDC();
	void splitCurrentDC();

	DivePlotDataModel *dataModel;
	DivePlannerPointsModel *plannerModel; // If null, no planning supported.
	int zoomLevel;
	qreal zoomFactor;
	bool isGrayscale;
	bool printMode;
	DivePixmapItem *background;
	QString backgroundFile;
#ifndef SUBSURFACE_MOBILE
	ToolTipItem *toolTipItem;
#endif
	// All those here should probably be merged into one structure,
	// So it's esyer to replicate for more dives later.
	// In the meantime, keep it here.
	struct plot_info plotInfo;
	DepthAxis *profileYAxis;
	PartialGasPressureAxis *gasYAxis;
	TemperatureAxis *temperatureAxis;
	TimeAxis *timeAxis;
	std::vector<AbstractProfilePolygonItem *> profileItems;
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
	DiveCalculatedCeiling *diveCeiling;
	DiveTextItem *decoModelParameters;
#ifndef SUBSURFACE_MOBILE
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

	std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> gases;

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
		PictureEntry (offset_t offsetIn, const QString &filenameIn, ProfileWidget2 *profile, bool synchronous);
		bool operator< (const PictureEntry &e) const;
	};
	void updateThumbnailXPos(PictureEntry &e);
	std::vector<PictureEntry> pictures;
	void calculatePictureYPositions();
	void updateDurationLine(PictureEntry &e);
	void updateThumbnailPaintOrder();
#endif

	std::vector<std::unique_ptr<DiveHandler>> handles;
	int handleIndex(const DiveHandler *h) const;
#ifndef SUBSURFACE_MOBILE
	void connectPlannerModel();
	void repositionDiveHandlers();
	int fixHandlerIndex(DiveHandler *activeHandler);
	DiveHandler *createHandle();
	QGraphicsSimpleTextItem *createGas();
#endif
	friend class DiveHandler;
	QHash<Qt::Key, QAction *> actionsForKeys;
	bool shouldCalculateMaxTime;
	bool shouldCalculateMaxDepth;
	int maxtime;
	int maxdepth;
	double fontPrintScale;
};

#endif // PROFILEWIDGET2_H
