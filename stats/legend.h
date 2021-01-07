// SPDX-License-Identifier: GPL-2.0
// A legend box, which is shown on the chart.
#ifndef STATS_LEGEND_H
#define STATS_LEGEND_H

#include "backend-shared/roundrectitem.h"

#include <memory>
#include <vector>

class QGraphicsScene;
class QGraphicsSceneMouseEvent;

class Legend : public RoundRectItem {
public:
	Legend(const std::vector<QString> &names);
	void hover(QPointF pos);
	void resize(); // called when the chart size changes.
private:
	// Each entry is a text besides a rectangle showing the color
	struct Entry {
		std::unique_ptr<QGraphicsRectItem> rect;
		std::unique_ptr<QGraphicsSimpleTextItem> text;
		QPointF pos;
		double width;
		Entry(const QString &name, int idx, int numBins, QGraphicsItem *parent);
	};
	int displayedItems;
	double width;
	double height;
	int fontHeight;
	std::vector<Entry> entries;
	void updatePosition();
	void hide();
};

#endif
