// SPDX-License-Identifier: GPL-2.0
// A pie chart series, which displays percentual information.
#ifndef PIE_SERIES_H
#define PIE_SERIES_H

#include "statshelper.h"
#include "statsseries.h"

#include <memory>
#include <vector>
#include <QString>

struct InformationBox;
struct ChartPieItem;
struct ChartTextItem;
class QRectF;

class PieSeries : public StatsSeries {
public:
	// The pie series is initialized with (name, count) pairs.
	// If keepOrder is false, bins will be sorted by size, otherwise the sorting
	// of the shown bins will be retained. Small bins are omitted for clarity.
	PieSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis, const QString &categoryName,
		  const std::vector<std::pair<QString, int>> &data, bool keepOrder, bool labels);
	~PieSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;

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
		int count;
		QPointF innerLabelPos, outerLabelPos; // With respect to a (-1, -1)-(1, 1) rectangle.
		Item(StatsView &view, const QString &name, int from, int count, int totalCount,
		     int bin_nr, int numBins, bool labels);
		void updatePositions(const QPointF &center, double radius);
		void highlight(ChartPieItem &item, int bin_nr, bool highlight, int numBins);
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
	int highlighted;
};

#endif
