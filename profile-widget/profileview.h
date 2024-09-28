// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILE_VIEW_H
#define PROFILE_VIEW_H

#include "qt-quick/chartview.h"
#include "core/units.h"
#include <memory>

class ChartGraphicsSceneItem;
class ChartLineItem;
class ChartRectItem;
class DiveEventItem;
class DivePlannerPointsModel;
class HandleItem;
class PictureItem;
class ProfileAnimation;
class ProfileScene;
class ToolTipItem;
struct picture;
class RulerItem;
class QModelIndex;

class ProfileView : public ChartView {
	Q_OBJECT

	// Communication with the mobile interface is via properties. I hate it.
	Q_PROPERTY(int diveId READ getDiveId WRITE setDiveId)
	Q_PROPERTY(int numDC READ numDC NOTIFY numDCChanged)
	Q_PROPERTY(double zoomLevel MEMBER zoomLevel NOTIFY zoomLevelChanged)
public:
	ProfileView();
	ProfileView(QQuickItem *parent);
	~ProfileView();

	// Flag set when constructing the object. Because QtQuick may decide to destroy the old one. :(
	bool initialized;
	void setPlannerModel(DivePlannerPointsModel &model); // enables planning and edit mdoe

	struct RenderFlags {
		static constexpr int None = 0;
		static constexpr int Instant = 1 << 0;
		static constexpr int DontRecalculatePlotInfo = 1 << 1;
		static constexpr int EditMode = 1 << 2;
		static constexpr int PlanMode = 1 << 3;
		static constexpr int Simplified = 1 << 4; // For mobile's overview page
		static constexpr int DontCalculateMax = 1 << 5;
	};

	void plotDive(const struct dive *d, int dc, int flags = RenderFlags::None);
	int timeAt(QPointF pos) const;
	void clear();
	void resetZoom();
	void anim(double fraction);
	void rulerDragged();				// Called by the RulterItem when a handle was dragged.
	void handleSelected(int idx);			// Called by the HandleItem when it is clicked.
	void handleDragged(int idx, QPointF pos);	// Called by the HandleItem when it is dragged.
	void handleReleased(int idx);			// Called by the HandleItem when it is released.

	// For mobile
	Q_INVOKABLE void pinchStart();
	Q_INVOKABLE void pinch(double factor);
	Q_INVOKABLE void nextDC();
	Q_INVOKABLE void prevDC();
	Q_INVOKABLE void panStart(double x, double y);
	Q_INVOKABLE void pan(double x, double y);

signals:
	void numDCChanged();
	void zoomLevelChanged();
	void stopAdded(); // only emitted in edit mode
	void stopRemoved(int count); // only emitted in edit mode
	void stopMoved(int count); // only emitted in edit mode
private:
	enum Mode {
		Normal,
		Edit,
		Plan
	};
	const struct dive *d;
	int dc;
	DivePlannerPointsModel *plannerModel;
	Mode mode;
	bool simplified;
	double dpr;
	double zoomLevel, zoomLevelPinchStart;
	double zoomedPosition;	// Position when zoomed: 0.0 = beginning, 1.0 = end.
	bool panning; // Currently panning.
	double panningOriginalMousePosition;
	double panningOriginalProfilePosition;
	bool empty; // No dive shown.
	QColor background;
	std::unique_ptr<ProfileScene> profileScene;
	ChartItemPtr<ChartGraphicsSceneItem> profileItem;
	std::unique_ptr<ProfileAnimation> animation;
	ChartItemPtr<ChartLineItem> mouseFollowerHorizontal, mouseFollowerVertical;
	dive *mutable_dive() const; // for functions manipulating the dive

	void plotAreaChanged(const QSizeF &size) override;
	void resetPointers() override;
	void replot();
	int rerenderFlags() const;
	void setZoom(double level);

	void hoverEnterEvent(QHoverEvent *event) override;
	void hoverMoveEvent(QHoverEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *e) override;
	void contextMenu(const QPointF pos, const QPoint globalPos);
	void addSetpointChange(int seconds);

	ChartItemPtr<ToolTipItem> tooltip;
	void updateTooltip(QPointF pos, bool plannerMode, int animSpeed);
	std::unique_ptr<ProfileAnimation> tooltip_animation;

	std::unique_ptr<RulerItem> ruler;
	void updateRuler(int animSpeed);
	std::unique_ptr<ProfileAnimation> ruler_animation;

	void updateMouseFollowers(QPointF pos);

	QPointF previousHoverMovePosition;
	std::vector<std::unique_ptr<HandleItem>> handles;
	int selectedHandleIdx;
	void clearHandles();
	void resetHandles();
	void placeHandles();
	void reindexHandles();
	void moveHandle(int time, int depth);
	void deleteHandle();

	void pointsInserted(const QModelIndex &, int from, int to);
	void pointsRemoved(const QModelIndex &, int start, int end);
	void pointsMoved(const QModelIndex &, int start, int end, const QModelIndex &destination, int row);

	// DC related
	void renameCurrentDC();

	// Event related
	void editEventName(const struct event &event, int idx);
	void hideEvent(DiveEventItem &item);
	void hideEventType(DiveEventItem &item);
	void removeEvent(DiveEventItem &item);
	void unhideEvents();

	// The list of pictures in this plot. The pictures are sorted by offset in seconds.
	// For the same offset, sort by filename.
	// Pictures that are outside of the dive time are not shown.
	struct PictureEntry {
		offset_t offset;
		double x, y;
		duration_t duration;
		QString filename;
		ChartItemPtr<PictureItem> thumbnail;
		// For videos with known duration, we represent the duration of the video by a line
		ChartItemPtr<ChartRectItem> durationLine;
		std::unique_ptr<ProfileAnimation> animation;
		PictureEntry (offset_t offset, const QString &filename, ChartItemPtr<PictureItem> thumbnail, double dpr, bool synchronous);
		bool operator< (const PictureEntry &e) const;
	};
	std::vector<PictureEntry> pictures;
	PictureEntry *highlightedPicture;

	// Picture (media) related functions
	void picturesRemoved(dive *d, QVector<QString> filenames);
	void picturesAdded(dive *d, QVector<picture> pics);
	void pictureOffsetChanged(dive *d, QString filename, offset_t offset);
	void updateDurationLine(PictureEntry &e);
	void updateThumbnail(QString filename, QImage thumbnail, duration_t duration);
	void updateThumbnailPaintOrder();
	void calculatePictureYPositions();
	void updateThumbnailXPos(PictureEntry &e);
	void plotPictures(const struct dive *d, bool synchronous);
	void updateThumbnails();
	void clearPictures();
	void removePictureThumbnail(PictureEntry &entry);
	void unhighlightPicture();
	void shrinkPictureItem(PictureEntry &e, int animSpeed);
	void growPictureItem(PictureEntry &e, int animSpeed);
	void moveThumbnailBefore(PictureEntry &e, std::vector<PictureEntry>::iterator &before);
	PictureEntry *getPictureUnderMouse(QPointF pos);

	// For mobile
	int getDiveId() const;
	void setDiveId(int id);
	int numDC() const;
	void rotateDC(int dir);
};

#endif
