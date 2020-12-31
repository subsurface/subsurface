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
struct StatsType;
struct dive;

class ScatterSeries : public StatsSeries {
public:
	ScatterSeries(QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis,
		      const StatsType &typeX, const StatsType &typeY);
	~ScatterSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;

	// Note: this expects that all items are added with increasing pos!
	void append(dive *d, double pos, double value);

private:
	// Get closest item. If the return value is -1, no item is inside the range.
	// Super weird: this function can't be const, because QChart::mapToValue takes
	// a non-const reference!?
	int getItemUnderMouse(const QPointF &f);

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
	int highlighted; // -1: no item highlighted
	const StatsType &typeX;
	const StatsType &typeY;
};

#endif
