// SPDX-License-Identifier: GPL-2.0
// The background grid of a chart

#include <memory>
#include <vector>
#include <QVector>
#include <QGraphicsLineItem>

class StatsAxis;
class QGraphicsScene;

class StatsGrid  {
public:
	StatsGrid(QGraphicsScene *scene, const StatsAxis &xAxis, const StatsAxis &yAxis);
	void updatePositions();
private:
	QGraphicsScene *scene;
	const StatsAxis &xAxis, &yAxis;
	std::vector<std::unique_ptr<QGraphicsLineItem>> lines;
};
