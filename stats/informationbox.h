// SPDX-License-Identifier: GPL-2.0
// A small box displaying statistics information, notably
// for a scatter-plot item or a bar in a bar chart.
#ifndef INFORMATION_BOX_H
#define INFORMATION_BOX_H

#include <vector>
#include <memory>
#include <QGraphicsRectItem>

namespace QtCharts {
	class QChart;
}
struct dive;

// Information window showing data of highlighted dive
struct InformationBox : QGraphicsRectItem {
	InformationBox(QtCharts::QChart *chart);
	void setText(const std::vector<QString> &text, QPointF pos);
	void setPos(QPointF pos);
private:
	QtCharts::QChart *chart;
	double width, height;
	void addLine(const QString &s);
	std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> textItems;
};

#endif
