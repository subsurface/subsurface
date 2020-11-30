// SPDX-License-Identifier: GPL-2.0
// A small custom bar series, which displays information
// when hovering over a bar. The QtCharts bar series were
// too inflexible with respect to placement of the bars.
#ifndef BAR_SERIES_H
#define BAR_SERIES_H

#include <memory>
#include <vector>
#include <QGraphicsRectItem>
#include <QScatterSeries>

namespace QtCharts {
	class QAbstractAxis;
	class QChart;
}
class StatsType;

// We derive from a proper scatter series to get access to the map-to
// and map-from coordinates calls. But we don't use any of its functionality.
// Can we avoid that?

class BarSeries : public QtCharts::QScatterSeries {
public:
	// If the horizontal flag is true, independent variable is plotted on the y-axis.
	BarSeries(bool horizontal);

	// Call if chart geometry changed
	void updatePositions();

	// Note: this expects that all items are added with increasing pos
	// and that no bar is inside another bar, i.e. lowerBound and upperBound
	// are ordered identically.
	void append(double lowerBound, double upperBound, double value, const std::vector<QString> &label);

	// Get item under mous pointer, or -1 if none
	int getItemUnderMouse(const QPointF &f);

	// Highlight item when hovering over item
	void highlight(int index);
private:
	// A label that is composed of multiple lines
	struct BarLabel {
		std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> items;
		double value, height; // Position and size of bar in graph
		double totalWidth, totalHeight; // Size of the item
		BarLabel(const std::vector<QString> &labels, double value, double height, bool horizontal,
			 QtCharts::QChart *chart, QtCharts::QAbstractSeries *series);
		void updatePosition(QtCharts::QChart *chart, QtCharts::QAbstractSeries *series, bool horizontal);
	};

	struct Item {
		std::unique_ptr<QGraphicsRectItem> item;
		double lowerBound, upperBound, value;
		Item(QtCharts::QChart *chart, BarSeries *series, double lowerBound, double upperBound, double value, bool horizontal);
		void updatePosition(QtCharts::QChart *chart, BarSeries *series, bool horizontal);
		void highlight(bool highlight);
	};

	std::vector<Item> items;
	std::vector<BarLabel> barLabels;
	bool horizontal;
	int highlighted; // -1: no item highlighted
};

#endif
