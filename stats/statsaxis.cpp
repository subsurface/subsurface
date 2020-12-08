// SPDX-License-Identifier: GPL-2.0
#include "statsaxis.h"
#include "statstypes.h"
#include "statstranslations.h"
#include "core/pref.h"
#include "core/subsurface-time.h"
#include <math.h> // for lrint
#include <QChart>
#include <QFontMetrics>
#include <QLocale>

StatsAxis::StatsAxis(bool horizontal) : horizontal(horizontal)
{
}

StatsAxis::~StatsAxis()
{
}

std::pair<double, double> StatsAxis::minMax() const
{
	return { 0.0, 1.0 };
}

// Guess the number of tick marks based on example strings.
// We will use minimum and maximum values, which are not necessarily the
// maximum-size strings especially, when using proportional fonts or for
// categorical data. Therefore, try to err on the safe side by adding enough
// margins.
int StatsAxis::guessNumTicks(const QtCharts::QChart *chart, const QtCharts::QAbstractAxis *axis, const std::vector<QString> &strings) const
{
	QFont font = axis->labelsFont();
	QFontMetrics fm(font);
	int minSize = fm.height();
	for (const QString &s: strings) {
		QSize size = fm.size(Qt::TextSingleLine, s);
		int needed = horizontal ? size.width() : size.height();
		if (needed > minSize)
			minSize = needed;
	}

	// Add space between labels
	if (horizontal)
		minSize = minSize * 3 / 2;
	else
		minSize *= 2;
	QRectF chartSize = chart->plotArea();
	double availableSpace = horizontal ? chartSize.width() : chartSize.height();
	int numTicks = lrint(availableSpace / minSize);
	return std::max(numTicks, 2);
}

ValueAxis::ValueAxis(double min, double max, int decimals, bool horizontal) : StatsAxisTemplate(horizontal),
	min(min), max(max), decimals(decimals)
{
}

std::pair<double, double> ValueAxis::minMax() const
{
	return { QValueAxis::min(), QValueAxis::max() };
}

static QString makeFormatString(int decimals)
{
	return QStringLiteral("%.%1f").arg(decimals < 0 ? 0 : decimals);
}

void ValueAxis::updateLabels(const QtCharts::QChart *chart)
{
	using QtCharts::QValueAxis;

	// Avoid degenerate cases
	if (max - min < 0.0001) {
		max = 0.5;
		min = -0.5;
	}

	QLocale loc;
	QString minString = loc.toString(min, 'f', decimals);
	QString maxString = loc.toString(max, 'f', decimals);
	int numTicks = guessNumTicks(chart, this, { minString, maxString});

	// Use full decimal increments
	double height = max - min;
	double inc = height / numTicks;
	double digits = floor(log10(inc));
	int digits_int = lrint(digits);
	double digits_factor = pow(10.0, digits);
	int inc_int = std::max((int)ceil(inc / digits_factor), 1);
	// Do "nice" increments: 1, 2, 4, 5.
	if (inc_int > 5)
		inc_int = 10;
	if (inc_int == 3)
		inc_int = 5;
	inc = inc_int * digits_factor;
	if (-digits_int > decimals)
		decimals = -digits_int;

	setLabelFormat(makeFormatString(decimals));
	double actMin = floor(min /  inc) * inc;
	double actMax = ceil(max /  inc) * inc;
	int num = lrint((actMax - actMin) / inc);
	setRange(actMin, actMax);
	setTickCount(num + 1);
}

CountAxis::CountAxis(int count, bool horizontal) : ValueAxis(0.0, (double)count, 0, horizontal),
	count(count)
{
}

void CountAxis::updateLabels(const QtCharts::QChart *chart)
{
	QLocale loc;
	QString countString = loc.toString(count);
	int numTicks = guessNumTicks(chart, this, { countString });

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
	numTicks = num + 1; // There is one more tick than steps

	setLabelFormat("%.0f");
	setRange(0, max);
	setTickCount(numTicks);
}

CategoryAxis::CategoryAxis(const std::vector<QString> &labels, bool horizontal) : StatsAxisTemplate(horizontal)
{
	for (const QString &s: labels)
		append(s);
}

void CategoryAxis::updateLabels(const QtCharts::QChart *)
{
}

// A small helper class that makes strings unique. We need this,
// because QCategoryAxis can only handle unique category names.
// Disambiguate strings by adding unicode zero-width spaces.
// Keep track of a list of strings and how many spaces have to
// be added.
class LabelDisambiguator {
	using Pair = std::pair<QString, int>;
	std::vector<Pair> entries;
public:
	QString transmogrify(const QString &s);
};

QString LabelDisambiguator::transmogrify(const QString &s)
{
	auto it = std::find_if(entries.begin(), entries.end(),
			       [&s](const Pair &p) { return p.first == s; });
	if (it == entries.end()) {
		entries.emplace_back(s, 0);
		return s;
	}  else {
		++(it->second);
		return s + QString(it->second, QChar(0x200b));
	}
}

HistogramAxis::HistogramAxis(std::vector<HistogramAxisEntry> bins, bool horizontal) : StatsAxisTemplate(horizontal),
	bin_values(bins)
{
	if (bin_values.size() < 2) // Less than two makes no sense -> there must be at least one category
		return;

	LabelDisambiguator labeler;
	for (HistogramAxisEntry &entry: bin_values)
		entry.name = labeler.transmogrify(entry.name);

	setMin(bin_values.front().value);
	setMax(bin_values.back().value);
	setStartValue(bin_values.front().value);
	setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
}

std::pair<double, double> HistogramAxis::minMax() const
{
	if (bin_values.size() < 2) // Less than two makes no sense -> there must be at least one category
		return { 0.0, 1.0 };
	return { QValueAxis::min(), QValueAxis::max() };
}

// Initialize a histogram axis with the given labels. Labels are specified as (name, value, recommended) triplets.
// If labels are skipped, try to skip it in such a way that a recommended label is shown.
// The one example where this is relevant is the quarterly bins, which are formated as (2019, q1, q2, q3, 2020, ...).
// There, we obviously want to show the years and not the quarters.
void HistogramAxis::updateLabels(const QtCharts::QChart *chart)
{
	if (bin_values.size() < 2) // Less than two makes no sense -> there must be at least one category
		return;

	// There is no clear all labels function in QCategoryAxis!? You must be kidding.
	for (const QString &label: categoriesLabels())
		remove(label);
	if (count() > 0)
		qWarning("HistogramAxis::updateLabels(): labels left after clearing!?");

	std::vector<QString> strings;
	strings.reserve(bin_values.size());
	for (auto &[name, value, recommended]: bin_values)
		strings.push_back(name);
	int maxLabels = guessNumTicks(chart, this, strings);

	int step = ((int)bin_values.size() - 1) / maxLabels + 1;
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
	for (int i = first; i < (int)bin_values.size(); i += step) {
		const auto &[name, value, recommended] = bin_values[i];
		append(name, value);
	}
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
// From and to are given in seconds since "epoch".
static std::vector<HistogramAxisEntry> timeRangeToBins(double from, double to)
{
	// from and two are given in days since the "Unix epoch".
	// The lowest precision we do is two days.
	if (to - from < 2.0) {
		double center = (from + to) / 2.0;
		from = center + 1.0;
		to = center - 1.0;
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
		for (auto act = day_from; act < day_to; inc(act)) {
			double val = date_to_double(act[0], act[1], act[2]);
			if (act[1] == 0) {
				res.push_back({ QString::number(act[0]), val, true });
			} else if (act[2] == 0) {
				res.push_back({ monthname(act[1]), val, true });
			} else {
				QString s = format.arg(QString::number(act[2]), sep, QString::number(act[1]));
				res.push_back({s, val, true });
			}
		}
	}
	return res;
}

DateAxis::DateAxis(double from, double to, bool horizontal) :
	HistogramAxis(timeRangeToBins(from, to), horizontal)
{
}
