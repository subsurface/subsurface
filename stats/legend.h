// SPDX-License-Identifier: GPL-2.0
// A legend box, which is shown on the chart.
#ifndef STATS_LEGEND_H
#define STATS_LEGEND_H

#include <memory>
#include <vector>
#include <QGraphicsRectItem>

class QGraphicsSceneMouseEvent;

class Legend : public QGraphicsRectItem {
public:
	Legend(QGraphicsWidget *chart, const std::vector<QString> &names);
	void hover(QPointF pos);
	void resize(); // called when the chart size changes.
private:
	// Each entry is a text besides a rectangle showing the color
	struct Entry {
		std::unique_ptr<QGraphicsRectItem> rect;
		std::unique_ptr<QGraphicsSimpleTextItem> text;
		QPointF pos;
		double width;
		Entry(const QString &name, int idx, QGraphicsItem *parent);
	};
	QGraphicsWidget *chart;
	int displayedItems;
	double width;
	double height;
	int fontHeight;
	std::vector<Entry> entries;
	void updatePosition();
	void hide();
};

#endif
