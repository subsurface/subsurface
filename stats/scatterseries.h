// SPDX-License-Identifier: GPL-2.0
// A small custom scatter series, where every item represents a dive
// The original QScatterSeries was buggy and distinctly slower
#ifndef SCATTER_SERIES_H
#define SCATTER_SERIES_H

#include "statshelper.h"
#include "statsseries.h"

#include <memory>
#include <vector>

class ChartScatterItem;
struct InformationBox;
struct StatsVariable;
struct dive;

class ScatterSeries : public StatsSeries {
public:
	ScatterSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		      const StatsVariable &varX, const StatsVariable &varY);
	~ScatterSeries();

	void updatePositions() override;
	bool hover(QPointF pos) override;
	void unhighlight() override;

	// Note: this expects that all items are added with increasing pos!
	void append(dive *d, double pos, double value);
	bool selectItemsUnderMouse(const QPointF &point, SelectionModifier modifier) override;
	bool supportsLassoSelection() const override;
	void selectItemsInRect(const QRectF &rect, SelectionModifier modifier, const std::vector<dive *> &oldSelection) override;

private:
	// Get items under mouse.
	std::vector<int> getItemsUnderMouse(const QPointF &f) const;
	std::vector<int> getItemsInRect(const QRectF &f) const;

	struct Item {
		ChartItemPtr<ChartScatterItem> item;
		dive *d;
		bool selected;
		double pos, value;
		Item(StatsView &view, ScatterSeries *series, const StatsTheme &theme, dive *d, double pos, double value);
		void updatePosition(ScatterSeries *series);
		void highlight(bool highlight);
	};

	ChartItemPtr<InformationBox> information;
	std::vector<Item> items;
	std::vector<int> highlighted;
	const StatsVariable &varX;
	const StatsVariable &varY;
	void divesSelected(const QVector<dive *> &) override;
};

#endif
