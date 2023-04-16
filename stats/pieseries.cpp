// SPDX-License-Identifier: GPL-2.0
#include "pieseries.h"
#include "informationbox.h"
#include "statscolors.h"
#include "statshelper.h"
#include "statstranslations.h"
#include "statsview.h"
#include "zvalues.h"
#include "core/selection.h"

#include <numeric>
#include <math.h>
#include <QLocale>

static const double pieSize = 0.9; // 1.0 = occupy full width of chart
static const double pieBorderWidth = 1.0;
static const double innerLabelRadius = 0.75; // 1.0 = at outer border of pie
static const double outerLabelRadius = 1.01; // 1.0 = at outer border of pie

PieSeries::Item::Item(StatsView &view, const QString &name, int from, std::vector<dive *> divesIn, int totalCount,
		      int bin_nr, int numBins, const StatsTheme &theme) :
	name(name),
	dives(std::move(divesIn)),
	selected(allDivesSelected(dives))
{
	const QFont &f = theme.labelFont;
	QLocale loc;

	int count = (int)dives.size();

	// totalCount = 0 shouldn't happen, but for robustness' sake, let's not divide by zero.
	if (totalCount <= 0)
		totalCount = count = 1;

	angleFrom = static_cast<double>(from) / totalCount;
	angleTo = static_cast<double>(from + count) / totalCount;
	double meanAngle = M_PI / 2.0 - (from + count / 2.0) / totalCount * M_PI * 2.0; // Note: "-" because we go CW.
	innerLabelPos = QPointF(cos(meanAngle) * innerLabelRadius, -sin(meanAngle) * innerLabelRadius);
	outerLabelPos = QPointF(cos(meanAngle) * outerLabelRadius, -sin(meanAngle) * outerLabelRadius);

	double percentage = count * 100.0 / totalCount;
	QString innerLabelText = QStringLiteral("%1\%").arg(loc.toString(percentage, 'f', 1));
	innerLabel = view.createChartItem<ChartTextItem>(ChartZValue::SeriesLabels, f, innerLabelText);

	outerLabel = view.createChartItem<ChartTextItem>(ChartZValue::SeriesLabels, f, name);
	outerLabel->setColor(theme.darkLabelColor);
}

void PieSeries::Item::updatePositions(const QPointF &center, double radius)
{
	// Note: the positions in this functions are rounded to integer values,
	// because half-integer values gives horrible aliasing artifacts.
	if (innerLabel) {
		QRectF labelRect = innerLabel->getRect();
		QPointF pos(center.x() + innerLabelPos.x() * radius - labelRect.width() / 2.0,
			    center.y() + innerLabelPos.y() * radius - labelRect.height() / 2.0);
		innerLabel->setPos(roundPos(pos));
	}
	if (outerLabel) {
		QRectF labelRect = outerLabel->getRect();
		QPointF pos(center.x() + outerLabelPos.x() * radius, center.y() + outerLabelPos.y() * radius);
		if (outerLabelPos.x() < 0.0) {
			if (outerLabelPos.y() < 0.0)
				pos -= QPointF(labelRect.width(), labelRect.height());
			else
				pos.rx() -= labelRect.width();
		} else if (outerLabelPos.y() < 0.0) {
			pos.ry() -= labelRect.height();
		}

		outerLabel->setPos(roundPos(pos));
	}
}

void PieSeries::Item::highlight(ChartPieItem &item, int bin_nr, bool highlight, int numBins, const StatsTheme &theme)
{
	QColor fill = highlight ? theme.highlightedColor : theme.binColor(bin_nr, numBins);
	QColor border = highlight ? theme.highlightedBorderColor : theme.borderColor;
	if (innerLabel)
		innerLabel->setColor(highlight ? theme.darkLabelColor : theme.labelColor(bin_nr, numBins), fill);
	item.drawSegment(angleFrom, angleTo, fill, border, selected);
}

PieSeries::PieSeries(StatsView &view, StatsAxis *xAxis, StatsAxis *yAxis, const QString &categoryName,
		     std::vector<std::pair<QString, std::vector<dive *>>> data, ChartSortMode sortMode) :
	StatsSeries(view, xAxis, yAxis),
	item(view.createChartItem<ChartPieItem>(ChartZValue::Series, theme, pieBorderWidth)),
	categoryName(categoryName),
	radius(0),
	highlighted(-1),
	lastClicked(-1)
{
	// Pie charts with many slices are unreadable. Therefore, subsume slices under
	// a certain percentage as "other". But draw a minimum number of slices
	// until we reach 50% so that we never get a pie only of "other".
	// This is heuristics, which might have to be optimized.
	const int smallest_slice_percentage = 5; // Smaller than 5% = others. That makes at most 20 slices.
	const int min_slices = 5; // Try to draw at least 5 slices until we reach 50%

	// Easier to read than std::accumulate
	totalCount = 0;
	for (const auto &[name, dives]: data)
		totalCount += (int)dives.size();

	// First of all, sort from largest to smallest slice. Instead
	// of sorting the initial array, sort a list of indices, so that
	// the original order can be easily reconstructed later.
	std::vector<int> sorted(data.size());
	std::iota(sorted.begin(), sorted.end(), 0); // Fill with 0..size-1.
	// Two notes:
	//  - by negating the counts in the sort below, count is sorted descending.
	//  - do a lexicographic sort by (count, idx) so that for equal counts the order is preserved.
	std::sort(sorted.begin(), sorted.end(),
		  [&data](int idx1, int idx2)
		  { return std::make_tuple(-data[idx1].second.size(), idx1) <
			   std::make_tuple(-data[idx2].second.size(), idx2); });
	auto it = std::find_if(sorted.begin(), sorted.end(),
			       [count=totalCount, &data](int idx)
			       { return (int)data[idx].second.size() * 100 / count < smallest_slice_percentage; });
	if (it - sorted.begin() < min_slices) {
		// Take minimum amount of slices below 50%...
		int sum = 0;
		for (auto it2 = sorted.begin(); it2 != it; ++it2)
			sum += (int)data[*it2].second.size();

		while(it != sorted.end() && sum * 2 < totalCount && it - sorted.begin() < min_slices) {
			sum += (int)data[*it].second.size();
			++it;
		}
	}

	// Don't do a single "other" group
	if (sorted.end() - it == 1)
		++it;

	// Sort the main groups and the other groups back, if requested
	if (sortMode == ChartSortMode::Bin) {
		std::sort(sorted.begin(), it);
		std::sort(it, sorted.end());
	}

	int numBins = it - sorted.begin();
	if (it != sorted.end())
		++numBins;
	items.reserve(numBins);
	int act = 0;
	for (auto it2 = sorted.begin(); it2 != it; ++it2) {
		int count = (int)data[*it2].second.size();
		items.emplace_back(view, data[*it2].first, act, std::move(data[*it2].second),
				   totalCount, (int)items.size(), numBins, theme);
		act += count;
	}

	// Register the items of the "other" group.
	if (it != sorted.end()) {
		std::vector<dive *> otherDives;
		otherDives.reserve(totalCount - act);
		other.reserve(sorted.end() - it);
		for (auto it2 = it; it2 != sorted.end(); ++it2) {
			other.push_back({ data[*it2].first, (int)data[*it2].second.size() });
			for (dive *d: data[*it2].second)
				otherDives.push_back(d);
		}
		QString name = StatsTranslations::tr("other (%1 items)").arg(other.size());
		items.emplace_back(view, name, act, std::move(otherDives), totalCount, (int)items.size(), numBins, theme);
	}
}

PieSeries::~PieSeries()
{
}

void PieSeries::updatePositions()
{
	QRectF plotRect = view.plotArea();
	center = plotRect.center();
	radius = ceil(std::min(plotRect.width(), plotRect.height()) * pieSize / 2.0);
	QRectF rect(round(center.x() - radius), round(center.y() - radius), ceil(2.0 * radius), ceil(2.0 * radius));
	item->resize(rect.size());
	item->setPos(rect.topLeft());
	int i = 0;
	for (Item &segment: items) {
		segment.updatePositions(center, radius);
		segment.highlight(*item, i, i == highlighted, (int)items.size(), theme); // Draw segment
		++i;
	}
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
						    makePercentageLine((int)items[idx].dives.size(), totalCount)));
		for (const OtherItem &item: other)
			res.push_back(QString("%1: %2").arg(item.name,
							    makePercentageLine((int)item.count, totalCount)));
	} else {
		// A "normal" item.
		res.reserve(2);
		res.push_back(QStringLiteral("%1: %2").arg(categoryName, items[idx].name));
		res.push_back(makePercentageLine((int)items[idx].dives.size(), totalCount));
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
		items[highlighted].highlight(*item, highlighted, true, (int)items.size(), theme);
		if (!information)
			information = view.createChartItem<InformationBox>(theme);
		information->setText(makeInfo(highlighted), pos);
		information->setVisible(true);
	} else {
		information->setVisible(false);
	}
	return highlighted >= 0;
}

void PieSeries::unhighlight()
{
	if (highlighted >= 0 && highlighted < (int)items.size())
		items[highlighted].highlight(*item, highlighted, false, (int)items.size(), theme);
	highlighted = -1;
}

bool PieSeries::selectItemsUnderMouse(const QPointF &pos, SelectionModifier modifier)
{
	int index = getItemUnderMouse(pos);

	if (modifier.shift && index < 0)
		return false;

	if (!modifier.shift || lastClicked < 0)
		lastClicked = index;

	std::vector<dive *> divesUnderMouse;
	if (modifier.shift && lastClicked >= 0 && index >= 0) {
		// Selecting a range in a pie plot is a bit special due to its cyclic nature.
		// One way would be to always select the "shorter" path, but that would restrict the user.
		// Thus, always select in the "positive" direction, i.e. clockwise.
		int idx = lastClicked;
		int last = index;
		for (;;) {
			const std::vector<dive *> &dives = items[idx].dives;
			divesUnderMouse.insert(divesUnderMouse.end(), dives.begin(), dives.end());
			if (idx == last)
				break;
			if (++idx >= (int)items.size())
				idx = 0;
		}
	} else if (index >= 0) {
		divesUnderMouse = items[index].dives;
	}
	processSelection(std::move(divesUnderMouse), modifier);

	return index >= 0;
}

void PieSeries::divesSelected(const QVector<dive *> &)
{
	for (Item &segment: items) {
		bool selected = allDivesSelected(segment.dives);
		if (segment.selected != selected) {
			segment.selected = selected;

			int idx = &segment - &items[0];
			segment.highlight(*item, idx, idx == highlighted, (int)items.size(), theme);
		}
	}
}
