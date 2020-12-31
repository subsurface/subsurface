// SPDX-License-Identifier: GPL-2.0
#include "boxseries.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statstranslations.h"
#include <QChart>
#include <QLocale>

// Constants that control the bar layout
static const double boxWidth = 0.8; // 1.0 = full width of category
static const int boxBorderWidth = 2;

BoxSeries::BoxSeries(QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis,
		     const QString &variable, const QString &unit, int decimals) :
	StatsSeries(chart, xAxis, yAxis),
	variable(variable), unit(unit), decimals(decimals), highlighted(-1)
{
}

BoxSeries::~BoxSeries()
{
}

BoxSeries::Item::Item(QtCharts::QChart *chart, BoxSeries *series, double lowerBound, double upperBound,
		      const StatsQuartiles &q, const QString &binName) :
	box(chart),
	topWhisker(chart), bottomWhisker(chart),
	topBar(chart), bottomBar(chart),
	center(chart),
	lowerBound(lowerBound), upperBound(upperBound), q(q),
	binName(binName)
{
	box.setZValue(5.0); // ? What is a sensible value here ?
	topWhisker.setZValue(5.0);
	bottomWhisker.setZValue(5.0);
	topBar.setZValue(5.0);
	bottomBar.setZValue(5.0);
	center.setZValue(5.0);
	highlight(false);
	updatePosition(chart, series);
}

BoxSeries::Item::~Item()
{
}

void BoxSeries::Item::highlight(bool highlight)
{
	QBrush brush = highlight ? QBrush(highlightedColor) : QBrush(fillColor);
	QPen pen = highlight ? QPen(highlightedBorderColor, boxBorderWidth) : QPen(::borderColor, boxBorderWidth);
	box.setBrush(brush);
	box.setPen(pen);
	topWhisker.setPen(pen);
	bottomWhisker.setPen(pen);
	topBar.setPen(pen);
	bottomBar.setPen(pen);
	center.setPen(pen);
}

void BoxSeries::Item::updatePosition(QtCharts::QChart *chart, BoxSeries *series)
{
	double delta = (upperBound - lowerBound) * boxWidth;
	double from = (lowerBound + upperBound - delta) / 2.0;
	double to = (lowerBound + upperBound + delta) / 2.0;
	double mid = (from + to) / 2.0;

	QPointF topLeft, bottomRight;
	QMarginsF margins(boxBorderWidth / 2.0, boxBorderWidth / 2.0, boxBorderWidth / 2.0, boxBorderWidth / 2.0);
	topLeft = chart->mapToPosition(QPointF(from, q.max), series);
	bottomRight = chart->mapToPosition(QPointF(to, q.min), series);
	bounding = QRectF(topLeft, bottomRight).marginsAdded(margins);
	double left = topLeft.x();
	double right = bottomRight.x();
	double width = right - left;
	double top = topLeft.y();
	double bottom = bottomRight.y();
	QPointF q1 = chart->mapToPosition(QPointF(mid, q.q1), series);
	QPointF q2 = chart->mapToPosition(QPointF(mid, q.q2), series);
	QPointF q3 = chart->mapToPosition(QPointF(mid, q.q3), series);
	box.setRect(left, q3.y(), width, q1.y() - q3.y());
	topWhisker.setLine(q3.x(), top, q3.x(), q3.y());
	bottomWhisker.setLine(q1.x(), q1.y(), q1.x(), bottom);
	topBar.setLine(left, top, right, top);
	bottomBar.setLine(left, bottom, right, bottom);
	center.setLine(left, q2.y(), right, q2.y());
}

void BoxSeries::append(double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName)
{
	QtCharts::QChart *c = chart();
	items.emplace_back(new Item(c, this, lowerBound, upperBound, q, binName));
}

void BoxSeries::updatePositions()
{
	QtCharts::QChart *c = chart();
	for (auto &item: items)
		item->updatePosition(c, this);
}

// Attention: this supposes that items are sorted by position and no box is inside another box!
int BoxSeries::getItemUnderMouse(const QPointF &point)
{
	// Search the first item whose "end" position is greater than the cursor position.
	auto it = std::lower_bound(items.begin(), items.end(), point.x(),
			   [] (const std::unique_ptr<Item> &item, double x) { return item->bounding.right() < x; });
	return it != items.end() && (*it)->bounding.contains(point) ? it - items.begin() : -1;
}

static QString infoItem(const QString &name, const QString &unit, int decimals, double value)
{
	QLocale loc;
	QString formattedValue = loc.toString(value, 'f', decimals);
	return unit.isEmpty() ? QStringLiteral(" %1: %2").arg(name, formattedValue)
			      : QStringLiteral(" %1: %2 %3").arg(name, formattedValue, unit);
}

std::vector<QString> BoxSeries::formatInformation(const Item &item) const
{
	QLocale loc;
	return {
		item.binName,
		QStringLiteral("%1:").arg(variable),
		infoItem(StatsTranslations::tr("min"), unit, decimals, item.q.min),
		infoItem(StatsTranslations::tr("Q1"), unit, decimals, item.q.q1),
		infoItem(StatsTranslations::tr("mean"), unit, decimals, item.q.q2),
		infoItem(StatsTranslations::tr("Q3"), unit, decimals, item.q.q3),
		infoItem(StatsTranslations::tr("max"), unit, decimals, item.q.max)
	};
}

// Highlight item when hovering over item
bool BoxSeries::hover(QPointF pos)
{
	int index = getItemUnderMouse(pos);
	if (index == highlighted) {
		if (information)
			information->setPos(pos);
		return index >= 0;
	}

	unhighlight();
	highlighted = index;

	// Highlight new item (if any)
	if (highlighted >= 0 && highlighted < (int)items.size()) {
		Item &item = *items[highlighted];
		item.highlight(true);
		if (!information)
			information.reset(new InformationBox(chart()));
		information->setText(formatInformation(item), pos);
	} else {
		information.reset();
	}
	return highlighted >= 0;
}

void BoxSeries::unhighlight()
{
	if (highlighted >= 0 && highlighted < (int)items.size())
		items[highlighted]->highlight(false);
	highlighted = -1;
}
