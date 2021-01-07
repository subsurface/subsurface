// SPDX-License-Identifier: GPL-2.0
// A small custom box plot series, which displays quartiles.
// The QtCharts bar series were  too inflexible with respect
// to placement of the bars.
#ifndef BOX_SERIES_H
#define BOX_SERIES_H

#include "statsseries.h"
#include "statsvariables.h" // for StatsQuartiles

#include <memory>
#include <vector>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>

class InformationBox;
class QGraphicsScene;

class BoxSeries : public StatsSeries {
public:
	BoxSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		  const QString &variable, const QString &unit, int decimals);
	~BoxSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;

	// Note: this expects that all items are added with increasing pos
	// and that no bar is inside another bar, i.e. lowerBound and upperBound
	// are ordered identically.
	void append(double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName);

private:
	// Get item under mouse pointer, or -1 if none
	int getItemUnderMouse(const QPointF &f);

	struct Item {
		QGraphicsRectItem box;
		QGraphicsLineItem topWhisker, bottomWhisker;
		QGraphicsLineItem topBar, bottomBar;
		QGraphicsLineItem center;
		QRectF bounding; // bounding box in screen coordinates
		~Item();
		double lowerBound, upperBound;
		StatsQuartiles q;
		QString binName;
		Item(QGraphicsScene *scene, BoxSeries *series, double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName);
		void updatePosition(BoxSeries *series);
		void highlight(bool highlight);
	};

	QString variable, unit;
	int decimals;

	std::vector<QString> formatInformation(const Item &item) const;
	std::unique_ptr<InformationBox> information;
	std::vector<std::unique_ptr<Item>> items;
	int highlighted; // -1: no item highlighted
};

#endif
