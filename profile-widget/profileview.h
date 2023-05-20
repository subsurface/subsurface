// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILE_VIEW_H
#define PROFILE_VIEW_H

#include "qt-quick/chartview.h"
#include <memory>

class ChartGraphicsSceneItem;
class ProfileAnimation;
class ProfileScene;

class ProfileView : public ChartView {
	Q_OBJECT
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
	void anim(double fraction);
private:
	const struct dive *d;
	int dc;
	int zoomLevel;
	double zoomedPosition;	// Position when zoomed: 0.0 = beginning, 1.0 = end.
	bool empty; // No dive shown.
	bool shouldCalculateMax; // Calculate maximum time and depth (default). False when dragging handles.
	QColor background;
	std::unique_ptr<ProfileScene> profileScene;
	ChartItemPtr<ChartGraphicsSceneItem> profileItem;
	std::unique_ptr<ProfileAnimation> animation;

	void plotAreaChanged(const QSizeF &size) override;
	void resetPointers() override;
	void replot();
};

#endif
