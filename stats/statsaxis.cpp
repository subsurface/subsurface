// SPDX-License-Identifier: GPL-2.0
#include "statsaxis.h"
#include "statstranslations.h"
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

CountAxis::CountAxis(int count, bool horizontal) : StatsAxisTemplate(horizontal), count(count)
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
	setTitleText(StatsTranslations::tr("No. dives"));
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

HistogramAxis::HistogramAxis(std::vector<HistogramAxisEntry> bin_values, bool horizontal) : StatsAxisTemplate(horizontal),
	bin_values(bin_values)
{
	if (bin_values.size() < 2) // Less than two makes no sense -> there must be at least one category
		return;
	setMin(bin_values.front().value);
	setMax(bin_values.back().value);
	setStartValue(bin_values.front().value);
	setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
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
