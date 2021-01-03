// SPDX-License-Identifier: GPL-2.0
// A small box displaying statistics information, notably
// for a scatter-plot item or a bar in a bar chart.
#ifndef INFORMATION_BOX_H
#define INFORMATION_BOX_H

#include "backend-shared/roundrectitem.h"

#include <vector>
#include <memory>
#include <QFont>

namespace QtCharts {
	class QChart;
}
struct dive;

// Information window showing data of highlighted dive
struct InformationBox : RoundRectItem {
	InformationBox(QtCharts::QChart *chart);
	void setText(const std::vector<QString> &text, QPointF pos);
	void setPos(QPointF pos);
	int recommendedMaxLines() const;
private:
	QtCharts::QChart *chart;
	QFont font; // For future specialization.
	double width, height;
	void addLine(const QString &s);
	std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> textItems;
};

#endif
