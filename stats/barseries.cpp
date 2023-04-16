// SPDX-License-Identifier: GPL-2.0
#include "barseries.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statshelper.h"
#include "statstranslations.h"
#include "statsview.h"
#include "zvalues.h"
#include "core/dive.h"
#include "core/selection.h"

#include <math.h> // for lrint()
#include <QLocale>

// Constants that control the bar layout
static const double barWidth = 0.8; // 1.0 = full width of category
static const double subBarWidth = 0.9; // For grouped bar charts
static const double barBorderWidth = 1.0;

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

// Note: only defined for valid indices.
bool BarSeries::Index::operator<=(const Index &i2) const
{
	return std::tie(bar, subitem) <= std::tie(i2.bar, i2.subitem);
}

// Note: only defined for valid indices.
void BarSeries::inc(Index &index)
{
	if (++index.subitem < binCount())
		return;
	++index.bar;
	index.subitem = 0;
}

BarSeries::BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, bool stacked, const QString &categoryName,
		     const StatsVariable *valueVariable, std::vector<QString> valueBinNames) :
	StatsSeries(view, xAxis, yAxis),
	horizontal(horizontal), stacked(stacked), categoryName(categoryName),
	valueVariable(valueVariable), valueBinNames(std::move(valueBinNames))
{
}

BarSeries::BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, const QString &categoryName,
		     std::vector<CountItem> items) :
	BarSeries(view, xAxis, yAxis, horizontal, false, categoryName, nullptr, std::vector<QString>())
{
	for (CountItem &item: items) {
		StatsOperationResults res;
		double value = (double)item.dives.size();
		add_item(item.lowerBound, item.upperBound, makeSubItems({ value, std::move(item.dives), std::move(item.label) }),
			 item.binName, res, item.total, horizontal, stacked);
	}
}

BarSeries::BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, const QString &categoryName, const StatsVariable *valueVariable,
		     std::vector<ValueItem> items) :
	BarSeries(view, xAxis, yAxis, horizontal, false, categoryName, valueVariable, std::vector<QString>())
{
	for (ValueItem &item: items) {
		add_item(item.lowerBound, item.upperBound, makeSubItems({ item.value, std::move(item.res.dives), std::move(item.label) }),
			 item.binName, item.res, -1, horizontal, stacked);
	}
}

BarSeries::BarSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		     bool horizontal, bool stacked, const QString &categoryName, const StatsVariable *valueVariable,
		     std::vector<QString> valueBinNames,
		     std::vector<MultiItem> items) :
	BarSeries(view, xAxis, yAxis, horizontal, stacked, categoryName, valueVariable, std::move(valueBinNames))
{
	for (MultiItem &item: items) {
		StatsOperationResults res;
		std::vector<SubItemDesc> subitems;
		subitems.reserve(item.items.size());
		int total = 0;
		for (auto &[dives, label]: item.items) {
			total += (int)dives.size();
			subitems.push_back({ static_cast<double>(dives.size()), std::move(dives), std::move(label) });
		}
		add_item(item.lowerBound, item.upperBound, makeSubItems(std::move(subitems)),
			 item.binName, res, total, horizontal, stacked);
	}
}

BarSeries::~BarSeries()
{
}

BarSeries::BarLabel::BarLabel(StatsView &view, const std::vector<QString> &labels, int bin_nr, int binCount) :
	isOutside(false)
{
	const QFont &f = view.getCurrentTheme().labelFont;
	item = view.createChartItem<ChartTextItem>(ChartZValue::SeriesLabels, f, labels, true);
	//highlight(false, bin_nr, binCount);
}

void BarSeries::BarLabel::setVisible(bool visible)
{
	item->setVisible(visible);
}

void BarSeries::BarLabel::highlight(bool highlight, int bin_nr, int binCount, const QColor &background, const StatsTheme &theme)
{
	// For labels that are on top of a bar, use the corresponding bar color
	// as background. Rendering on a transparent background gives ugly artifacts.
	item->setColor(highlight || isOutside ? theme.darkLabelColor : theme.labelColor(bin_nr, binCount),
		       isOutside ? Qt::transparent : background);
}

void BarSeries::BarLabel::updatePosition(bool horizontal, bool center, const QRectF &rect,
					 int bin_nr, int binCount, const QColor &background,
					 const StatsTheme &theme)
{
	QSizeF itemSize = item->getRect().size();
	if (!horizontal) {
		if (itemSize.width() > rect.width()) {
			setVisible(false);
			return;
		}
		QPointF pos = rect.center();
		pos.rx() -= itemSize.width() / 2.0;

		// Heuristics: if the label fits nicely into the bar (bar height is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it at the top of the bar.
		isOutside = !center && rect.height() < 2.0 * itemSize.height();
		if (isOutside) {
			pos.ry() = rect.top() - (itemSize.height() + 2.0); // Leave two pixels(?) space
		} else {
			if (itemSize.height() > rect.height()) {
				setVisible(false);
				return;
			}
			pos.ry() -= itemSize.height() / 2.0;
		}
		item->setPos(roundPos(pos)); // Round to integer to avoid ugly artifacts.
	} else {
		if (itemSize.height() > rect.height()) {
			setVisible(false);
			return;
		}
		QPointF pos = rect.center();
		pos.ry() -= itemSize.height() / 2.0;

		// Heuristics: if the label fits nicely into the bar (bar width is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it to the right of the bar.
		isOutside = !center && rect.width() < 2.0 * itemSize.width();
		if (isOutside) {
			pos.rx() = rect.right() + 2.0; // Leave two pixels(?) space
		} else {
			if (itemSize.width() > rect.width()) {
				setVisible(false);
				return;
			}
			pos.rx() -= itemSize.width() / 2.0;
		}
		item->setPos(roundPos(pos)); // Round to integer to avoid ugly artifacts.
	}
	setVisible(true);
	// If label changed from inside to outside, or vice-versa, the color might change.
	highlight(false, bin_nr, binCount, background, theme);
}

BarSeries::Item::Item(BarSeries *series, double lowerBound, double upperBound,
		      std::vector<SubItem> subitemsIn,
		      const QString &binName, const StatsOperationResults &res, int total,
		      bool horizontal, bool stacked, int binCount,
		      const StatsTheme &theme) :
	lowerBound(lowerBound),
	upperBound(upperBound),
	subitems(std::move(subitemsIn)),
	binName(binName),
	res(res),
	total(total)
{
	for (SubItem &item: subitems)
		item.highlight(false, binCount, theme);
	updatePosition(series, horizontal, stacked, binCount, theme);
}

void BarSeries::Item::highlight(int subitem, bool highlight, int binCount, const StatsTheme &theme)
{
	if (subitem < 0 || subitem >= (int)subitems.size())
		return;
	subitems[subitem].highlight(highlight, binCount, theme);
}

// For single-bin charts, selected items are marked with a special fill and border color.
// For multi-bin charts, they are marked by a differend border color and border width.
void BarSeries::SubItem::highlight(bool highlight, int binCount, const StatsTheme &theme)
{
	fill = highlight ? theme.highlightedColor : theme.binColor(bin_nr, binCount);
	QColor border = highlight ? theme.highlightedBorderColor : theme.borderColor;
	item->setColor(fill, border);
	item->setSelected(selected);
	if (label)
		label->highlight(highlight, bin_nr, binCount, fill, theme);
}

void BarSeries::Item::updatePosition(BarSeries *series, bool horizontal, bool stacked, int binCount, const StatsTheme &theme)
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
		item.updatePosition(series, horizontal, stacked, center - subWidth / 2.0, center + subWidth / 2.0, binCount, theme);
	}
	rect = subitems[0].item->getRect();
	for (auto it = std::next(subitems.begin()); it != subitems.end(); ++it)
		rect = rect.united(it->item->getRect());
}

void BarSeries::SubItem::updatePosition(BarSeries *series, bool horizontal, bool stacked,
					double from, double to, int binCount,
					const StatsTheme &theme)
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
		label->updatePosition(horizontal, stacked, rect, bin_nr, binCount, fill, theme);
}

std::vector<BarSeries::SubItem> BarSeries::makeSubItems(std::vector<SubItemDesc> items) const
{
	std::vector<SubItem> res;
	res.reserve(items.size());
	double from = 0.0;
	int bin_nr = 0;
	for (auto &[v, dives, label]: items) {
		if (v > 0.0) {
			bool selected = allDivesSelected(dives);
			res.push_back({ view.createChartItem<ChartBarItem>(ChartZValue::Series, theme, barBorderWidth),
					std::move(dives),
					{}, from, from + v, bin_nr, selected });
			if (!label.empty())
				res.back().label = std::make_unique<BarLabel>(view, label, bin_nr, binCount());
		}
		if (stacked)
			from += v;
		++bin_nr;
	}
	return res;
}

std::vector<BarSeries::SubItem> BarSeries::makeSubItems(SubItemDesc item) const
{
	return makeSubItems(std::vector<SubItemDesc>{ std::move(item) });
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
	items.emplace_back(this, lowerBound, upperBound, std::move(subitems), binName, res,
			   total, horizontal, stacked, binCount(), theme);
}

void BarSeries::updatePositions()
{
	for (Item &item: items)
		item.updatePosition(this, horizontal, stacked, binCount(), theme);
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
					      [] (const SubItem &item, double x) { return item.item->getRect().right() < x; })
			   : std::lower_bound(subitems.begin(), subitems.end(), point.y(),
					      [] (const SubItem &item, double y) { return item.item->getRect().top() > y; });
	return it != subitems.end() && it->item->getRect().contains(point) ? it - subitems.begin() : -1;
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
					  const StatsVariable &valueVariable, const StatsOperationResults &values,
					  int count)
{
	QLocale loc;
	int decimals = valueVariable.decimals();
	QString unit = valueVariable.unitSymbol();
	std::vector<StatsOperation> operations = valueVariable.supportedOperations();
	std::vector<QString> res;
	res.reserve(operations.size() + 3);
	res.push_back(QStringLiteral("%1: %2").arg(axisName, binName));
	res.push_back(QStringLiteral("%1: %2").arg(StatsTranslations::tr("Count"), loc.toString(count)));
	res.push_back(QStringLiteral("%1: ").arg(valueVariable.name()));
	for (StatsOperation op: operations) {
		QString valueFormatted = loc.toString(values.get(op), 'f', decimals);
		res.push_back(QString(" %1: %2 %3").arg(StatsVariable::operationName(op), valueFormatted, unit));
	}
	return res;
}

std::vector<QString> BarSeries::makeInfo(const Item &item, int subitem_idx) const
{
	if (item.subitems.empty())
		return {};
	if (!valueBinNames.empty() && valueVariable) {
		if (subitem_idx < 0 || subitem_idx >= (int)item.subitems.size())
			return {};
		const SubItem &subitem = item.subitems[subitem_idx];
		if (subitem.bin_nr < 0 || subitem.bin_nr >= (int)valueBinNames.size())
			return {};
		int count = (int)subitem.dives.size();
		return makeCountInfo(item.binName, categoryName, valueBinNames[subitem.bin_nr],
				     valueVariable->name(), count, item.total);
	} else if (valueVariable) {
		int count = (int)item.subitems[0].dives.size();
		return makeValueInfo(item.binName, categoryName, *valueVariable, item.res, count);
	} else {
		int count = (int)item.subitems[0].dives.size();
		return makeCountInfo(item.binName, categoryName, QString(), QString(), count, item.total);
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
		item.highlight(index.subitem, true, binCount(), theme);
		if (!information)
			information = view.createChartItem<InformationBox>(theme);
		information->setText(makeInfo(item, highlighted.subitem), pos);
		information->setVisible(true);
	} else {
		information->setVisible(false);
	}

	return highlighted.bar >= 0;
}

void BarSeries::unhighlight()
{
	if (highlighted.bar >= 0 && highlighted.bar < (int)items.size())
		items[highlighted.bar].highlight(highlighted.subitem, false, binCount(), theme);
	highlighted = Index();
}

bool BarSeries::selectItemsUnderMouse(const QPointF &pos, SelectionModifier modifier)
{
	Index index = getItemUnderMouse(pos);

	if (modifier.shift && index.bar < 0)
		return false;

	if (!modifier.shift || lastClicked.bar < 0)
		lastClicked = index;

	std::vector<dive *> divesUnderMouse;
	if (modifier.shift && lastClicked.bar >= 0 && index.bar >= 0) {
		Index first = lastClicked;
		Index last = index;
		if (last <= first)
			std::swap(first, last);
		for (Index idx = first; idx <= last; inc(idx)) {
			const std::vector<dive *> &dives = items[idx.bar].subitems[idx.subitem].dives;
			divesUnderMouse.insert(divesUnderMouse.end(), dives.begin(), dives.end());
		}
	} else if (index.bar >= 0) {
		divesUnderMouse = items[index.bar].subitems[index.subitem].dives;
	}
	processSelection(std::move(divesUnderMouse), modifier);

	return index.bar >= 0;
}

void BarSeries::divesSelected(const QVector<dive *> &)
{
	for (Item &item: items) {
		for (SubItem &subitem: item.subitems) {
			bool selected = allDivesSelected(subitem.dives);
			if (subitem.selected != selected) {
				subitem.selected = selected;

				Index idx(&item - &items[0], &subitem - &item.subitems[0]);
				bool highlight = idx == highlighted;
				item.highlight(idx.subitem, highlight, binCount(), theme);
			}
		}
	}
}
