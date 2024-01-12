// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILE_TOOLTIPITEM_H
#define PROFILE_TOOLTIPITEM_H

#include "qt-quick/chartitem.h"

#include <QFont>
#include <QFontMetrics>
#include <QPixmap>

struct dive;
struct plot_info;

class ToolTipItem : public AnimatedChartRectItem {
public:
	ToolTipItem(ChartView &view, double dpr);
	void update(const dive *d, double dpr, int time, const plot_info &pInfo,
		    const std::vector<std::pair<QString, QPixmap>> &events, bool inPlanner, int animSpeed);
private:
	QFont font;
	QFontMetrics fm;
	double fontHeight;
	QPixmap title;
	double width, height;

	QPixmap stringToPixmap(const QString &s) const;

	void stopDrag(QPointF pos) override;
};


#endif // PROFILE_TOOLTIPITEM
