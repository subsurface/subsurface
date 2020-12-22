// SPDX-License-Identifier: GPL-2.0
#include "barseries.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statstranslations.h"
#include "statstypes.h"
#include <math.h> // for lrint()
#include <QChart>
#include <QLocale>

// Constants that control the bar layout
static const double barWidth = 0.8; // 1.0 = full width of category
static const double subBarWidth = 0.9; // For grouped bar charts

BarSeriesIndex BarSeries::invalidIndex()
{
	return { -1, -1};
}

bool BarSeries::isValidIndex(BarSeriesIndex idx)
{
	return idx.bar >= 0;
}

static bool operator==(const BarSeriesIndex &i1, const BarSeriesIndex &i2)
{
	return std::tie(i1.bar, i1.subitem) == std::tie(i2.bar, i2.subitem);
}

BarSeries::BarSeries(bool horizontal, bool stacked, const QString &categoryName,
		     const StatsType *valueType, std::vector<QString> valueBinNames) :
	horizontal(horizontal), stacked(stacked), categoryName(categoryName),
	valueType(valueType), valueBinNames(std::move(valueBinNames)), highlighted(invalidIndex())
{
}

BarSeries::~BarSeries()
{
}

BarSeries::BarLabel::BarLabel(QtCharts::QChart *chart, const std::vector<QString> &labels) :
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
}

void BarSeries::BarLabel::setVisible(bool visible)
{
	for (auto &item: items)
		item->setVisible(visible);
}

void BarSeries::BarLabel::updatePosition(QtCharts::QChart *chart, QtCharts::QAbstractSeries *series,
					 bool horizontal, bool center, const QRectF &rect)
{
	if (!horizontal) {
		if (totalWidth > rect.width()) {
			setVisible(false);
			return;
		}
		QPointF pos = rect.center();

		// Heuristics: if the label fits nicely into the bar (bar height is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it at the top of the bar.
		if (!center && rect.height() < 2.0 * totalHeight) {
			pos.ry() = rect.top() - (totalHeight + 2.0); // Leave two pixels(?) space
		} else {
			if (totalHeight > rect.height()) {
				setVisible(false);
				return;
			}
			pos.ry() -= totalHeight / 2.0;
		}
		for (auto &it: items) {
			QPointF itemPos = pos;
			QRectF rect = it->boundingRect();
			itemPos.rx() -= rect.width() / 2.0;
			it->setPos(itemPos);
			pos.ry() += rect.height();
		}
	} else {
		if (totalHeight > rect.height()) {
			setVisible(false);
			return;
		}
		QPointF pos = rect.center();
		pos.ry() -= totalHeight / 2.0;

		// Heuristics: if the label fits nicely into the bar (bar width is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it to the right of the bar.
		if (!center && rect.width() < 2.0 * totalWidth) {
			pos.rx() = rect.right() + (totalWidth / 2.0 + 2.0); // Leave two pixels(?) space
		} else {
			if (totalWidth > rect.width()) {
				setVisible(false);
				return;
			}
		}
		for (auto &it: items) {
			QPointF itemPos = pos;
			QRectF rect = it->boundingRect();
			itemPos.rx() -= rect.width() / 2.0;
			it->setPos(itemPos);
			pos.ry() += rect.height();
		}
	}
	setVisible(true);
}

BarSeries::Item::Item(QtCharts::QChart *chart, BarSeries *series, double lowerBound, double upperBound,
		      std::vector<SubItem> subitemsIn,
		      const QString &binName, const StatsOperationResults &res, int total,
		      bool horizontal, bool stacked, int binCount) :
	lowerBound(lowerBound),
	upperBound(upperBound),
	subitems(std::move(subitemsIn)),
	binName(binName),
	res(res),
	total(total)
{
	for (SubItem &item: subitems) {
		item.item->setZValue(5.0); // ? What is a sensible value here ?
		item.highlight(false);
	}
	updatePosition(chart, series, horizontal, stacked, binCount);
}

void BarSeries::Item::highlight(int subitem, bool highlight)
{
	if (subitem < 0 || subitem >= (int)subitems.size())
		return;
	subitems[subitem].highlight(highlight);
}

void BarSeries::SubItem::highlight(bool highlight)
{
	if (highlight) {
		item->setBrush(QBrush(highlightedColor));
		item->setPen(QPen(highlightedBorderColor));
	} else {
		item->setBrush(QBrush(binColor(bin_nr)));
		item->setPen(QPen(::borderColor));
	}
}

void BarSeries::Item::updatePosition(QtCharts::QChart *chart, BarSeries *series, bool horizontal, bool stacked, int binCount)
{
	if (subitems.empty())
		return;

	int num = stacked ? 1 : binCount;

	// barWidth gives the total width of the rod or group of rods.
	// subBarWidth gives the the width of each bar in the case of grouped bar charts.
	// calculated the group width such that after applying the latter, the former is obtained.
	double groupWidth = (upperBound - lowerBound) * barWidth * num / (num - 1 + subBarWidth);
	double from = (lowerBound + upperBound - groupWidth) / 2.0;
	double to = (lowerBound + upperBound + groupWidth) / 2.0;

	double fullSubWidth = (to - from) / num; // width including gap
	double subWidth = fullSubWidth * subBarWidth; // width without gap
	for (SubItem &item: subitems) {
		int idx = stacked ? 0 : item.bin_nr;
		double center = (idx + 0.5) * fullSubWidth + from;
		item.updatePosition(chart, series, horizontal, stacked, center - subWidth / 2.0, center + subWidth / 2.0);
	}
	rect = subitems[0].item->rect();
	for (auto it = std::next(subitems.begin()); it != subitems.end(); ++it)
		rect = rect.united(it->item->rect());
}

void BarSeries::SubItem::updatePosition(QtCharts::QChart *chart, BarSeries *series, bool horizontal, bool stacked,
					double from, double to)
{
	QPointF topLeft, bottomRight;
	if (horizontal) {
		topLeft = chart->mapToPosition(QPointF(value_from, to), series);
		bottomRight = chart->mapToPosition(QPointF(value_to, from), series);
	} else {
		topLeft = chart->mapToPosition(QPointF(from, value_to), series);
		bottomRight = chart->mapToPosition(QPointF(to, value_from), series);
	}
	QRectF rect(topLeft, bottomRight);
	item->setRect(rect);
	if (label)
		label->updatePosition(chart, series, horizontal, stacked, rect);
}

std::vector<BarSeries::SubItem> BarSeries::makeSubItems(const std::vector<std::pair<double, std::vector<QString>>> &values) const
{
	std::vector<SubItem> res;
	res.reserve(values.size());
	double from = 0.0;
	int bin_nr = 0;
	for (auto &[v, label]: values) {
		if (v > 0.0) {
			res.push_back({ std::make_unique<QGraphicsRectItem>(chart()), {}, from, from + v, bin_nr });
			if (!label.empty())
				res.back().label = std::make_unique<BarLabel>(chart(), label);
		}
		if (stacked)
			from += v;
		++bin_nr;
	}
	return res;
}

std::vector<BarSeries::SubItem> BarSeries::makeSubItems(double value, const std::vector<QString> &label) const
{
	return makeSubItems(std::vector<std::pair<double, std::vector<QString>>>{ { value, label } });
}

int BarSeries::binCount() const
{
	return std::max(1, (int)valueBinNames.size());
}

void BarSeries::add_item(double lowerBound, double upperBound, std::vector<SubItem> subitems,
			 const QString &binName, const StatsOperationResults &res, int total, bool horizontal,
			 bool stacked)
{
	// Don't add empty items, as that messes with the "find item under mouse" routine.
	if (subitems.empty())
		return;
	items.emplace_back(chart(), this, lowerBound, upperBound, std::move(subitems), binName, res,
			   total, horizontal, stacked, binCount());
}

void BarSeries::append(double lowerBound, double upperBound, int count, const std::vector<QString> &label,
		       const QString &binName, int total)
{
	StatsOperationResults res;
	res.count = count;
	double value = count;
	add_item(lowerBound, upperBound, makeSubItems(value, label),
		 binName, res, total, horizontal, stacked);
}

void BarSeries::append(double lowerBound, double upperBound, double value, const std::vector<QString> &label,
		       const QString &binName, const StatsOperationResults res)
{
	add_item(lowerBound, upperBound, makeSubItems(value, label),
		 binName, res, -1, horizontal, stacked);
}

void BarSeries::append(double lowerBound, double upperBound,
		       std::vector<std::pair<int, std::vector<QString>>> countLabels, const QString &binName)
{
	StatsOperationResults res;
	std::vector<std::pair<double, std::vector<QString>>> valuesLabels;
	valuesLabels.reserve(countLabels.size());
	int total = 0;
	for (auto &[count, label]: countLabels) {
		valuesLabels.push_back({ static_cast<double>(count), std::move(label) });
		total += count;
	}
	add_item(lowerBound, upperBound, makeSubItems(valuesLabels),
		 binName, res, total, horizontal, stacked);
}

void BarSeries::updatePositions()
{
	QtCharts::QChart *c = chart();
	for (Item &item: items)
		item.updatePosition(c, this, horizontal, stacked, binCount());
}

// Attention: this supposes that items are sorted by position and no bar is inside another bar!
BarSeriesIndex BarSeries::getItemUnderMouse(const QPointF &point) const
{
	// Search the first item whose "end" position is greater than the cursor position.
	auto it = horizontal ? std::lower_bound(items.begin(), items.end(), point.y(),
						[] (const Item &item, double y) { return item.rect.top() > y; })
			     : std::lower_bound(items.begin(), items.end(), point.x(),
						[] (const Item &item, double x) { return item.rect.right() < x; });
	if (it == items.end() || !it->rect.contains(point))
		return invalidIndex();
	int subitem = it->getSubItemUnderMouse(point, horizontal, stacked);
	return subitem >= 0 ? BarSeriesIndex{(int)(it - items.begin()), subitem} : invalidIndex();
}

// Attention: this supposes that sub items are sorted by position and no subitem is inside another bar!
int BarSeries::Item::getSubItemUnderMouse(const QPointF &point, bool horizontal, bool stacked) const
{
	// Search the first item whose "end" position is greater than the cursor position.
	bool search_x = horizontal == stacked;
	auto it = search_x ? std::lower_bound(subitems.begin(), subitems.end(), point.x(),
					      [] (const SubItem &item, double x) { return item.item->rect().right() < x; })
			   : std::lower_bound(subitems.begin(), subitems.end(), point.y(),
					      [] (const SubItem &item, double y) { return item.item->rect().top() > y; });
	return it != subitems.end() && it->item->rect().contains(point) ? it - subitems.begin() : -1;
}

// Format information in a count-based bar chart.
// Essentially, the name of the bin and the number and percentages of dives.
static std::vector<QString> makeCountInfo(const QString &binName, const QString &axisName,
					  const QString &valueBinName, const QString &valueAxisName,
					  int count, int total)
{
	double percentage = count * 100.0 / total;
	QString countString = QString("%L1").arg(count);
	QString percentageString = QString("%L1%").arg(percentage, 0, 'f', 1);
	QString totalString = QString("%L1").arg(total);
	std::vector<QString> res;
	res.reserve(3);
	res.push_back(QStringLiteral("%1: %2").arg(axisName, binName));
	if (!valueAxisName.isEmpty())
		res.push_back(QStringLiteral("%1: %2").arg(valueAxisName, valueBinName));
	res.push_back(QStringLiteral("%1 (%2 of %3) dives").arg(countString, percentageString, totalString));
	return res;
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

std::vector<QString> BarSeries::makeInfo(const Item &item, int subitem_idx) const
{
	if (!valueBinNames.empty() && valueType) {
		if (subitem_idx < 0 || subitem_idx >= (int)item.subitems.size())
			return {};
		const SubItem &subitem = item.subitems[subitem_idx];
		if (subitem.bin_nr < 0 || subitem.bin_nr >= (int)valueBinNames.size())
			return {};
		int count = (int)lrint(subitem.value_to - subitem.value_from);
		return makeCountInfo(item.binName, categoryName, valueBinNames[subitem.bin_nr],
				     valueType->name(), count, item.total);
	} else if (valueType) {
		return makeValueInfo(item.binName, categoryName, *valueType, item.res);
	} else {
		return makeCountInfo(item.binName, categoryName, QString(), QString(), item.res.count, item.total);
	}
}

// Highlight item when hovering over item
void BarSeries::highlight(BarSeriesIndex index, QPointF pos)
{
	if (index == highlighted) {
		if (information)
			information->setPos(pos);
		return;
	}

	// Unhighlight old highlighted item (if any)
	if (highlighted.bar >= 0 && highlighted.bar < (int)items.size())
		items[highlighted.bar].highlight(highlighted.subitem, false);
	highlighted = index;

	// Highlight new item (if any)
	if (highlighted.bar >= 0 && highlighted.bar < (int)items.size()) {
		Item &item = items[highlighted.bar];
		item.highlight(index.subitem, true);
		if (!information)
			information.reset(new InformationBox(chart()));
		information->setText(makeInfo(item, highlighted.subitem), pos);
	} else {
		information.reset();
	}
}
