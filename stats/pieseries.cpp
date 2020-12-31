// SPDX-License-Identifier: GPL-2.0
#include "pieseries.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statstranslations.h"

#include <numeric>
#include <math.h>
#include <QChart>
#include <QGraphicsEllipseItem>
#include <QLocale>

static const double pieSize = 0.9; // 1.0 = occupy full width of chart
static const double pieBorderWidth = 1.0;
static const double innerLabelRadius = 0.75; // 1.0 = at outer border of pie
static const double outerLabelRadius = 1.01; // 1.0 = at outer border of pie

PieSeries::Item::Item(QtCharts::QChart *chart, const QString &name, int from, int count, int totalCount, int bin_nr, bool labels) :
	item(new QGraphicsEllipseItem(chart)),
	name(name),
	count(count)
{
	QLocale loc;
	// For whatever obscure reason, angles in QGraphicsEllipseItem are given as 16th of a degree...?
	// Angles increase CCW, whereas pie charts usually are read CW.
	item->setStartAngle(90 * 16 - (from + count) * 360 * 16 / totalCount);
	item->setSpanAngle(count * 360 * 16 / totalCount);
	item->setPen(QPen(::borderColor));

	angleTo = static_cast<double>(from + count) / totalCount;
	double meanAngle = M_PI / 2.0 - (from + count / 2.0) / totalCount * M_PI * 2.0; // Note: "-" because we go CW.
	innerLabelPos = QPointF(cos(meanAngle) * innerLabelRadius, -sin(meanAngle) * innerLabelRadius);
	outerLabelPos = QPointF(cos(meanAngle) * outerLabelRadius, -sin(meanAngle) * outerLabelRadius);

	if (labels) {
		double percentage = count * 100.0 / totalCount;
		QString innerLabelText = QStringLiteral("%1\%").arg(loc.toString(percentage, 'f', 1));
		innerLabel.reset(new QGraphicsSimpleTextItem(innerLabelText, chart));
		innerLabel->setBrush(QBrush(labelColor));
		innerLabel->setZValue(10.0); // ? What is a sensible value here ?

		outerLabel.reset(new QGraphicsSimpleTextItem(name, chart));
		innerLabel->setBrush(QBrush(labelColor));
		innerLabel->setZValue(10.0); // ? What is a sensible value here ?
	}

	highlight(bin_nr, false);
}

void PieSeries::Item::updatePositions(const QRectF &rect, const QPointF &center, double radius)
{
	item->setRect(rect);
	if (innerLabel) {
		QRectF labelRect = innerLabel->boundingRect();
		innerLabel->setPos(center.x() + innerLabelPos.x() * radius - labelRect.width() / 2.0,
				   center.y() + innerLabelPos.y() * radius - labelRect.height() / 2.0);
	}
	if (outerLabel) {
		QRectF labelRect = outerLabel->boundingRect();
		QPointF pos(center.x() + outerLabelPos.x() * radius, center.y() + outerLabelPos.y() * radius);
		if (outerLabelPos.x() < 0.0) {
			if (outerLabelPos.y() < 0.0)
				pos -= QPointF(labelRect.width(), labelRect.height());
			else
				pos.rx() -= labelRect.width();
		} else if (outerLabelPos.y() < 0.0) {
			pos.ry() -= labelRect.height();
		}

		outerLabel->setPos(pos);
	}
}

void PieSeries::Item::highlight(int bin_nr, bool highlight)
{
	QBrush brush = highlight ? QBrush(highlightedColor) : QBrush(binColor(bin_nr));
	QPen pen = highlight ? QPen(highlightedBorderColor, pieBorderWidth) : QPen(::borderColor, pieBorderWidth);
	item->setBrush(brush);
	item->setPen(pen);
}

PieSeries::PieSeries(QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis, const QString &categoryName,
		     const std::vector<std::pair<QString, int>> &data, bool keepOrder, bool labels) :
	StatsSeries(chart, xAxis, yAxis),
	categoryName(categoryName),
	highlighted(-1)
{
	// Pie charts with many slices are unreadable. Therefore, subsume slices under
	// a certain percentage as "other". But draw a minimum number of slices
	// until we reach 50% so that we never get a pie only of "other".
	// This is heuristics, which might have to be optimized.
	const int smallest_slice_percentage = 5; // Smaller than 5% = others. That makes at most 20 slices.
	const int min_slices = 5; // Try to draw at least 5 slices until we reach 50%

	// Easier to read than std::accumulate
	totalCount = 0;
	for (const auto &[name, count]: data)
		totalCount += count;

	// First of all, sort from largest to smalles slice. Instead
	// of sorting the initial array, sort a list of indices, so that
	// the original order can be easily reconstructed later.
	std::vector<int> sorted(data.size());
	std::iota(sorted.begin(), sorted.end(), 0); // Fill with 0..size-1.
	// Two notes:
	//  - by negating the counts in the sort below, count is sorted descending.
	//  - do a lexicographic sort by (count, idx) so that for equal counts the order is preserved.
	std::sort(sorted.begin(), sorted.end(),
		  [&data](int idx1, int idx2)
		  { return std::make_tuple(-data[idx1].second, idx1) <
		  	   std::make_tuple(-data[idx2].second, idx2); });
	auto it = std::find_if(sorted.begin(), sorted.end(),
			       [count=totalCount, &data, smallest_slice_percentage](int idx)
			       { return data[idx].second * 100 / count < smallest_slice_percentage; });
	if (it - sorted.begin() < min_slices) {
		// Take minimum amount of slices below 50%...
		int sum = 0;
		for (auto it2 = sorted.begin(); it2 != it; ++it2)
			sum += data[*it2].second;

		while(it != sorted.end() && sum * 2 < totalCount && it - sorted.begin() < min_slices) {
			sum += data[*it].second;
			++it;
		}
	}

	// Don't do a single "other" group
	if (sorted.end() - it == 1)
		++it;

	// Sort the main groups and the other groups back, if requested
	if (keepOrder) {
		std::sort(sorted.begin(), it);
		std::sort(it, sorted.end());
	}

	items.reserve(it - sorted.begin() + 1); // +1 in case that there is an "other" group.
	int act = 0;
	for (auto it2 = sorted.begin(); it2 != it; ++it2) {
		int count = data[*it2].second;
		items.emplace_back(chart, data[*it2].first, act, count, totalCount, (int)items.size(), labels);
		act += count;
	}

	// Register the items of the "other" group.
	if (it != sorted.end()) {
		other.reserve(sorted.end() - it);
		for (auto it2 = it; it2 != sorted.end(); ++it2)
			other.push_back({ data[*it2].first, data[*it2].second });
		QString name = StatsTranslations::tr("other (%1 items)").arg(other.size());
		items.emplace_back(chart, name, act, totalCount - act, totalCount, (int)items.size(), labels);
	}
}

PieSeries::~PieSeries()
{
}

void PieSeries::updatePositions()
{
	QtCharts::QChart *c = chart();
	QRectF plotRect = c->plotArea();
	center = plotRect.center();
	radius = std::min(plotRect.width(), plotRect.height()) * pieSize / 2.0;
	QRectF rect(center.x() - radius, center.y() - radius, 2.0 * radius, 2.0 * radius);
	for (Item &item: items)
		item.updatePositions(rect, center, radius);
}

std::vector<QString> PieSeries::binNames()
{
	std::vector<QString> res;
	res.reserve(items.size());
	for (Item &item: items)
		res.push_back(item.name);
	return res;
}

int PieSeries::getItemUnderMouse(const QPointF &f) const
{
	QPointF delta = f - center;
	double len = sqrt(QPointF::dotProduct(delta, delta));
	if (len > radius)
		return -1;
	delta /= len;
	double angle = 0.25 - atan2(-delta.y(), delta.x()) / 2.0 / M_PI;
	while (angle < 0.0)
		angle += 1.0;
	auto it = std::lower_bound(items.begin(), items.end(), angle,
				   [](const Item &item, double angle) { return item.angleTo < angle; });
	if (it == items.end())
		return -1; // Floating point rounding issues?
	return it - items.begin();
}

static QString makePercentageLine(int count, int total)
{
	double percentage = count * 100.0 / total;
	QString countString = QString("%L1").arg(count);
	QString percentageString = QString("%L1%").arg(percentage, 0, 'f', 1);
	QString totalString = QString("%L1").arg(total);
	return StatsTranslations::tr("%1 (%2 of %3) dives").arg(countString, percentageString, totalString);
}

std::vector<QString> PieSeries::makeInfo(int idx) const
{
	std::vector<QString> res;
	if (idx + 1 == (int)items.size() && !other.empty()) {
		// This is the "other" bin. Format all these items and an overview item.
		res.reserve(other.size() + 1);
		res.push_back(QString("%1: %2").arg(StatsTranslations::tr("other"),
						    makePercentageLine(items[idx].count, totalCount)));
		for (const OtherItem &item: other)
			res.push_back(QString("%1: %2").arg(item.name,
							    makePercentageLine(item.count, totalCount)));
	} else {
		// A "normal" item.
		res.reserve(2);
		res.push_back(QStringLiteral("%1: %2").arg(categoryName, items[idx].name));
		res.push_back(makePercentageLine(items[idx].count, totalCount));
	}
	return res;
}

bool PieSeries::hover(QPointF pos)
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
		items[highlighted].highlight(highlighted, true);
		if (!information)
			information.reset(new InformationBox(chart()));
		information->setText(makeInfo(highlighted), pos);
	} else {
		information.reset();
	}
	return highlighted >= 0;
}

void PieSeries::unhighlight()
{
	if (highlighted >= 0 && highlighted < (int)items.size())
		items[highlighted].highlight(highlighted, false);
	highlighted = -1;
}
