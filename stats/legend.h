// SPDX-License-Identifier: GPL-2.0
// A legend box, which is shown on the chart.
#ifndef STATS_LEGEND_H
#define STATS_LEGEND_H

#include "chartitem.h"

#include <memory>
#include <vector>
#include <QFont>

class QFontMetrics;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;

class Legend : public ChartRectItem {
public:
	Legend(StatsView &view, const std::vector<QString> &names);
	void resize(); // called when the chart size changes.
private:
	// Each entry is a text besides a rectangle showing the color
	struct Entry {
		QString name;
		QBrush rectBrush;
		QPointF pos;
		double width;
		Entry(const QString &name, int idx, int numBins, const QFontMetrics &fm);
	};
	int displayedItems;
	double width;
	double height;
	QFont font;
	int fontHeight;
	std::vector<Entry> entries;
	void hide();
};

#endif
