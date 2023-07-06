// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILE_VIEW_H
#define PROFILE_VIEW_H

#include "qt-quick/chartview.h"
#include <memory>

class ChartGraphicsSceneItem;
class ProfileAnimation;
class ProfileScene;
class ToolTipItem;

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

	struct RenderFlags {
		static constexpr int None = 0;
		static constexpr int Instant = 1 << 0;
		static constexpr int DontRecalculatePlotInfo = 1 << 1;
		static constexpr int EditMode = 1 << 2;
		static constexpr int PlanMode = 1 << 3;
	};

	void plotDive(const struct dive *d, int dc, int flags = RenderFlags::None);
	void clear();
	void resetZoom();
	void anim(double fraction);

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
private:
	const struct dive *d;
	int dc;
	double dpr;
	double zoomLevel, zoomLevelPinchStart;
	double zoomedPosition;	// Position when zoomed: 0.0 = beginning, 1.0 = end.
	bool panning; // Currently panning.
	double panningOriginalMousePosition;
	double panningOriginalProfilePosition;
	bool empty; // No dive shown.
	bool shouldCalculateMax; // Calculate maximum time and depth (default). False when dragging handles.
	QColor background;
	std::unique_ptr<ProfileScene> profileScene;
	ChartItemPtr<ChartGraphicsSceneItem> profileItem;
	std::unique_ptr<ProfileAnimation> animation;

	void plotAreaChanged(const QSizeF &size) override;
	void resetPointers() override;
	void replot();
	void setZoom(double level);

	void hoverEnterEvent(QHoverEvent *event) override;
	void hoverMoveEvent(QHoverEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;

	ChartItemPtr<ToolTipItem> tooltip;
	void updateTooltip(QPointF pos, bool plannerMode, int animSpeed);
	std::unique_ptr<ProfileAnimation> tooltip_animation;

	QPointF previousHoveMovePosition;

	// For mobile
	int getDiveId() const;
	void setDiveId(int id);
	int numDC() const;
	void rotateDC(int dir);
};

#endif
