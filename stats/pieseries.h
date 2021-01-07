// SPDX-License-Identifier: GPL-2.0
// A pie chart series, which displays percentual information.
#ifndef PIE_SERIES_H
#define PIE_SERIES_H

#include "statsseries.h"

#include <memory>
#include <vector>
#include <QString>

class InformationBox;
class QGraphicsEllipseItem;
class QGraphicsScene;
class QGraphicsSimpleTextItem;
class QRectF;

class PieSeries : public StatsSeries {
public:
	// The pie series is initialized with (name, count) pairs.
	// If keepOrder is false, bins will be sorted by size, otherwise the sorting
	// of the shown bins will be retained. Small bins are omitted for clarity.
	PieSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis, const QString &categoryName,
		  const std::vector<std::pair<QString, int>> &data, bool keepOrder, bool labels);
	~PieSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;

	std::vector<QString> binNames();

private:
	// Get item under mouse pointer, or -1 if none
	int getItemUnderMouse(const QPointF &f) const;

	QString categoryName;
	std::vector<QString> makeInfo(int idx) const;

	struct Item {
		std::unique_ptr<QGraphicsEllipseItem> item;
		std::unique_ptr<QGraphicsSimpleTextItem> innerLabel, outerLabel;
		QString name;
		double angleTo; // In fraction of total
		int count;
		QPointF innerLabelPos, outerLabelPos; // With respect to a (-1, -1)-(1, 1) rectangle.
		Item(QGraphicsScene *scene, const QString &name, int from, int count, int totalCount,
		     int bin_nr, int numBins, bool labels);
		void updatePositions(const QRectF &rect, const QPointF &center, double radius);
		void highlight(int bin_nr, bool highlight, int numBins);
	};
	std::vector<Item> items;
	int totalCount;

	// Entries in the "other" group. If empty, there is no "other" group.
	struct OtherItem {
		QString name;
		int count;
	};
	std::vector<OtherItem> other;

	std::unique_ptr<InformationBox> information;
	QPointF center; // center of drawing area
	double radius; // radius of pie
	int highlighted;
};

#endif
