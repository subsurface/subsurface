// SPDX-License-Identifier: GPL-2.0
// A small custom scatter series, where every item represents a dive
// The original QScatterSeries was buggy and distinctly slower
#ifndef SCATTER_SERIES_H
#define SCATTER_SERIES_H

#include <memory>
#include <vector>
#include <QGraphicsRectItem>
#include <QScatterSeries>

namespace QtCharts {
	class QAbstractAxis;
	class QChart;
}
class QGraphicsPixmapItem;
class StatsType;
struct dive;

// We derive from a proper scatter series to get access to the map-to
// and map-from coordinates calls. But we don't use any of its functionality.
// Can we avoid that?

class ScatterSeries : public QtCharts::QScatterSeries {
public:
	ScatterSeries(const StatsType &typeX, const StatsType &typeY);

	// Call if chart geometry changed
	void updatePositions();

	// Note: this expects that all items are added with increasing pos!
	void append(dive *d, double pos, double value);

	// Get closest item. Returns square of distance as double and item index.
	// If the index is -1, no item is inside the range.
	// Super weird: this function can't be const, because QChart::mapToValue takes
	// a non-const reference!?
	std::pair<double, int> getClosest(const QPointF &f);

	// Highlight item when hovering over item
	void highlight(int index);
private:
	struct Item {
		std::unique_ptr<QGraphicsPixmapItem> item;
		dive *d;
		double pos, value;
		Item(QtCharts::QChart *chart, ScatterSeries *series, dive *d, double pos, double value);
		void updatePosition(QtCharts::QChart *chart, ScatterSeries *series);
		void highlight(bool highlight);
	};

	// Information window showing data of highlighted dive
	struct Information : QGraphicsRectItem {
		Information(QtCharts::QChart *chart);
		void set(const dive *d, QPointF pos, const StatsType &typeX, const StatsType &typeY);
	private:
		double width, height;
		void addLine(const QString &s);
		void addDataLine(const StatsType &type, const dive *d);
		std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> textItems;
	};

	std::unique_ptr<Information> information;
	std::vector<Item> items;
	int highlighted; // -1: no item highlighted
	const StatsType &typeX;
	const StatsType &typeY;
};

#endif
