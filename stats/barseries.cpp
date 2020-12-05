// SPDX-License-Identifier: GPL-2.0
#include "barseries.h"
#include "informationbox.h"
#include "statstranslations.h"
#include "statstypes.h"
#include <QChart>
#include <QLocale>

// Constants that control the bar layout
static const double barWidth = 0.8; // 1.0 = full width of category
static const QColor barColor(0x66, 0xb2, 0xff);
static const QColor barBorderColor(0x66, 0xb2, 0xff);
static const QColor barHighlightedColor(Qt::yellow);
static const QColor barHighlightedBorderColor(0xaa, 0xaa, 0x22);

BarSeries::BarSeries(bool horizontal, const QString &categoryName, const StatsType *valueType) :
	horizontal(horizontal), categoryName(categoryName), valueType(valueType), highlighted(-1)
{
}

BarSeries::~BarSeries()
{
}

BarSeries::BarLabel::BarLabel(const std::vector<QString> &labels,
			      double value, double height,
			      bool horizontal, 
			      QtCharts::QChart *chart, QtCharts::QAbstractSeries *series) :
	value(value), height(height),
	totalWidth(0.0), totalHeight(0.0)
{
	items.reserve(labels.size());
	for (const QString &label: labels) {
		items.emplace_back(new QGraphicsSimpleTextItem(chart));
		items.back()->setText(label);
		items.back()->setZValue(10.0); // ? What is a sensible value here ?
		QRectF rect = items.back()->boundingRect();
		if (rect.width() > totalWidth)
			totalWidth = rect.width();
		totalHeight += rect.height();
	}
	updatePosition(chart, series, horizontal);
}

void BarSeries::BarLabel::updatePosition(QtCharts::QChart *chart, QtCharts::QAbstractSeries *series, bool horizontal)
{
	if (!horizontal) {
		QPointF pos = chart->mapToPosition(QPointF(value, height), series);
		QPointF lowPos = chart->mapToPosition(QPointF(value, 0.0), series);
		// Attention: the lower position is at the bottom and therefore has the higher y-coordinate on the screen. Ugh.
		double barHeight = lowPos.y() - pos.y();
		// Heuristics: if the label fits nicely into the bar (bar height is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it on top of the bar.
		if (barHeight >= 2.0 * totalHeight)
			pos.ry() = lowPos.y() - (barHeight + totalHeight) / 2.0;
		else
			pos.ry() -= totalHeight + 2.0; // Leave two pixels(?) space
		for (auto &it: items) {
			QPointF itemPos = pos;
			QRectF rect = it->boundingRect();
			itemPos.rx() -= rect.width() / 2.0;
			it->setPos(itemPos);
			pos.ry() += rect.height();
		}
	} else {
		QPointF pos = chart->mapToPosition(QPointF(height, value), series);
		QPointF lowPos = chart->mapToPosition(QPointF(0.0, value), series);
		double barWidth = pos.x() - lowPos.x();
		// Heuristics: if the label fits nicely into the bar (bar width is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it to the right of the bar.
		if (barWidth >= 2.0 * totalWidth)
			pos.rx() = lowPos.x() + (barWidth - totalWidth) / 2.0;
		else
			pos.rx() += totalWidth / 2.0 + 2.0; // Leave two pixels(?) space
		pos.ry() -= totalHeight / 2.0;
		for (auto &it: items) {
			QPointF itemPos = pos;
			QRectF rect = it->boundingRect();
			itemPos.rx() -= rect.width() / 2.0;
			it->setPos(itemPos);
			pos.ry() += rect.height();
		}
	}
}

BarSeries::Item::Item(QtCharts::QChart *chart, BarSeries *series, double lowerBound, double upperBound, const QString &binName,
		      double value, const StatsOperationResults &res, int total, bool horizontal) :
	item(new QGraphicsRectItem(chart)),
	lowerBound(lowerBound),
	upperBound(upperBound),
	value(value),
	binName(binName),
	res(res),
	total(total)
{
	item->setZValue(5.0); // ? What is a sensible value here ?
	highlight(false);
	updatePosition(chart, series, horizontal);
}

void BarSeries::Item::highlight(bool highlight)
{
	if (highlight) {
		item->setBrush(QBrush(barHighlightedColor));
		item->setPen(QPen(barHighlightedBorderColor));
	} else {
		item->setBrush(QBrush(barColor));
		item->setPen(QPen(barBorderColor));
	}
}

void BarSeries::Item::updatePosition(QtCharts::QChart *chart, BarSeries *series, bool horizontal)
{
	double delta = (upperBound - lowerBound) * barWidth;
	double from = (lowerBound + upperBound - delta) / 2.0;
	double to = (lowerBound + upperBound + delta) / 2.0;

	QPointF topLeft, bottomRight;
	if (horizontal) {
		topLeft = chart->mapToPosition(QPointF(0.0, to), series);
		bottomRight = chart->mapToPosition(QPointF(value, from), series);
	} else {
		topLeft = chart->mapToPosition(QPointF(from, value), series);
		bottomRight = chart->mapToPosition(QPointF(to, 0.0), series);
	}
	item->setRect(QRectF(topLeft, bottomRight));
}

void BarSeries::append(double lowerBound, double upperBound, int count, const std::vector<QString> &label,
		       const QString &binName, int total)
{
	StatsOperationResults res;
	res.count = count;
	double value = count;
	items.emplace_back(chart(), this, lowerBound, upperBound, binName, value, res, total, horizontal);
	addLabel(lowerBound, upperBound, value, label);
}

void BarSeries::append(double lowerBound, double upperBound, double value, const std::vector<QString> &label,
		       const QString &binName, const StatsOperationResults res)
{
	items.emplace_back(chart(), this, lowerBound, upperBound, binName, value, res, -1, horizontal);
	addLabel(lowerBound, upperBound, value, label);
}

void BarSeries::addLabel(double lowerBound, double upperBound, double value, const std::vector<QString> &label)
{
	// Add label if provided
	if (!label.empty()) {
		double mid = (lowerBound + upperBound) / 2.0;
		barLabels.emplace_back(label, mid, value, horizontal, chart(), this);
	}
}

void BarSeries::updatePositions()
{
	QtCharts::QChart *c = chart();
	for (Item &item: items)
		item.updatePosition(c, this, horizontal);
	for (BarLabel &label: barLabels)
		label.updatePosition(c, this, horizontal);
}

// Attention: this supposes that items are sorted by position and no bar is inside another bar!
int BarSeries::getItemUnderMouse(const QPointF &point)
{
	// Search the first item whose "end" position is greater than the cursor position.
	auto it = horizontal ? std::lower_bound(items.begin(), items.end(), point.y(),
						[] (const Item &item, double y) { return item.item->rect().top() > y; })
			     : std::lower_bound(items.begin(), items.end(), point.x(),
						[] (const Item &item, double x) { return item.item->rect().right() < x; });
	return it != items.end() && it->item->rect().contains(point) ? it - items.begin() : -1;
}

// Format information in a count-based bar chart.
// Essentially, the name of the bin and the number and percentages of dives.
static std::vector<QString> makeCountInfo(const QString &binName, const QString &axisName, int count, int total)
{
	double percentage = count * 100.0 / total;
	QString countString = QString("%L1").arg(count);
	QString percentageString = QString("%L1%").arg(percentage, 0, 'f', 1);
	QString totalString = QString("%L1").arg(total);
	return {
		QStringLiteral("%1: %2").arg(axisName, binName),
		QStringLiteral("%1 (%2 of %3) dives").arg(countString, percentageString, totalString)
	};
}

// Format information in a value bar chart: the name of the bin and the value with unit.
static std::vector<QString> makeValueInfo(const QString &binName, const QString &axisName,
					  const StatsType &valueType, const StatsOperationResults &values)
{
	QLocale loc;
	int decimals = valueType.decimals();
	QString unit = valueType.unitSymbol();
	std::vector<StatsOperation> operations = valueType.supportedOperations();
	std::vector<QString> res;
	res.reserve(operations.size() + 3);
	res.push_back(QStringLiteral("%1: %2").arg(axisName, binName));
	res.push_back(QStringLiteral("%1: %2").arg(StatsTranslations::tr("Count"), loc.toString(values.count)));
	res.push_back(QStringLiteral("%1: ").arg(valueType.name()));
	for (StatsOperation op: operations) {
		QString valueFormatted = loc.toString(values.get(op), 'f', decimals);
		res.push_back(QString(" %1: %2 %3").arg(StatsType::operationName(op), valueFormatted, unit));
	}
	return res;
}

// Highlight item when hovering over item
void BarSeries::highlight(int index, QPointF pos)
{
	if (index == highlighted) {
		if (information)
			information->setPos(pos);
		return;
	}

	// Unhighlight old highlighted item (if any)
	if (highlighted >= 0 && highlighted < (int)items.size())
		items[highlighted].highlight(false);
	highlighted = index;

	// Highlight new item (if any)
	if (highlighted >= 0 && highlighted < (int)items.size()) {
		Item &item = items[highlighted];
		item.highlight(true);
		if (!information)
			information.reset(new InformationBox(chart()));
		std::vector<QString> info = valueType ?
			makeValueInfo(item.binName, categoryName, *valueType, item.res) :
			makeCountInfo(item.binName, categoryName, item.res.count, item.total);
		information->setText(info, pos);
	} else {
		information.reset();
	}
}
