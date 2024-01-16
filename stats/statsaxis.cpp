// SPDX-License-Identifier: GPL-2.0
#include "statsaxis.h"
#include "statscolors.h"
#include "statshelper.h"
#include "statstranslations.h"
#include "statsvariables.h"
#include "statsview.h"
#include "zvalues.h"
#include "core/pref.h"
#include "core/subsurface-time.h"
#include <math.h> // for lrint
#include <numeric>
#include <array> // for std::array
#include <QFontMetrics>
#include <QLocale>

// Define most constants for horizontal and vertical axes for more flexibility.
// Note: *Horizontal means that this is for the horizontal axis, so a vertical space.
static const double axisWidth = 0.5;
static const double axisTickWidth = 0.3;
static const double axisTickSizeHorizontal = 6.0;
static const double axisTickSizeVertical = 6.0;
static const double axisLabelSpaceHorizontal = 2.0;	// Space between axis or ticks and labels
static const double axisLabelSpaceVertical = 2.0;	// Space between axis or ticks and labels
static const double axisTitleSpaceHorizontal = 2.0;	// Space between labels and title
static const double axisTitleSpaceVertical = 2.0;	// Space between labels and title

StatsAxis::StatsAxis(StatsView &view, const QString &title, bool horizontal, bool labelsBetweenTicks) :
	ChartPixmapItem(view, ChartZValue::Axes),
	theme(view.getCurrentTheme()),
	line(view.createChartItem<ChartLineItem>(ChartZValue::Axes, theme.axisColor, axisWidth)),
	title(title), horizontal(horizontal), labelsBetweenTicks(labelsBetweenTicks),
	size(1.0), zeroOnScreen(0.0), min(0.0), max(1.0), labelWidth(0.0)
{
	QFontMetrics fm(theme.axisTitleFont);
	titleWidth = title.isEmpty() ? 0.0
				     : static_cast<double>(fm.size(Qt::TextSingleLine, title).width());
}

StatsAxis::~StatsAxis()
{
}

std::pair<double, double> StatsAxis::minMax() const
{
	return { min, max };
}

std::pair<double, double> StatsAxis::minMaxScreen() const
{
	return horizontal ? std::make_pair(zeroOnScreen, zeroOnScreen + size)
			  : std::make_pair(zeroOnScreen, zeroOnScreen - size);
}

std::pair<double, double> StatsAxis::horizontalOverhang() const
{
	// If the labels are between ticks, they cannot peak out
	if (!horizontal || labelsBetweenTicks)
		return { 0.0, 0.0 };
	QFontMetrics fm(theme.axisLabelFont);
	auto [firstLabel, lastLabel] = getFirstLastLabel();
	return { fm.size(Qt::TextSingleLine, firstLabel).width() / 2.0,
		 fm.size(Qt::TextSingleLine, lastLabel).width() / 2.0 };
}

void StatsAxis::setRange(double minIn, double maxIn)
{
	min = minIn;
	max = maxIn;
}

// Guess the number of tick marks based on example strings.
// We will use minimum and maximum values, which are not necessarily the
// maximum-size strings especially, when using proportional fonts or for
// categorical data. Therefore, try to err on the safe side by adding enough
// margins.
int StatsAxis::guessNumTicks(const std::vector<QString> &strings) const
{
	QFontMetrics fm(theme.axisLabelFont);
	int minSize = fm.height();
	for (const QString &s: strings) {
		QSize labelSize = fm.size(Qt::TextSingleLine, s);
		int needed = horizontal ? labelSize.width() : labelSize.height();
		if (needed > minSize)
			minSize = needed;
	}

	// Add space between labels
	if (horizontal)
		minSize = minSize * 3 / 2;
	else
		minSize *= 2;
	int numTicks = lrint(size / minSize);
	return std::max(numTicks, 2);
}

double StatsAxis::titleSpace() const
{
	if (title.isEmpty())
		return 0.0;
	return horizontal ? QFontMetrics(theme.axisTitleFont).height() + axisTitleSpaceHorizontal
			  : QFontMetrics(theme.axisTitleFont).height() + axisTitleSpaceVertical;
}

double StatsAxis::width() const
{
	if (horizontal)
		return 0.0;	// Only supported for vertical axes
	return labelWidth + axisLabelSpaceVertical + titleSpace() +
	       (labelsBetweenTicks ? 0.0 : axisTickSizeVertical);
}

double StatsAxis::height() const
{
	if (!horizontal)
		return 0.0;	// Only supported for horizontal axes
	return QFontMetrics(theme.axisLabelFont).height() + axisLabelSpaceHorizontal +
	       titleSpace() +
	       (labelsBetweenTicks ? 0.0 : axisTickSizeHorizontal);
}

void StatsAxis::addLabel(const QFontMetrics &fm, const QString &label, double pos)
{
	labels.push_back({ label, fm.size(Qt::TextSingleLine, label).width(), pos });
}

void StatsAxis::addTick(double pos)
{
	ticks.push_back({ view.createChartItem<ChartLineItem>(ChartZValue::Axes, theme.axisColor, axisTickWidth), pos });
}

std::vector<double> StatsAxis::ticksPositions() const
{
	std::vector<double> res;
	res.reserve(ticks.size());
	for (const Tick &tick: ticks)
		res.push_back(toScreen(tick.pos));
	return res;
}

// Map x (horizontal) or y (vertical) coordinate to or from screen coordinate
double StatsAxis::toScreen(double pos) const
{
	// Vertical is bottom-up
	return horizontal ? (pos - min) / (max - min) * size + zeroOnScreen
			  : (min - pos) / (max - min) * size + zeroOnScreen;
}

double StatsAxis::toValue(double pos) const
{
	// Vertical is bottom-up
	return horizontal ? (pos - zeroOnScreen) / size * (max - min) + min
			  : (zeroOnScreen - pos) / size * (max - min) + zeroOnScreen;
}

void StatsAxis::setSize(double sizeIn)
{
	size = sizeIn;

	// Ticks (and labels) should probably be reused. For now, clear them.
	for (Tick &tick: ticks)
		view.deleteChartItem(tick.item);
	labels.clear();
	ticks.clear();
	updateLabels();

	labelWidth = 0.0;
	for (const Label &label: labels) {
		if (label.width > labelWidth)
			labelWidth = label.width;
	}

	QFontMetrics fm(theme.axisLabelFont);
	int fontHeight = fm.height();
	if (horizontal) {
		double pixmapWidth = size;
		double offsetX = 0.0;
		if (!labels.empty() && !labelsBetweenTicks) {
			pixmapWidth += labels.front().width / 2.0 + labels.back().width / 2.0;
			offsetX += labels.front().width / 2.0;
		}

		double pixmapHeight = fontHeight + titleSpace();
		double offsetY = -axisWidth / 2.0 - axisLabelSpaceHorizontal -
				 (labelsBetweenTicks ? 0.0 : axisTickSizeHorizontal);

		ChartPixmapItem::resize(QSizeF(pixmapWidth, pixmapHeight)); // Note: this rounds the dimensions up
		offset = QPointF(round(offsetX), round(offsetY));
		img->fill(Qt::transparent);

		painter->setPen(QPen(theme.darkLabelColor));
		painter->setFont(theme.axisLabelFont);
		for (const Label &label: labels) {
			double x = (label.pos - min) / (max - min) * size + offset.x() - round(label.width / 2.0);
			QRectF rect(x, 0.0, label.width, fontHeight);
			painter->drawText(rect, label.label);
		}
		if (!title.isEmpty()) {
			QRectF rect(offset.x() + round((size - titleWidth) / 2.0),
				    fontHeight + axisTitleSpaceHorizontal,
				    titleWidth, fontHeight);
			painter->setFont(theme.axisTitleFont);
			painter->drawText(rect, title);
		}
	} else {
		double pixmapWidth = labelWidth + titleSpace();
		double offsetX = pixmapWidth + axisLabelSpaceVertical + (labelsBetweenTicks ? 0.0 : axisTickSizeVertical);

		double pixmapHeight = ceil(size + axisTickWidth);
		double offsetY = size;
		if (!labels.empty() && !labelsBetweenTicks) {
			pixmapHeight += fontHeight;
			offsetY += fontHeight / 2.0;
		}

		ChartPixmapItem::resize(QSizeF(pixmapWidth, pixmapHeight)); // Note: this rounds the dimensions up
		offset = QPointF(round(offsetX), round(offsetY));
		img->fill(Qt::transparent);

		painter->setPen(QPen(theme.darkLabelColor));
		painter->setFont(theme.axisLabelFont);
		for (const Label &label: labels) {
			double y = (min - label.pos) / (max - min) * size + offset.y() - round(fontHeight / 2.0);
			QRectF rect(pixmapWidth - label.width, y, label.width, fontHeight);
			painter->drawText(rect, label.label);
		}
		if (!title.isEmpty()) {
			painter->rotate(-90.0);
			QRectF rect(round(-(offsetY + titleWidth) / 2.0), 0.0, titleWidth, fontHeight);
			painter->setFont(theme.axisTitleFont);
			painter->drawText(rect, title);
			painter->resetTransform();
		}
	}
}

void StatsAxis::setPos(QPointF pos)
{
	zeroOnScreen = horizontal ? pos.x() : pos.y();
	ChartPixmapItem::setPos(pos - offset);

	if (horizontal) {
		double y = pos.y();
		for (const Tick &tick: ticks) {
			double x = toScreen(tick.pos);
			tick.item->setLine(QPointF(x, y), QPointF(x, y + axisTickSizeHorizontal));
		}
		line->setLine(QPointF(zeroOnScreen, y), QPointF(zeroOnScreen + size, y));
	} else {
		double x = pos.x();
		for (const Tick &tick: ticks) {
			double y = toScreen(tick.pos);
			tick.item->setLine(QPointF(x, y), QPointF(x - axisTickSizeVertical, y));
		}
		line->setLine(QPointF(x, zeroOnScreen), QPointF(x, zeroOnScreen - size));
	}
}

ValueAxis::ValueAxis(StatsView &view, const QString &title, double min, double max, int decimals, bool horizontal) :
	StatsAxis(view, title, horizontal, false),
	min(min), max(max), decimals(decimals)
{
	// Avoid degenerate cases
	if (max - min < 0.0001) {
		max += 0.5;
		min -= 0.5;
	}
}

// Attention: this is only heuristics. Before setting the actual size, we
// don't know the actual numbers of the minimum and maximum value.
std::pair<QString, QString> ValueAxis::getFirstLastLabel() const
{
	QLocale loc;
	return { loc.toString(min, 'f', decimals), loc.toString(max, 'f', decimals) };
}

void ValueAxis::updateLabels()
{
	QLocale loc;
	auto [minString, maxString] = getFirstLastLabel();
	int numTicks = guessNumTicks({ std::move(minString), std::move(maxString)});

	// Use full decimal increments
	double height = max - min;
	double inc = height / numTicks;
	double digits = std::max(floor(log10(inc)), static_cast<double>(-decimals));
	int digits_int = lrint(digits);
	double digits_factor = pow(10.0, digits);
	int inc_int = std::max((int)ceil(inc / digits_factor), 1);
	// Do "nice" increments of the leading digit: 1, 2, 4, 5.
	if (inc_int > 5)
		inc_int = 10;
	if (inc_int == 3)
		inc_int = 4;
	inc = inc_int * digits_factor;
	int show_decimals = std::max(-digits_int, decimals);

	double actMin = floor(min / inc) * inc;
	double actMax = ceil(max / inc) * inc;
	if (actMax - actMin < inc) {
		actMax += inc;
		actMin -= inc;
	}
	int num = lrint((actMax - actMin) / inc);
	setRange(actMin, actMax);

	double actStep = (actMax - actMin) / static_cast<double>(num);
	double act = actMin;
	labels.reserve(num + 1);
	ticks.reserve(num + 1);
	QFontMetrics fm(theme.axisLabelFont);
	for (int i = 0; i <= num; ++i) {
		addLabel(fm, loc.toString(act, 'f', show_decimals), act);
		addTick(act);
		act += actStep;
	}
}

CountAxis::CountAxis(StatsView &view, const QString &title, int count, bool horizontal) :
	ValueAxis(view, title, 0.0, (double)count, 0, horizontal),
	count(count)
{
}

// Attention: this is only heuristics. Before setting the actual size, we
// don't know the actual numbers of the minimum and maximum value.
std::pair<QString, QString> CountAxis::getFirstLastLabel() const
{
	QLocale loc;
	return { QString("0"), loc.toString(count) };
}

void CountAxis::updateLabels()
{
	QLocale loc;
	QString countString = loc.toString(count);
	int numTicks = guessNumTicks({ countString });

	// Get estimate of step size
	if (count <= 0)
		count = 1;
	// When determining the step size, make sure to round up
	int step = (count + numTicks - 1) / numTicks;
	if (step <= 0)
		step = 1;

	// Get the significant first or first two digits
	int scale = 1;
	int significant = step;
	while (significant > 25) {
		significant /= 10;
		scale *= 10;
	}

	for (int increment: { 1, 2, 4, 5, 10, 15, 20, 25 }) {
		if (increment >= significant) {
			significant = increment;
			break;
		}
	}
	step = significant * scale;

	// Make maximum an integer number of steps, equal or greater than the needed counts
	int num = (count - 1) / step + 1;
	int max = num * step;

	setRange(0, max);

	labels.reserve(max + 1);
	ticks.reserve(max + 1);
	QFontMetrics fm(theme.axisLabelFont);
	for (int i = 0; i <= max; i += step) {
		addLabel(fm, loc.toString(i), static_cast<double>(i));
		addTick(static_cast<double>(i));
	}
}

CategoryAxis::CategoryAxis(StatsView &view, const QString &title, const std::vector<QString> &labels, bool horizontal) :
	StatsAxis(view, title, horizontal, true),
	labelsText(labels)
{
	if (!labels.empty())
		setRange(-0.5, static_cast<double>(labels.size()) - 0.5);
	else
		setRange(-1.0, 1.0);
}

// No implementation because the labels are inside ticks and this
// is only used to calculate the "overhang" of labels under ticks.
std::pair<QString, QString> CategoryAxis::getFirstLastLabel() const
{
	return { QString(), QString() };
}

// Get ellipses for a given label size
static QString ellipsis1 = QStringLiteral("․");
static QString ellipsis2 = QStringLiteral("‥");
static QString ellipsis3 = QStringLiteral("…");
static QString getEllipsis(const QFontMetrics &fm, double widthIn)
{
	int width = static_cast<int>(floor(widthIn));
	if (fm.size(Qt::TextSingleLine, ellipsis3).width() < width)
		return ellipsis3;
	if (fm.size(Qt::TextSingleLine, ellipsis2).width() < width)
		return ellipsis2;
	if (fm.size(Qt::TextSingleLine, ellipsis1).width() < width)
		return ellipsis1;
	return QString();
}

void CategoryAxis::updateLabels()
{
	if (labelsText.empty())
		return;

	QFontMetrics fm(theme.axisLabelFont);
	double size_per_label = size / static_cast<double>(labelsText.size()) - axisTickWidth;
	double fontHeight = fm.height();

	QString ellipsis = horizontal ? getEllipsis(fm, size_per_label) : QString();

	labels.reserve(labelsText.size());
	ticks.reserve(labelsText.size() + 1);
	double pos = 0.0;
	addTick(-0.5);
	for (const QString &s: labelsText) {
		if (horizontal) {
			double width = static_cast<double>(fm.size(Qt::TextSingleLine, s).width());
			addLabel(fm, width < size_per_label ? s : ellipsis, pos);
		} else {
			if (fontHeight < size_per_label)
				addLabel(fm, s, pos);
		}
		addTick(pos + 0.5);
		pos += 1.0;
	}
}

HistogramAxis::HistogramAxis(StatsView &view, const QString &title, std::vector<HistogramAxisEntry> bins, bool horizontal) :
	StatsAxis(view, title, horizontal, false),
	bin_values(std::move(bins))
{
	if (bin_values.size() < 2) // Less than two makes no sense -> there must be at least one category
		return;

	// The caller can declare some bin labels as preferred, when there are
	// too many labels to show all. Try to infer the preferred step size
	// by finding two consecutive preferred labels. This supposes that
	// the preferred labels are equi-distant and that the caller does not
	// use large prime (or nearly prime) steps.
	auto it1 = std::find_if(bin_values.begin(), bin_values.end(),
				[](const HistogramAxisEntry &e) { return e.recommended; });
	auto next_it = it1 == bin_values.end() ? it1 : std::next(it1);
	auto it2 = std::find_if(next_it, bin_values.end(),
				[](const HistogramAxisEntry &e) { return e.recommended; });
	preferred_step = it2 == bin_values.end() ? 1 : it2 - it1;
	setRange(bin_values.front().value, bin_values.back().value);
}

std::pair<QString, QString> HistogramAxis::getFirstLastLabel() const
{
	if (bin_values.empty())
		return { QString(), QString() };
	else
		return { bin_values.front().name, bin_values.back().name };
}

// Initialize a histogram axis with the given labels. Labels are specified as (name, value, recommended) triplets.
// If labels are skipped, try to skip it in such a way that a recommended label is shown.
// The one example where this is relevant is the quarterly bins, which are formated as (2019, q1, q2, q3, 2020, ...).
// There, we obviously want to show the years and not the quarters.
void HistogramAxis::updateLabels()
{
	if (bin_values.size() < 2) // Less than two makes no sense -> there must be at least one category
		return;

	std::vector<QString> strings;
	strings.reserve(bin_values.size());
	for (auto &[name, value, recommended]: bin_values)
		strings.push_back(name);
	int maxLabels = guessNumTicks(strings);

	int step = ((int)bin_values.size() - 1) / maxLabels + 1;
	if (step < preferred_step) {
		if (step * 2 > preferred_step)  {
			step = preferred_step;
		} else {
			int gcd = std::gcd(step, preferred_step);
			while (preferred_step % step != 0)
				step += gcd;
		}
	} else if (step > preferred_step) {
		int remainder = (step + preferred_step) % preferred_step;
		if (remainder != 0)
			step = step + preferred_step - remainder;
	}
	int first = 0;
	if (step > 1) {
		for (int i = 0; i < (int)bin_values.size(); ++i) {
			const auto &[name, value, recommended] = bin_values[i];
			if (recommended) {
				first = i % step;
				break;
			}
		}
	}
	labels.reserve((bin_values.size() - first) / step + 1);
	// Always add a tick at the beginning of the axis - this is
	// important for the grid, which uses the ticks.
	if (first != 0)
		addTick(bin_values.front().value);
	int last = first;
	QFontMetrics fm(theme.axisLabelFont);
	for (int i = first; i < (int)bin_values.size(); i += step) {
		const auto &[name, value, recommended] = bin_values[i];
		addLabel(fm, name, value);
		addTick(value);
		last = i;
	}
	// Always add a tick at the end of the axis (see above).
	if (last != (int)bin_values.size() - 1)
		addTick(bin_values.back().value);
}

// Helper function to turn days since "Unix epoch" into a timestamp_t
static const double seconds_in_day = 86400.0;
static timestamp_t double_to_timestamp(double d)
{
	return timestamp_t{ lrint(d * seconds_in_day) };
}

// Turn double to (year, month) pair
static std::pair<int, int> double_to_month(double d)
{
	struct tm tm;
	utc_mkdate(double_to_timestamp(d), &tm);
	return { tm.tm_year, tm.tm_mon };
}

// Increase (year, month) pair by one month
static void inc(std::pair<int, int> &ym)
{
	if (++ym.second >= 12) {
		++ym.first;
		ym.second = 0;
	}
}

static std::array<int, 3> double_to_day(double d)
{
	struct tm tm;
	utc_mkdate(double_to_timestamp(d), &tm);
	return { tm.tm_year, tm.tm_mon, tm.tm_mday };
}

// This is trashy: to increase a day, turn into timestamp and back.
// This surely can be done better.
static void inc(std::array<int, 3> &ymd)
{
	struct tm tm = { 0 };
	tm.tm_year = ymd[0];
	tm.tm_mon = ymd[1];
	tm.tm_mday = ymd[2] + 1;
	timestamp_t t = utc_mktime(&tm);
	utc_mkdate(t, &tm);
	ymd = { tm.tm_year, tm.tm_mon, tm.tm_mday };
}

// Use heuristics to determine the preferred day/month format:
// Try to see whether day or month comes first and try to extract
// the separator character. Returns a (day_first, separator) pair.
static std::pair<bool, char> day_format()
{
	const char *fmt = prefs.date_format;
	const char *d, *m, *sep;
	for (d = fmt; *d && *d != 'd' && *d != 'D'; ++d)
		;
	for (m = fmt; *m && *m != 'm' && *m != 'M'; ++m)
		;
	for(sep = std::min(m, d); *sep == 'm' || *sep == 'M' || *sep == 'd' || *sep == 'D'; ++sep)
		;
	return { d < m, *sep ? *sep : '.' };
}

// For now, misuse the histogram axis for creating a time axis. Depending on the range,
// create year, month or day-based bins. This is certainly not efficient and may need
// some tuning. However, it should ensure that no crazy number of bins is generated.
// Ultimately, this should be replaced by a better and dynamic scheme
// From and to are given in days since "epoch".
static std::vector<HistogramAxisEntry> timeRangeToBins(double from, double to)
{
	// from and two are given in days since the "Unix epoch".
	// The lowest precision we do is two days.
	if (to - from < 2.0) {
		double center = (from + to) / 2.0;
		from = center - 1.0;
		to = center + 1.0;
	}

	std::vector<HistogramAxisEntry> res;
	if (to - from > 2.0 * 356.0) {
		// For two years or more, do year based bins
		int year_from = utc_year(double_to_timestamp(from));
		int year_to = utc_year(double_to_timestamp(to)) + 1;
		for (int year = year_from; year <= year_to; ++year)
			res.push_back({ QString::number(year), date_to_double(year, 0, 0), true });
	} else if (to - from > 2.0 * 30.0) {
		// For two months or more, do month based bins
		auto year_month_from = double_to_month(from);
		auto year_month_to = double_to_month(to);
		inc(year_month_to);
		for (auto act = year_month_from; act <= year_month_to; inc(act)) {
			double val = date_to_double(act.first, act.second, 0);
			if (act.second == 0)
				res.push_back({ QString::number(act.first), val, true });
			else
				res.push_back({ monthname(act.second), val, false });
		}
	} else {
		// For less than two months, do date based bins
		auto day_from = double_to_day(from);
		auto day_to = double_to_day(to);
		inc(day_to);
		auto [day_before_month, separator] = day_format();
		QString format = day_before_month ? QStringLiteral("%1%2%3")
						  : QStringLiteral("%3%2%1");
		QString sep = QString(separator);
		// Attention: In a histogramm axis, we must add one more entries than
		// histogram bins. The entries are the values *between* the histograms.
		for (auto act = day_from; act <= day_to; inc(act)) {
			double val = date_to_double(act[0], act[1], act[2]);
			if (act[1] == 0) {
				res.push_back({ QString::number(act[0]), val, true });
			} else if (act[2] == 0) {
				res.push_back({ monthname(act[1]), val, true });
			} else {
				QString s = format.arg(QString::number(act[2]), sep, QString::number(act[1] + 1));
				res.push_back({s, val, true });
			}
		}
	}
	return res;
}

DateAxis::DateAxis(StatsView &view, const QString &title, double from, double to, bool horizontal) :
	HistogramAxis(view, title, timeRangeToBins(from, to), horizontal)
{
}
