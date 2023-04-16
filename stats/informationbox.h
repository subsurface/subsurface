// SPDX-License-Identifier: GPL-2.0
// A small box displaying statistics information, notably
// for a scatter-plot item or a bar in a bar chart.
#ifndef INFORMATION_BOX_H
#define INFORMATION_BOX_H

#include "chartitem.h"

#include <vector>
#include <memory>

struct dive;
class ChartView;
class StatsTheme;

// Information window showing data of highlighted dive
struct InformationBox : ChartRectItem {
	InformationBox(ChartView &, const StatsTheme &theme);
	void setText(const std::vector<QString> &text, QPointF pos);
	void setPos(QPointF pos);
	int recommendedMaxLines() const;
private:
	const StatsTheme &theme; // Set once in constructor.
	double width, height;
};

#endif
