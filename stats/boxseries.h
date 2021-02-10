// SPDX-License-Identifier: GPL-2.0
// A small custom box plot series, which displays quartiles.
// The QtCharts bar series were  too inflexible with respect
// to placement of the bars.
#ifndef BOX_SERIES_H
#define BOX_SERIES_H

#include "chartitem.h"
#include "statsseries.h"
#include "statsvariables.h" // for StatsQuartiles

#include <memory>
#include <vector>

struct InformationBox;

class BoxSeries : public StatsSeries {
public:
	BoxSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		  const QString &variable, const QString &unit, int decimals);
	~BoxSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;
	bool selectItemsUnderMouse(const QPointF &point, SelectionModifier modifier) override;

	// Note: this expects that all items are added with increasing pos
	// and that no bar is inside another bar, i.e. lowerBound and upperBound
	// are ordered identically.
	void append(double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName);

private:
	// Get item under mouse pointer, or -1 if none
	int getItemUnderMouse(const QPointF &f);

	struct Item {
		ChartItemPtr<ChartBoxItem> item;
		double lowerBound, upperBound;
		StatsQuartiles q;
		QString binName;
		bool selected;
		Item(StatsView &view, BoxSeries *series, double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName);
		~Item();
		void updatePosition(BoxSeries *series);
		void highlight(bool highlight);
	};

	QString variable, unit;
	int decimals;

	std::vector<QString> formatInformation(const Item &item) const;
	ChartItemPtr<InformationBox> information;
	std::vector<std::unique_ptr<Item>> items;
	int highlighted; // -1: no item highlighted
	int lastClicked; // -1: no item clicked
	void divesSelected(const QVector<dive *> &) override;
};

#endif
