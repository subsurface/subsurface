// SPDX-License-Identifier: GPL-2.0
// The background grid of a chart

#include <memory>
#include <vector>
#include <QVector>
#include <QGraphicsLineItem>

class StatsAxis;
namespace QtCharts {
	class QChart;
};

class StatsGrid  {
public:
	StatsGrid(QtCharts::QChart *chart, const StatsAxis &xAxis, const StatsAxis &yAxis);
	void updatePositions();
private:
	QtCharts::QChart *chart;
	const StatsAxis &xAxis, &yAxis;
	std::vector<std::unique_ptr<QGraphicsLineItem>> lines;
};
