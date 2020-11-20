// SPDX-License-Identifier: GPL-2.0
// A small custom scatter series, where every item represents a dive
// The original QScatterSeries was buggy and distinctly slower
#ifndef SCATTER_SERIES_H
#define SCATTER_SERIES_H

#include <memory>
#include <vector>
#include <QScatterSeries>

namespace QtCharts {
	class QAbstractAxis;
	class QChart;
}
class QGraphicsPixmapItem;

// We derive from a proper scatter series to get access to the map-to
// and map-from coordinates calls. But we don't use any of its functionality.
// Can we avoid that?

class ScatterSeries : public QtCharts::QScatterSeries {
public:
	// A short line used to mark quartiles
	struct Item {
		std::unique_ptr<QGraphicsPixmapItem> item;
		double pos, value;
		Item(QtCharts::QChart *chart, ScatterSeries *series, double pos, double value);
		void updatePosition(QtCharts::QChart *chart, ScatterSeries *series);
	};

	// Call if chart geometry changed
	void updatePositions();

	// Note: this expects that all items are added with increasing pos!
	void append(double pos, double value);
private:
	std::vector<Item> items;
};

#endif
