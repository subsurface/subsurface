// SPDX-License-Identifier: GPL-2.0
// A small custom bar series, which displays information
// when hovering over a bar. The QtCharts bar series were
// too inflexible with respect to placement of the bars.
#ifndef BAR_SERIES_H
#define BAR_SERIES_H

#include "statsseries.h"
#include "statsvariables.h"

#include <memory>
#include <vector>
#include <QGraphicsRectItem>

class QGraphicsScene;
class InformationBox;
class StatsVariable;

class BarSeries : public StatsSeries {
public:
	// There are three versions of creating bar series: for value-based (mean, etc) charts, for counts
	// based charts and for stacked bar charts with multiple items.
	struct CountItem {
		double lowerBound, upperBound;
		int count;
		std::vector<QString> label;
		QString binName;
		int total;
	};
	struct ValueItem {
		double lowerBound, upperBound;
		double value;
		std::vector<QString> label;
		QString binName;
		StatsOperationResults res;
	};
	struct MultiItem {
		double lowerBound, upperBound;
		std::vector<std::pair<int, std::vector<QString>>> countLabels;
		QString binName;
	};

	// If the horizontal flag is true, independent variable is plotted on the y-axis.
	// A non-empty valueBinNames vector flags that this is a stacked bar chart.
	// In that case, a valueVariable must also be provided.
	// For count-based bar series in one variable, valueVariable is null.
	// Note: this expects that all items are added with increasing pos
	// and that no bar is inside another bar, i.e. lowerBound and upperBound
	// are ordered identically.
	BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, const QString &categoryName,
		  const std::vector<CountItem> &items);
	BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, const QString &categoryName, const StatsVariable *valueVariable,
		  const std::vector<ValueItem> &items);
	BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, bool stacked, const QString &categoryName, const StatsVariable *valueVariable,
		  std::vector<QString> valueBinNames,
		  const std::vector<MultiItem> &items);
	~BarSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;
private:
	BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, bool stacked, const QString &categoryName, const StatsVariable *valueVariable,
		  std::vector<QString> valueBinNames);

	struct Index {
		int bar;
		int subitem;
		Index();
		Index(int bar, int subitem);
		bool operator==(const Index &i2) const;
	};

	// Get item under mouse pointer, or -1 if none
	Index getItemUnderMouse(const QPointF &f) const;

	// A label that is composed of multiple lines
	struct BarLabel {
		std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> items;
		double totalWidth, totalHeight; // Size of the item
		bool isOutside; // Is shown outside of bar
		BarLabel(QGraphicsScene *scene, const std::vector<QString> &labels, int bin_nr, int binCount);
		void setVisible(bool visible);
		void updatePosition(bool horizontal, bool center, const QRectF &rect, int bin_nr, int binCount);
		void highlight(bool highlight, int bin_nr, int binCount);
	};

	struct SubItem {
		std::unique_ptr<QGraphicsRectItem> item;
		std::unique_ptr<BarLabel> label;
		double value_from;
		double value_to;
		int bin_nr;
		void updatePosition(BarSeries *series, bool horizontal, bool stacked,
				    double from, double to, int binCount);
		void highlight(bool highlight, int binCount);
	};

	struct Item {
		double lowerBound, upperBound;
		std::vector<SubItem> subitems;
		QRectF rect;
		const QString binName;
		StatsOperationResults res;
		int total;
		Item(QGraphicsScene *scene, BarSeries *series, double lowerBound, double upperBound,
		     std::vector<SubItem> subitems,
		     const QString &binName, const StatsOperationResults &res, int total, bool horizontal,
		     bool stacked, int binCount);
		void updatePosition(BarSeries *series, bool horizontal, bool stacked, int binCount);
		void highlight(int subitem, bool highlight, int binCount);
		int getSubItemUnderMouse(const QPointF &f, bool horizontal, bool stacked) const;
	};

	std::unique_ptr<InformationBox> information;
	std::vector<Item> items;
	std::vector<BarLabel> barLabels;
	bool horizontal;
	bool stacked;
	QString categoryName;
	const StatsVariable *valueVariable; // null: this is count based
	std::vector<QString> valueBinNames;
	Index highlighted;
	std::vector<SubItem> makeSubItems(double value, const std::vector<QString> &label) const;
	std::vector<SubItem> makeSubItems(const std::vector<std::pair<double, std::vector<QString>>> &values) const;
	void add_item(double lowerBound, double upperBound, std::vector<SubItem> subitems,
		      const QString &binName, const StatsOperationResults &res, int total, bool horizontal,
		      bool stacked);
	std::vector<QString> makeInfo(const Item &item, int subitem) const;
	int binCount() const;
};

#endif
