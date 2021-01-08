// SPDX-License-Identifier: GPL-2.0
#include "barseries.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statshelper.h"
#include "statstranslations.h"
#include "zvalues.h"

#include <math.h> // for lrint()
#include <QLocale>

// Constants that control the bar layout
static const double barWidth = 0.8; // 1.0 = full width of category
static const double subBarWidth = 0.9; // For grouped bar charts

// Default constructor: invalid index.
BarSeries::Index::Index() : bar(-1), subitem(-1)
{
}

BarSeries::Index::Index(int bar, int subitem) : bar(bar), subitem(subitem)
{
}

bool BarSeries::Index::operator==(const Index &i2) const
{
	return std::tie(bar, subitem) == std::tie(i2.bar, i2.subitem);
}

BarSeries::BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, bool stacked, const QString &categoryName,
		     const StatsVariable *valueVariable, std::vector<QString> valueBinNames) :
	StatsSeries(scene, xAxis, yAxis),
	horizontal(horizontal), stacked(stacked), categoryName(categoryName),
	valueVariable(valueVariable), valueBinNames(std::move(valueBinNames))
{
}

BarSeries::BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, const QString &categoryName,
		     const std::vector<CountItem> &items) :
	BarSeries(scene, xAxis, yAxis, horizontal, false, categoryName, nullptr, std::vector<QString>())
{
	for (const CountItem &item: items) {
		StatsOperationResults res;
		res.count = item.count;
		double value = item.count;
		add_item(item.lowerBound, item.upperBound, makeSubItems(value, item.label),
			 item.binName, res, item.total, horizontal, stacked);
	}
}

BarSeries::BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, const QString &categoryName, const StatsVariable *valueVariable,
		     const std::vector<ValueItem> &items) :
	BarSeries(scene, xAxis, yAxis, horizontal, false, categoryName, valueVariable, std::vector<QString>())
{
	for (const ValueItem &item: items) {
		add_item(item.lowerBound, item.upperBound, makeSubItems(item.value, item.label),
			 item.binName, item.res, -1, horizontal, stacked);
	}
}

BarSeries::BarSeries(QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, bool stacked, const QString &categoryName, const StatsVariable *valueVariable,
		     std::vector<QString> valueBinNames,
		     const std::vector<MultiItem> &items) :
	BarSeries(scene, xAxis, yAxis, horizontal, stacked, categoryName, valueVariable, std::move(valueBinNames))
{
	for (const MultiItem &item: items) {
		StatsOperationResults res;
		std::vector<std::pair<double, std::vector<QString>>> valuesLabels;
		valuesLabels.reserve(item.countLabels.size());
		int total = 0;
		for (auto &[count, label]: item.countLabels) {
			valuesLabels.push_back({ static_cast<double>(count), std::move(label) });
			total += count;
		}
		add_item(item.lowerBound, item.upperBound, makeSubItems(valuesLabels),
			 item.binName, res, total, horizontal, stacked);
	}
}

BarSeries::~BarSeries()
{
}

BarSeries::BarLabel::BarLabel(QGraphicsScene *scene, const std::vector<QString> &labels, int bin_nr, int binCount) :
	totalWidth(0.0), totalHeight(0.0), isOutside(false)
{
	items.reserve(labels.size());
	for (const QString &label: labels) {
		items.emplace_back(createItem<QGraphicsSimpleTextItem>(scene));
		items.back()->setText(label);
		items.back()->setZValue(ZValues::seriesLabels);
		QRectF rect = items.back()->boundingRect();
		if (rect.width() > totalWidth)
			totalWidth = rect.width();
		totalHeight += rect.height();
	}
	highlight(false, bin_nr, binCount);
}

void BarSeries::BarLabel::setVisible(bool visible)
{
	for (auto &item: items)
		item->setVisible(visible);
}

void BarSeries::BarLabel::highlight(bool highlight, int bin_nr, int binCount)
{
	QBrush brush(highlight || isOutside ? darkLabelColor : labelColor(bin_nr, binCount));
	for (auto &item: items)
		item->setBrush(brush);
}

void BarSeries::BarLabel::updatePosition(bool horizontal, bool center, const QRectF &rect,
					 int bin_nr, int binCount)
{
	if (!horizontal) {
		if (totalWidth > rect.width()) {
			setVisible(false);
			return;
		}
		QPointF pos = rect.center();

		// Heuristics: if the label fits nicely into the bar (bar height is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it at the top of the bar.
		isOutside = !center && rect.height() < 2.0 * totalHeight;
		if (isOutside) {
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
		isOutside = !center && rect.width() < 2.0 * totalWidth;
		if (isOutside) {
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
	// If label changed from inside to outside, or vice-versa, the color might change.
	highlight(false, bin_nr, binCount);
}

BarSeries::Item::Item(QGraphicsScene *scene, BarSeries *series, double lowerBound, double upperBound,
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
		item.item->setZValue(ZValues::series);
		item.highlight(false, binCount);
	}
	updatePosition(series, horizontal, stacked, binCount);
}

void BarSeries::Item::highlight(int subitem, bool highlight, int binCount)
{
	if (subitem < 0 || subitem >= (int)subitems.size())
		return;
	subitems[subitem].highlight(highlight, binCount);
}

void BarSeries::SubItem::highlight(bool highlight, int binCount)
{
	if (highlight) {
		item->setBrush(QBrush(highlightedColor));
		item->setPen(QPen(highlightedBorderColor));
	} else {
		item->setBrush(QBrush(binColor(bin_nr, binCount)));
		item->setPen(QPen(::borderColor));
	}
	if (label)
		label->highlight(highlight, bin_nr, binCount);
}

void BarSeries::Item::updatePosition(BarSeries *series, bool horizontal, bool stacked, int binCount)
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
		item.updatePosition(series, horizontal, stacked, center - subWidth / 2.0, center + subWidth / 2.0, binCount);
	}
	rect = subitems[0].item->rect();
	for (auto it = std::next(subitems.begin()); it != subitems.end(); ++it)
		rect = rect.united(it->item->rect());
}

void BarSeries::SubItem::updatePosition(BarSeries *series, bool horizontal, bool stacked,
					double from, double to, int binCount)
{
	QPointF topLeft, bottomRight;
	if (horizontal) {
		topLeft = series->toScreen(QPointF(value_from, to));
		bottomRight = series->toScreen(QPointF(value_to, from));
	} else {
		topLeft = series->toScreen(QPointF(from, value_to));
		bottomRight = series->toScreen(QPointF(to, value_from));
	}
	QRectF rect(topLeft, bottomRight);
	item->setRect(rect);
	if (label)
		label->updatePosition(horizontal, stacked, rect, bin_nr, binCount);
}

std::vector<BarSeries::SubItem> BarSeries::makeSubItems(const std::vector<std::pair<double, std::vector<QString>>> &values) const
{
	std::vector<SubItem> res;
	res.reserve(values.size());
	double from = 0.0;
	int bin_nr = 0;
	for (auto &[v, label]: values) {
		if (v > 0.0) {
			res.push_back({ createItemPtr<QGraphicsRectItem>(scene), {}, from, from + v, bin_nr });
			if (!label.empty())
				res.back().label = std::make_unique<BarLabel>(scene, label, bin_nr, binCount());
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
	items.emplace_back(scene, this, lowerBound, upperBound, std::move(subitems), binName, res,
			   total, horizontal, stacked, binCount());
}

void BarSeries::updatePositions()
{
	for (Item &item: items)
		item.updatePosition(this, horizontal, stacked, binCount());
}

// Attention: this supposes that items are sorted by position and no bar is inside another bar!
BarSeries::Index BarSeries::getItemUnderMouse(const QPointF &point) const
{
	// Search the first item whose "end" position is greater than the cursor position.
	auto it = horizontal ? std::lower_bound(items.begin(), items.end(), point.y(),
						[] (const Item &item, double y) { return item.rect.top() > y; })
			     : std::lower_bound(items.begin(), items.end(), point.x(),
						[] (const Item &item, double x) { return item.rect.right() < x; });
	if (it == items.end() || !it->rect.contains(point))
		return Index();
	int subitem = it->getSubItemUnderMouse(point, horizontal, stacked);
	return subitem >= 0 ? Index{(int)(it - items.begin()), subitem} : Index();
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
	res.push_back(StatsTranslations::tr("%1 (%2 of %3) dives").arg(countString, percentageString, totalString));
	return res;
}

// Format information in a value bar chart: the name of the bin and the value with unit.
static std::vector<QString> makeValueInfo(const QString &binName, const QString &axisName,
					  const StatsVariable &valueVariable, const StatsOperationResults &values)
{
	QLocale loc;
	int decimals = valueVariable.decimals();
	QString unit = valueVariable.unitSymbol();
	std::vector<StatsOperation> operations = valueVariable.supportedOperations();
	std::vector<QString> res;
	res.reserve(operations.size() + 3);
	res.push_back(QStringLiteral("%1: %2").arg(axisName, binName));
	res.push_back(QStringLiteral("%1: %2").arg(StatsTranslations::tr("Count"), loc.toString(values.count)));
	res.push_back(QStringLiteral("%1: ").arg(valueVariable.name()));
	for (StatsOperation op: operations) {
		QString valueFormatted = loc.toString(values.get(op), 'f', decimals);
		res.push_back(QString(" %1: %2 %3").arg(StatsVariable::operationName(op), valueFormatted, unit));
	}
	return res;
}

std::vector<QString> BarSeries::makeInfo(const Item &item, int subitem_idx) const
{
	if (!valueBinNames.empty() && valueVariable) {
		if (subitem_idx < 0 || subitem_idx >= (int)item.subitems.size())
			return {};
		const SubItem &subitem = item.subitems[subitem_idx];
		if (subitem.bin_nr < 0 || subitem.bin_nr >= (int)valueBinNames.size())
			return {};
		int count = (int)lrint(subitem.value_to - subitem.value_from);
		return makeCountInfo(item.binName, categoryName, valueBinNames[subitem.bin_nr],
				     valueVariable->name(), count, item.total);
	} else if (valueVariable) {
		return makeValueInfo(item.binName, categoryName, *valueVariable, item.res);
	} else {
		return makeCountInfo(item.binName, categoryName, QString(), QString(), item.res.count, item.total);
	}
}

// Highlight item when hovering over item
bool BarSeries::hover(QPointF pos)
{
	Index index = getItemUnderMouse(pos);
	if (index == highlighted) {
		if (information)
			information->setPos(pos);
		return index.bar >= 0;
	}

	unhighlight();
	highlighted = index;

	// Highlight new item (if any)
	if (highlighted.bar >= 0 && highlighted.bar < (int)items.size()) {
		Item &item = items[highlighted.bar];
		item.highlight(index.subitem, true, binCount());
		if (!information)
			information = createItemPtr<InformationBox>(scene);
		information->setText(makeInfo(item, highlighted.subitem), pos);
	} else {
		information.reset();
	}

	return highlighted.bar >= 0;
}

void BarSeries::unhighlight()
{
	if (highlighted.bar >= 0 && highlighted.bar < (int)items.size())
		items[highlighted.bar].highlight(highlighted.subitem, false, binCount());
	highlighted = Index();
}
