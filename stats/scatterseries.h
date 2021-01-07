// SPDX-License-Identifier: GPL-2.0
// A small custom scatter series, where every item represents a dive
// The original QScatterSeries was buggy and distinctly slower
#ifndef SCATTER_SERIES_H
#define SCATTER_SERIES_H

#include "statsseries.h"

#include <memory>
#include <vector>
#include <QGraphicsRectItem>

class QGraphicsPixmapItem;
class QGraphicsScene;
class InformationBox;
struct StatsVariable;
struct dive;

class ScatterSeries : public StatsSeries {
public:
	ScatterSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		      const StatsVariable &varX, const StatsVariable &varY);
	~ScatterSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;

	// Note: this expects that all items are added with increasing pos!
	void append(dive *d, double pos, double value);

private:
	// Get items under mouse.
	std::vector<int> getItemsUnderMouse(const QPointF &f) const;

	struct Item {
		std::unique_ptr<QGraphicsPixmapItem> item;
		dive *d;
		double pos, value;
		Item(QGraphicsScene *scene, ScatterSeries *series, dive *d, double pos, double value);
		void updatePosition(ScatterSeries *series);
		void highlight(bool highlight);
	};

	std::unique_ptr<InformationBox> information;
	std::vector<Item> items;
	std::vector<int> highlighted;
	const StatsVariable &varX;
	const StatsVariable &varY;
};

#endif
