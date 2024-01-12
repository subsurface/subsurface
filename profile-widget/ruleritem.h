// SPDX-License-Identifier: GPL-2.0
#ifndef RULERITEM_H
#define RULERITEM_H

#include "qt-quick/chartitem.h"

#include <QFont>
#include <QFontMetrics>

class ProfileView;
class ProfileScene;
class RulerItemHandle;
struct plot_info;
struct dive;

class RulerItem {
	ChartItemPtr<ChartLineItem> line;
	ChartItemPtr<RulerItemHandle> handle1, handle2;
	ChartItemPtr<AnimatedChartRectItem> tooltip;

	QFont font;
	QFontMetrics fm;
	double fontHeight;
	QPixmap title;
public:
	RulerItem(ProfileView &view, double dpr);
	void setVisible(bool visible);
	void update(const dive *d, double dpr, const ProfileScene &scene, const plot_info &info, int animspeed);
	void anim(double progress);
};

#endif
