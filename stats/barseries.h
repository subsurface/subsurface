// SPDX-License-Identifier: GPL-2.0
// A small custom bar series, which displays information
// when hovering over a bar. The QtCharts bar series were
// too inflexible with respect to placement of the bars.
#ifndef BAR_SERIES_H
#define BAR_SERIES_H

#include "stats/statstypes.h"
#include <memory>
#include <vector>
#include <QGraphicsRectItem>
#include <QScatterSeries>

namespace QtCharts {
	class QAbstractAxis;
	class QChart;
}
class InformationBox;

struct BarSeriesIndex {
	int bar;
	int subitem;
};

// We derive from a proper scatter series to get access to the map-to
// and map-from coordinates calls. But we don't use any of its functionality.
// Can we avoid that?

class BarSeries : public QtCharts::QScatterSeries {
public:
	// If the horizontal flag is true, independent variable is plotted on the y-axis.
	// A non-empty valueBinNames vector flags that this is a stacked bar chart.
	// In that case, a valueType must also be provided.
	// For count-based bar series in one variable, valueType is null.
	BarSeries(bool horizontal, bool stacked, const QString &categoryName, const StatsType *valueType,
		  std::vector<QString> valueBinNames);
	~BarSeries();

	// Call if chart geometry changed
	void updatePositions();

	// Note: this expects that all items are added with increasing pos
	// and that no bar is inside another bar, i.e. lowerBound and upperBound
	// are ordered identically.
	// There are three versions: for value-based (mean, etc) charts, for counts
	// based charts and for stacked bar charts with multiple items.
	void append(double lowerBound, double upperBound, int count, const std::vector<QString> &label,
		    const QString &binName, int total);
	void append(double lowerBound, double upperBound, double value, const std::vector<QString> &label,
		    const QString &binName, const StatsOperationResults);
	void append(double lowerBound, double upperBound, std::vector<std::pair<int, std::vector<QString>>> countLabels,
		    const QString &binName);

	// Get item under mous pointer, or -1 if none
	BarSeriesIndex getItemUnderMouse(const QPointF &f) const;
	static BarSeriesIndex invalidIndex();
	static bool isValidIndex(BarSeriesIndex);

	// Highlight item when hovering over item
	void highlight(BarSeriesIndex index, QPointF pos);
private:
	// A label that is composed of multiple lines
	struct BarLabel {
		std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> items;
		double totalWidth, totalHeight; // Size of the item
		BarLabel(QtCharts::QChart *chart, const std::vector<QString> &labels);
		void setVisible(bool visible);
		void updatePosition(QtCharts::QChart *chart, QtCharts::QAbstractSeries *series,
				    bool horizontal, bool center, const QRectF &rect);
	};

	struct SubItem {
		std::unique_ptr<QGraphicsRectItem> item;
		std::unique_ptr<BarLabel> label;
		double value_from;
		double value_to;
		int bin_nr;
		void updatePosition(QtCharts::QChart *chart, BarSeries *series, bool horizontal, bool stacked,
				    double from, double to);
		void highlight(bool highlight);
	};

	struct Item {
		double lowerBound, upperBound;
		std::vector<SubItem> subitems;
		QRectF rect;
		const QString binName;
		StatsOperationResults res;
		int total;
		Item(QtCharts::QChart *chart, BarSeries *series, double lowerBound, double upperBound,
		     std::vector<SubItem> subitems,
		     const QString &binName, const StatsOperationResults &res, int total, bool horizontal,
		     bool stacked, int binCount);
		void updatePosition(QtCharts::QChart *chart, BarSeries *series, bool horizontal, bool stacked, int binCount);
		void highlight(int subitem, bool highlight);
		int getSubItemUnderMouse(const QPointF &f, bool horizontal, bool stacked) const;
	};

	std::unique_ptr<InformationBox> information;
	std::vector<Item> items;
	std::vector<BarLabel> barLabels;
	bool horizontal;
	bool stacked;
	QString categoryName;
	const StatsType *valueType; // null: this is count based
	std::vector<QString> valueBinNames;
	BarSeriesIndex highlighted;
	std::vector<SubItem> makeSubItems(double value, const std::vector<QString> &label) const;
	std::vector<SubItem> makeSubItems(const std::vector<std::pair<double, std::vector<QString>>> &values) const;
	void add_item(double lowerBound, double upperBound, std::vector<SubItem> subitems,
		      const QString &binName, const StatsOperationResults &res, int total, bool horizontal,
		      bool stacked);
	std::vector<QString> makeInfo(const Item &item, int subitem) const;
	int binCount() const;
};

#endif
