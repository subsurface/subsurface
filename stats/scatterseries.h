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
class InformationBox;
struct StatsVariable;
struct dive;

class ScatterSeries : public StatsSeries {
public:
	ScatterSeries(QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis,
		      const StatsVariable &varX, const StatsVariable &varY);
	~ScatterSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;

	// Note: this expects that all items are added with increasing pos!
	void append(dive *d, double pos, double value);

private:
	// Get items under mouse.
	// Super weird: this function can't be const, because QChart::mapToValue takes
	// a non-const reference!?
	std::vector<int> getItemsUnderMouse(const QPointF &f);

	struct Item {
		std::unique_ptr<QGraphicsPixmapItem> item;
		dive *d;
		double pos, value;
		Item(QtCharts::QChart *chart, ScatterSeries *series, dive *d, double pos, double value);
		void updatePosition(QtCharts::QChart *chart, ScatterSeries *series);
		void highlight(bool highlight);
	};

	std::unique_ptr<InformationBox> information;
	std::vector<Item> items;
	std::vector<int> highlighted;
	const StatsVariable &varX;
	const StatsVariable &varY;
};

#endif
