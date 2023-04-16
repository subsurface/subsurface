// SPDX-License-Identifier: GPL-2.0
// The background grid of a chart

#include "qt-quick/chartitem_ptr.h"

#include <memory>
#include <vector>

class StatsAxis;
class StatsTheme;
class StatsView;
class ChartLineItem;

class StatsGrid  {
public:
	StatsGrid(StatsView &view, const StatsAxis &xAxis, const StatsAxis &yAxis);
	void updatePositions();
private:
	StatsView &view;
	const StatsTheme &theme; // Initialized once in constructor.
	const StatsAxis &xAxis, &yAxis;
	std::vector<ChartItemPtr<ChartLineItem>> lines;
};
