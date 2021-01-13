// SPDX-License-Identifier: GPL-2.0
// A small box displaying statistics information, notably
// for a scatter-plot item or a bar in a bar chart.
#ifndef INFORMATION_BOX_H
#define INFORMATION_BOX_H

#include "chartitem.h"

#include <vector>
#include <memory>
#include <QFont>

struct dive;
class StatsView;

// Information window showing data of highlighted dive
struct InformationBox : ChartRectItem {
	InformationBox(StatsView &);
	void setText(const std::vector<QString> &text, QPointF pos);
	void setPos(QPointF pos);
	int recommendedMaxLines() const;
private:
	QFont font; // For future specialization.
	double width, height;
};

#endif
