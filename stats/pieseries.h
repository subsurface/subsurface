// SPDX-License-Identifier: GPL-2.0
// A pie chart series, which displays percentual information.
#ifndef PIE_SERIES_H
#define PIE_SERIES_H

#include "statsseries.h"
#include "qt-quick/chartitem_ptr.h"

#include <memory>
#include <vector>
#include <QString>

struct dive;
struct InformationBox;
class ChartPieItem;
class ChartTextItem;
class QRectF;
enum class ChartSortMode : int;

class PieSeries : public StatsSeries {
public:
	// The pie series is initialized with (name, count) pairs.
	// If keepOrder is false, bins will be sorted by size, otherwise the sorting
	// of the shown bins will be retained. Small bins are omitted for clarity.
	PieSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis, const QString &categoryName,
		  std::vector<std::pair<QString, std::vector<dive *>>> data, ChartSortMode sortMode);
	~PieSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;
	bool selectItemsUnderMouse(const QPointF &point, SelectionModifier modifier) override;

	std::vector<QString> binNames();

private:
	// Get item under mouse pointer, or -1 if none
	int getItemUnderMouse(const QPointF &f) const;

	ChartItemPtr<ChartPieItem> item;
	QString categoryName;
	std::vector<QString> makeInfo(int idx) const;

	struct Item {
		ChartItemPtr<ChartTextItem> innerLabel, outerLabel;
		QString name;
		double angleFrom, angleTo; // In fraction of total
		std::vector<dive *> dives;
		QPointF innerLabelPos, outerLabelPos; // With respect to a (-1, -1)-(1, 1) rectangle.
		bool selected;
		Item(StatsView &view, const QString &name, int from, std::vector<dive *> dives, int totalCount,
		     int bin_nr, int numBins, const StatsTheme &theme);
		void updatePositions(const QPointF &center, double radius);
		void highlight(ChartPieItem &item, int bin_nr, bool highlight, int numBins, const StatsTheme &theme);
	};
	std::vector<Item> items;
	int totalCount;

	// Entries in the "other" group. If empty, there is no "other" group.
	struct OtherItem {
		QString name;
		int count;
	};
	std::vector<OtherItem> other;

	ChartItemPtr<InformationBox> information;
	QPointF center; // center of drawing area
	double radius; // radius of pie
	int highlighted; // -1: no item highlighted
	int lastClicked; // -1: no item clicked
	void divesSelected(const QVector<dive *> &) override;
};

#endif
