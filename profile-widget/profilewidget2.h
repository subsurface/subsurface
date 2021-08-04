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
#include "core/pictureobj.h"
#include "core/units.h"
#include "core/subsurface-qt/divelistnotifier.h"

class ProfileScene;
class RulerItem2;
struct dive;
class ToolTipItem;
class DiveEventItem;
class DivePlannerPointsModel;
class DiveHandler;
class QGraphicsSimpleTextItem;
class QModelIndex;
class DivePictureItem;

class ProfileWidget2 : public QGraphicsView {
	Q_OBJECT
public:
	enum State {
		PROFILE,
		EDIT,
		PLAN,
		INIT
	};

	// Pass null as plannerModel if no support for planning required
	ProfileWidget2(DivePlannerPointsModel *plannerModel, double fontPrintScale, QWidget *parent = 0);
	~ProfileWidget2();
	void resetZoom();
	void scale(qreal sx, qreal sy);
	void plotDive(const struct dive *d, int dc, bool clearPictures = false, bool instant = false);
	void setProfileState(const struct dive *d, int dc);
	void setPlanState(const struct dive *d, int dc);
	void setEditState(const struct dive *d, int dc);
	void setPrintMode(bool grayscale = false);
	bool isPlanner() const;
	void clear();
#ifndef SUBSURFACE_MOBILE
	bool eventFilter(QObject *, QEvent *) override;
#endif
	std::unique_ptr<ProfileScene> profileScene;
	State currentState;

signals:
	void editCurrentDive();

public
slots: // Necessary to call from QAction's signals.
	void settingsChanged();
	void actionRequestedReplot(bool triggered);
	void divesChanged(const QVector<dive *> &dives, DiveField field);
#ifndef SUBSURFACE_MOBILE
	void plotPictures();
	void picturesRemoved(dive *d, QVector<QString> filenames);
	void picturesAdded(dive *d, QVector<PictureObj> pics);
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
	void setProfileState(); // keep currently displayed dive
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
	void scrollViewTo(const QPoint &pos);
	void setupSceneAndFlags();
	void addItemsToScene();
	void setupItemOnScene();
	void disconnectTemporaryConnections();
	struct plot_data *getEntryFromPos(QPointF pos);
	void addActionShortcut(const Qt::Key shortcut, void (ProfileWidget2::*slot)());
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
	void renameCurrentDC();

	DivePlannerPointsModel *plannerModel; // If null, no planning supported.
	int zoomLevel;
	qreal zoomFactor;
#ifndef SUBSURFACE_MOBILE
	ToolTipItem *toolTipItem;
#endif
	// All those here should probably be merged into one structure,
	// So it's esyer to replicate for more dives later.
	// In the meantime, keep it here.
	const struct dive *d;
	int dc;
#ifndef SUBSURFACE_MOBILE
	DiveLineItem *mouseFollowerVertical;
	DiveLineItem *mouseFollowerHorizontal;
	RulerItem2 *rulerItem;
#endif

	std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> gases;

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
	bool shouldCalculateMax; // Calculate maximum time and depth (default). False when dragging handles.

	// We store a const pointer to the shown dive. However, the undo commands want
	// (understandably) a non-const pointer. Since the profile has a context-menu
	// with actions, it needs such a non-const pointer. This function turns the
	// currently shown dive into such a pointer. Ugly, yes.
	struct dive *mutable_dive() const;
};

#endif // PROFILEWIDGET2_H
