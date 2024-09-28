// SPDX-License-Identifier: GPL-2.0
// A small custom bar series, which displays information
// when hovering over a bar. The QtCharts bar series were
// too inflexible with respect to placement of the bars.
#ifndef BAR_SERIES_H
#define BAR_SERIES_H

#include "statsseries.h"
#include "statsvariables.h"
#include "qt-quick/chartitem_ptr.h"

#include <memory>
#include <vector>
#include <QColor>
#include <QRectF>

class ChartBarItem;
class ChartTextItem;
struct InformationBox;
struct StatsVariable;

class BarSeries : public StatsSeries {
public:
	// There are three versions of creating bar series: for value-based (mean, etc) charts, for counts
	// based charts and for stacked bar charts with multiple items.
	struct CountItem {
		double lowerBound, upperBound;
		std::vector<dive *> dives;
		std::vector<QString> label;
		QString binName;
		int total;
	};
	struct ValueItem {
		double lowerBound, upperBound;
		double value;
		std::vector<QString> label;
		QString binName;
		StatsOperationResults res;	// Contains the dives
	};
	struct MultiItem {
		struct Item {
			std::vector<dive *> dives;
			std::vector<QString> label;
		};
		double lowerBound, upperBound;
		std::vector<Item> items;
		QString binName;
	};

	// If the horizontal flag is true, independent variable is plotted on the y-axis.
	// A non-empty valueBinNames vector flags that this is a stacked bar chart.
	// In that case, a valueVariable must also be provided.
	// For count-based bar series in one variable, valueVariable is null.
	// Note: this expects that all items are added with increasing pos
	// and that no bar is inside another bar, i.e. lowerBound and upperBound
	// are ordered identically.
	BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, const QString &categoryName,
		  std::vector<CountItem> items);
	BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, const QString &categoryName, const StatsVariable *valueVariable,
		  std::vector<ValueItem> items);
	BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, bool stacked, const QString &categoryName, const StatsVariable *valueVariable,
		  std::vector<QString> valueBinNames,
		  std::vector<MultiItem> items);
	~BarSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;
	bool selectItemsUnderMouse(const QPointF &point, SelectionModifier modifier) override;

private:
	BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		  bool horizontal, bool stacked, const QString &categoryName, const StatsVariable *valueVariable,
		  std::vector<QString> valueBinNames);

	struct Index {
		int bar;
		int subitem;
		Index();
		Index(int bar, int subitem);
		bool operator==(const Index &i2) const;
		bool operator<=(const Index &i2) const;
	};
	void inc(Index &index);

	// Get item under mouse pointer, or -1 if none
	Index getItemUnderMouse(const QPointF &f) const;

	// A label that is composed of multiple lines
	struct BarLabel {
		ChartItemPtr<ChartTextItem> item;
		bool isOutside; // Is shown outside of bar
		BarLabel(StatsView &view, const std::vector<QString> &labels, int bin_nr, int binCount);
		void setVisible(bool visible);
		void updatePosition(bool horizontal, bool center, const QRectF &rect, int bin_nr, int binCount,
				    const QColor &background, const StatsTheme &theme);
		void highlight(bool highlight, int bin_nr, int binCount, const QColor &background, const StatsTheme &theme);
	};

	struct SubItem {
		ChartItemPtr<ChartBarItem> item;
		std::vector<dive *> dives;
		std::unique_ptr<BarLabel> label;
		double value_from;
		double value_to;
		int bin_nr;
		bool selected;
		QColor fill;
		void updatePosition(BarSeries *series, bool horizontal, bool stacked,
				    double from, double to, int binCount, const StatsTheme &theme);
		void highlight(bool highlight, int binCount, const StatsTheme &theme);
	};

	struct Item {
		double lowerBound, upperBound;
		std::vector<SubItem> subitems;
		QRectF rect;
		const QString binName;
		StatsOperationResults res;
		int total;
		Item(BarSeries *series, double lowerBound, double upperBound,
		     std::vector<SubItem> subitems,
		     const QString &binName, const StatsOperationResults &res, int total, bool horizontal,
		     bool stacked, int binCount,
		     const StatsTheme &theme);
		void updatePosition(BarSeries *series, bool horizontal, bool stacked, int binCount,
				    const StatsTheme &theme);
		void highlight(int subitem, bool highlight, int binCount, const StatsTheme &theme);
		int getSubItemUnderMouse(const QPointF &f, bool horizontal, bool stacked) const;
	};

	ChartItemPtr<InformationBox> information;
	std::vector<Item> items;
	bool horizontal;
	bool stacked;
	QString categoryName;
	const StatsVariable *valueVariable; // null: this is count based
	std::vector<QString> valueBinNames;
	Index highlighted;
	Index lastClicked;
	struct SubItemDesc {
		double v;
		std::vector<dive *> dives;
		std::vector<QString> label;
	};
	std::vector<SubItem> makeSubItems(SubItemDesc item) const;
	std::vector<SubItem> makeSubItems(std::vector<SubItemDesc> items) const;
	void add_item(double lowerBound, double upperBound, std::vector<SubItem> subitems,
		      const QString &binName, const StatsOperationResults &res, int total, bool horizontal,
		      bool stacked);
	std::vector<QString> makeInfo(const Item &item, int subitem) const;
	int binCount() const;
	void divesSelected(const QVector<dive *> &) override;
};

#endif
