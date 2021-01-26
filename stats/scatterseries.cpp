// SPDX-License-Identifier: GPL-2.0
#include "scatterseries.h"
#include "chartitem.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statshelper.h"
#include "statstranslations.h"
#include "statsvariables.h"
#include "statsview.h"
#include "zvalues.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/qthelper.h"

ScatterSeries::ScatterSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
			     const StatsVariable &varX, const StatsVariable &varY) :
	StatsSeries(view, xAxis, yAxis),
	varX(varX), varY(varY)
{
}

ScatterSeries::~ScatterSeries()
{
}

ScatterSeries::Item::Item(StatsView &view, ScatterSeries *series, dive *d, double pos, double value) :
	item(view.createChartItem<ChartScatterItem>(ChartZValue::Series)),
	d(d),
	pos(pos),
	value(value)
{
	updatePosition(series);
}

void ScatterSeries::Item::updatePosition(ScatterSeries *series)
{
	item->setPos(series->toScreen(QPointF(pos, value)));
}

void ScatterSeries::Item::highlight(bool highlight)
{
	item->setHighlight(highlight);
}

void ScatterSeries::append(dive *d, double pos, double value)
{
	items.emplace_back(view, this, d, pos, value);
}

void ScatterSeries::updatePositions()
{
	for (Item &item: items)
		item.updatePosition(this);
}

std::vector<int> ScatterSeries::getItemsUnderMouse(const QPointF &point) const
{
	std::vector<int> res;
	double x = point.x();

	auto low = std::lower_bound(items.begin(), items.end(), x,
				    [] (const Item &item, double x) { return item.item->getRect().right() < x; });
	auto high = std::upper_bound(low, items.end(), x,
				    [] (double x, const Item &item) { return x < item.item->getRect().left(); });
	// Hopefully that narrows it down enough. For discrete scatter plots, we could also partition
	// by equal x and do a binary search in these partitions. But that's probably not worth it.
	res.reserve(high - low);
	for (auto it = low; it < high; ++it) {
		if (it->item->contains(point))
			res.push_back(it - items.begin());
	}
	return res;
}

static QString dataInfo(const StatsVariable &var, const dive *d)
{
	// For "numeric" variables, we display value and unit.
	// For "discrete" variables, we display all categories the dive belongs to.
	// There is only one "continuous" variable, the date, for which we don't display anything,
	// because the date is displayed anyway.
	QString val;
	switch (var.type()) {
	case StatsVariable::Type::Numeric:
		val = var.valueWithUnit(d);
		break;
	case StatsVariable::Type::Discrete:
		val = var.diveCategories(d);
		break;
	default:
		return QString();
	}

	return QString("%1: %2").arg(var.name(), val);
}

// Highlight item when hovering over item
bool ScatterSeries::hover(QPointF pos)
{
	std::vector<int> newHighlighted = getItemsUnderMouse(pos);

	if (newHighlighted == highlighted) {
		if (information)
			information->setPos(pos);
		return !newHighlighted.empty();
	}

	// This might be overkill: differential unhighlighting / highlighting of items.
	for (int idx: highlighted) {
		if (std::find(newHighlighted.begin(), newHighlighted.end(), idx) == newHighlighted.end())
			items[idx].highlight(false);
	}
	for (int idx: newHighlighted) {
		if (std::find(highlighted.begin(), highlighted.end(), idx) == highlighted.end())
			items[idx].highlight(true);
	}
	highlighted = std::move(newHighlighted);

	if (highlighted.empty()) {
		information->setVisible(false);
		return false;
	} else {
		if (!information)
			information = view.createChartItem<InformationBox>();

		std::vector<QString> text;
		text.reserve(highlighted.size() * 5);
		int show = information->recommendedMaxLines() / 5;
		int shown = 0;
		for (int idx: highlighted) {
			const dive *d = items[idx].d;
			if (!text.empty())
				text.push_back(QString(" ")); // Argh. Empty strings are filtered away.
			text.push_back(StatsTranslations::tr("Dive #%1").arg(d->number));
			text.push_back(get_dive_date_string(d->when));
			text.push_back(dataInfo(varX, d));
			text.push_back(dataInfo(varY, d));
			if (++shown >= show && shown < (int)highlighted.size()) {
				text.push_back(" ");
				text.push_back(StatsTranslations::tr("and %1 more").arg((int)highlighted.size() - shown));
				break;
			}
		}

		information->setText(text, pos);
		information->setVisible(true);
		return true;
	}
}

void ScatterSeries::unhighlight()
{
	for (int idx: highlighted)
		items[idx].highlight(false);
	highlighted.clear();
}
