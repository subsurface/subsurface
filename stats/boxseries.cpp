// SPDX-License-Identifier: GPL-2.0
#include "boxseries.h"
#include "informationbox.h"
#include "statsaxis.h"
#include "statscolors.h"
#include "statshelper.h"
#include "statstranslations.h"
#include "statsview.h"
#include "zvalues.h"
#include "core/selection.h"

#include <QLocale>

// Constants that control the bar layout
static const double boxWidth = 0.8; // 1.0 = full width of category
static const int boxBorderWidth = 2.0;

BoxSeries::BoxSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis,
		     const QString &variable, const QString &unit, int decimals) :
	StatsSeries(view, xAxis, yAxis),
	variable(variable), unit(unit), decimals(decimals), highlighted(-1), lastClicked(-1)
{
}

BoxSeries::~BoxSeries()
{
}

BoxSeries::Item::Item(StatsView &view, BoxSeries *series, double lowerBound, double upperBound,
		      const StatsQuartiles &qIn, const QString &binName, const StatsTheme &theme) :
	lowerBound(lowerBound), upperBound(upperBound), q(qIn),
	binName(binName),
	selected(allDivesSelected(q.dives))
{
	item = view.createChartItem<ChartBoxItem>(ChartZValue::Series, theme, boxBorderWidth);
	highlight(false, theme);
	updatePosition(series);
}

BoxSeries::Item::~Item()
{
}

void BoxSeries::Item::highlight(bool highlight, const StatsTheme &theme)
{
	if (highlight)
		item->setColor(theme.highlightedColor, theme.highlightedBorderColor);
	else if (selected)
		item->setColor(theme.selectedColor, theme.selectedBorderColor);
	else
		item->setColor(theme.fillColor, theme.borderColor);
}

void BoxSeries::Item::updatePosition(BoxSeries *series)
{
	StatsAxis *xAxis = series->xAxis;
	StatsAxis *yAxis = series->yAxis;
	if (!xAxis || !yAxis)
		return;

	double delta = (upperBound - lowerBound) * boxWidth;
	double from = (lowerBound + upperBound - delta) / 2.0;
	double to = (lowerBound + upperBound + delta) / 2.0;

	double fromScreen = xAxis->toScreen(from);
	double toScreen = xAxis->toScreen(to);
	double q1 = yAxis->toScreen(q.q1);
	double q3 = yAxis->toScreen(q.q3);
	QRectF rect(fromScreen, q3, toScreen - fromScreen, q1 - q3);
	item->setBox(rect, yAxis->toScreen(q.min), yAxis->toScreen(q.max), yAxis->toScreen(q.q2));
}

void BoxSeries::append(double lowerBound, double upperBound, const StatsQuartiles &q, const QString &binName)
{
	items.emplace_back(new Item(view, this, lowerBound, upperBound, q, binName, theme));
}

void BoxSeries::updatePositions()
{
	for (auto &item: items)
		item->updatePosition(this);
}

// Attention: this supposes that items are sorted by position and no box is inside another box!
int BoxSeries::getItemUnderMouse(const QPointF &point)
{
	// Search the first item whose "end" position is greater than the cursor position.
	auto it = std::lower_bound(items.begin(), items.end(), point.x(),
			   [] (const std::unique_ptr<Item> &item, double x) { return item->item->getRect().right() < x; });
	return it != items.end() && (*it)->item->getRect().contains(point) ? it - items.begin() : -1;
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
		StatsTranslations::tr("%1 (%2 dives)").arg(item.binName, loc.toString((int)item.q.dives.size())),
		QStringLiteral("%1:").arg(variable),
		infoItem(StatsTranslations::tr("min"), unit, decimals, item.q.min),
		infoItem(StatsTranslations::tr("Q1"), unit, decimals, item.q.q1),
		infoItem(StatsTranslations::tr("median"), unit, decimals, item.q.q2),
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
		item.highlight(true, theme);
		if (!information)
			information = view.createChartItem<InformationBox>();
		information->setText(formatInformation(item), pos);
		information->setVisible(true);
	} else {
		information->setVisible(false);
	}
	return highlighted >= 0;
}

void BoxSeries::unhighlight()
{
	if (highlighted >= 0 && highlighted < (int)items.size())
		items[highlighted]->highlight(false, theme);
	highlighted = -1;
}

bool BoxSeries::selectItemsUnderMouse(const QPointF &pos, SelectionModifier modifier)
{
	int index = getItemUnderMouse(pos);

	if (modifier.shift && index < 0)
		return false;

	if (!modifier.shift || lastClicked < 0)
		lastClicked = index;

	std::vector<dive *> divesUnderMouse;
	if (modifier.shift && lastClicked >= 0 && index >= 0) {
		int first = lastClicked;
		int last = index;
		if (last < first)
			std::swap(first, last);
		for (int idx = first; idx <= last; ++idx) {
			const std::vector<dive *> &dives = items[idx]->q.dives;
			divesUnderMouse.insert(divesUnderMouse.end(), dives.begin(), dives.end());
		}
	} else if (index >= 0) {
		divesUnderMouse = items[index]->q.dives;
	}
	processSelection(std::move(divesUnderMouse), modifier);

	return index >= 0;
}

void BoxSeries::divesSelected(const QVector<dive *> &)
{
	for (auto &item: items) {
		bool selected = allDivesSelected(item->q.dives);
		if (item->selected != selected) {
			item->selected = selected;

			int idx = &item - &items[0];
			bool highlight = idx == highlighted;
			item->highlight(highlight, theme);
		}
	}
}
