// SPDX-License-Identifier: GPL-2.0
#include "statsview.h"
#include "statstypes.h"
#include "core/divefilter.h"
#include <QQuickItem>
#include <QAreaSeries>
#include <QBarCategoryAxis>
#include <QBarSet>
#include <QBarSeries>
#include <QBoxPlotSeries>
#include <QCategoryAxis>
#include <QChart>
#include <QGraphicsSimpleTextItem>
#include <QHorizontalBarSeries>
#include <QHorizontalStackedBarSeries>
#include <QLineSeries>
#include <QLocale>
#include <QPieSeries>
#include <QScatterSeries>
#include <QStackedBarSeries>
#include <QValueAxis>

// Some of our compilers do not support std::size(). Roll our own for now.
template <typename T, size_t N>
static constexpr size_t array_size(const T (&)[N])
{
	return N;
}

// Constants that control the graph layouts
static const double barTopSpace = 0.05; // 0.01 = one percent more than needed for longest bar
static const double histogramBarWidth = 0.8; // 1.0 = full width of category

enum class ChartSubType {
	Vertical = 0,
	VerticalStacked,
	Horizontal,
	HorizontalStacked,
	Pie
};

// Attn: The order must correspond to the enum above
static const char *chart_subtype_names[] = {
	QT_TRANSLATE_NOOP("StatsTranslations", "Vertical"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Vertical stacked"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Horizontal"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Horizontal stacked"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Pie")
};

enum class ChartTypeId {
	DiscreteBar,
	DiscreteValue,
	DiscreteCount,
	DiscreteBox,
	DiscreteScatter,
	HistogramCount,
	HistogramBar,
	ScatterPlot
};

static const struct ChartType {
	ChartTypeId id;
	const char *name;
	const std::vector<const StatsType *> &firstAxisTypes;
	const std::vector<const StatsType *> &secondAxisTypes;	// empty if no second axis supported
	bool firstAxisBinned;
	bool secondAxisBinned;
	bool firstAxisHasOperations;
	bool secondAxisHasOperations;
	const std::vector<ChartSubType> subtypes;
} chart_types[] = {
	{
		ChartTypeId::DiscreteBar,
		QT_TRANSLATE_NOOP("StatsTranslations", "Discrete bar"),
		stats_types,	// supports all types as first axis
		stats_types,	// supports all types as second axis
		true,
		true,
		false,
		false,
		{ ChartSubType::Vertical, ChartSubType::VerticalStacked, ChartSubType::Horizontal, ChartSubType::HorizontalStacked }
	},
	{
		ChartTypeId::DiscreteValue,
		QT_TRANSLATE_NOOP("StatsTranslations", "Discrete value"),
		stats_types,		// supports all types as first axis
		stats_numeric_types,	// supports numeric types as second axis, since we want to calculate the mean, etc
		true,
		false,
		false,
		true,
		{ ChartSubType::Vertical, ChartSubType::Horizontal }
	},
	{
		ChartTypeId::DiscreteCount,
		QT_TRANSLATE_NOOP("StatsTranslations", "Discrete count"),
		stats_types,	// supports all types as first axis
		{},		// no second axis
		true,
		false,
		false,
		false,
		{ ChartSubType::Vertical, ChartSubType::Horizontal, ChartSubType::Pie }
	},
	{
		ChartTypeId::DiscreteBox,
		QT_TRANSLATE_NOOP("StatsTranslations", "Discrete box"),
		stats_types,		// supports all types as first axis
		stats_numeric_types,	// supports numeric types as second axis, since we want to calculate quartiles
		true,
		false,
		false,
		false,
		{ }
	},
	{
		ChartTypeId::DiscreteScatter,
		QT_TRANSLATE_NOOP("StatsTranslations", "Discrete scatter"),
		stats_types,	// supports all types as first axis
		stats_numeric_types,	// supports numeric types as second axis, since we want to calculate the mean, etc
		true,
		false,
		false,
		false,
		{ }
	},
	{
		ChartTypeId::HistogramCount,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram counts"),
		stats_continuous_types,	// supports continuous types as first axis
		{},			// no second axis
		true,
		false,
		false,
		false,
		{ ChartSubType::Vertical, ChartSubType::Horizontal }
	},
	{
		ChartTypeId::HistogramBar,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram bar"),
		stats_continuous_types,	// supports continuous types as first axis
		stats_numeric_types,	// supports numeric types as second axis, since we want to calculate the mean, etc
		true,
		false,
		false,
		true,
		{ ChartSubType::Vertical, ChartSubType::Horizontal }
	},
	{
		ChartTypeId::ScatterPlot,
		QT_TRANSLATE_NOOP("StatsTranslations", "Scatter plot"),
		stats_numeric_types,	// plot any numeric type against any other numeric type
		stats_numeric_types,	// for time-based charts, there is a different plot
		false,
		false,
		false,
		false,
		{ }
	}
};

// returns null on out-of-bound
static const ChartType *idxToChartType(int idx)
{
	return idx < 0 || idx >= (int)array_size(chart_types) ?
		nullptr : &chart_types[idx];
}

// returns null on out-of-bound
static const StatsType *idxToFirstAxisType(int chartType, int firstAxis)
{
	const ChartType *t = idxToChartType(chartType);
	return !t || firstAxis < 0 || firstAxis >= (int)t->firstAxisTypes.size() ?
		nullptr : t->firstAxisTypes[firstAxis];
}

QStringList StatsView::getChartTypes()
{
	QStringList res;
	res.reserve(array_size(chart_types));
	for (const ChartType &t: chart_types)
		res.push_back(StatsTranslations::tr(t.name));
	return res;
}

QStringList StatsView::getChartSubTypes(int chartType)
{
	const ChartType *t = idxToChartType(chartType);
	if (!t)
		return {};

	QStringList res;
	res.reserve(t->subtypes.size());
	for (ChartSubType subtype: t->subtypes)
		res.push_back(StatsTranslations::tr(chart_subtype_names[(int)subtype]));
	return res;
}

QStringList StatsView::getFirstAxisTypes(int chartType)
{
	const ChartType *t = idxToChartType(chartType);
	if (!t)
		return {};

	QStringList res;
	res.reserve(t->firstAxisTypes.size());
	for (const StatsType *type: t->firstAxisTypes)
		res.push_back(type->name());
	return res;
}

static QStringList statsTypeToBinnerNames(const StatsType *t)
{
	if (!t)
		return {};
	std::vector<const StatsBinner *> binners = t->binners();
	QStringList res;
	res.reserve(binners.size());
	for (const StatsBinner *binner: binners)
		res.push_back(binner->name());
	return res;
}

QStringList StatsView::getFirstAxisBins(int chartType, int firstAxis)
{
	const ChartType *t = idxToChartType(chartType);
	if (!t || !t->firstAxisBinned)
		return {};
	return statsTypeToBinnerNames(idxToFirstAxisType(chartType, firstAxis));
}

QStringList StatsView::getFirstAxisOperations(int chartType, int firstAxis)
{
	const ChartType *t = idxToChartType(chartType);
	const StatsType *first = idxToFirstAxisType(chartType, firstAxis);
	if (!t || !t->firstAxisHasOperations || !first)
		return {};
	return first->supportedOperationNames();
}

// Getting the second axis is a bit special: it removes the first axis
// from the list, as plotting the same value against itself makes no sense.
static std::vector<const StatsType *> calcSecondAxisTypes(int chartType, int firstAxis)
{
	const ChartType *t = idxToChartType(chartType);
	const StatsType *first = idxToFirstAxisType(chartType, firstAxis);
	if (!t || !first)
		return {};
	const std::vector<const StatsType *> &types = t->secondAxisTypes;
	std::vector<const StatsType *> res;
	res.reserve(types.size());
	for (const StatsType *t: types) {
		if (t != first)
			res.push_back(t);
	}
	return res;
}

// returns null on out-of-bound
static const StatsType *idxToSecondAxisType(int chartType, int firstAxis, int secondAxis)
{
	std::vector<const StatsType *> secondAxisTypes = calcSecondAxisTypes(chartType, firstAxis);
	return secondAxis < 0 || secondAxis >= (int)secondAxisTypes.size() ?
		nullptr : secondAxisTypes[secondAxis];
}

QStringList StatsView::getSecondAxisTypes(int chartType, int firstAxis)
{
	std::vector<const StatsType *> types = calcSecondAxisTypes(chartType, firstAxis);
	QStringList res;
	res.reserve(types.size());
	for (const StatsType *t: types)
		res.push_back(t->name());

	return res;
}

QStringList StatsView::getSecondAxisBins(int chartType, int firstAxis, int secondAxis)
{
	const ChartType *t = idxToChartType(chartType);
	if (!t || !t->secondAxisBinned)
		return {};
	return statsTypeToBinnerNames(idxToSecondAxisType(chartType, firstAxis, secondAxis));
}

QStringList StatsView::getSecondAxisOperations(int chartType, int firstAxis, int secondAxis)
{
	const ChartType *t = idxToChartType(chartType);
	const StatsType *second = idxToSecondAxisType(chartType, firstAxis, secondAxis);
	if (!t || !t->secondAxisHasOperations || !second)
		return {};
	return second->supportedOperationNames();
}

static const QUrl urlStatsView = QUrl(QStringLiteral("qrc:/qml/statsview.qml"));

// We use QtQuick's ChartView so that we can show the statistics on mobile.
// However, accessing the ChartView from C++ is maliciously cumbersome and
// the full QChart interface is not exported. Fortunately, the interface
// leaks the QChart object: We can create a dummy-series and access the chart
// object via the chart() accessor function. By creating a "PieSeries", the
// ChartView does not automatically add axes.
static QtCharts::QChart *getChart(QQuickItem *item)
{
	QtCharts::QAbstractSeries *abstract_series;
	if (!item)
		return nullptr;
	if (!QMetaObject::invokeMethod(item, "createSeries", Qt::AutoConnection,
				       Q_RETURN_ARG(QtCharts::QAbstractSeries *, abstract_series),
				       Q_ARG(int, QtCharts::QAbstractSeries::SeriesTypePie),
				       Q_ARG(QString, QString()))) {
		qWarning("Couldn't call createSeries()");
		return nullptr;
	}
	QtCharts::QChart *res = abstract_series->chart();
	res->removeSeries(abstract_series);
	delete abstract_series;
	return res;
}

StatsView::StatsView(QWidget *parent) : QQuickWidget(parent)
{
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(urlStatsView);
	chart = getChart(rootObject());
	connect(chart, &QtCharts::QChart::plotAreaChanged, this, &StatsView::plotAreaChanged);
}

StatsView::~StatsView()
{
}

void StatsView::plotAreaChanged(const QRectF &)
{
	for (BarLabel &label: barLabels)
		label.updatePosition();
}

void StatsView::initSeries(QtCharts::QAbstractSeries *series, const QString &name)
{
	series->setName(name);
	chart->addSeries(series);
	if (axes.size() >= 2) {
		// Not all charts have axes (e.g. Pie charts)
		series->attachAxis(axes[0].get());
		series->attachAxis(axes[1].get());
	}
}

template<typename Type>
Type *StatsView::addSeries(const QString &name)
{
	Type *res = new Type;
	initSeries(res, name);
	return res;
}

void StatsView::showLegend()
{
	QtCharts::QLegend *legend = chart->legend();
	if (!legend)
		return;
	legend->setVisible(true);
	legend->setAlignment(Qt::AlignBottom);
}

void StatsView::hideLegend()
{
	QtCharts::QLegend *legend = chart->legend();
	if (!legend)
		return;
	legend->setVisible(false);
}

void StatsView::setTitle(const QString &s)
{
	chart->setTitle(s);
}

template <typename T>
T *StatsView::makeAxis()
{
	T *res = new T;
	axes.emplace_back(res);
	return res;
}

void StatsView::addAxes(QtCharts::QAbstractAxis *x, QtCharts::QAbstractAxis *y)
{
	chart->addAxis(x, Qt::AlignBottom);
	chart->addAxis(y, Qt::AlignLeft);
}

void StatsView::reset()
{
	if (!chart)
		return;
	barLabels.clear();
	chart->removeAllSeries();
	axes.clear();
}

void StatsView::plot(int type, int subTypeIdx,
		     int firstAxis, int firstAxisBin, int firstAxisOperation,
		     int secondAxis, int secondAxisBin, int secondAxisOperation)
{
	if (!chart)
		return;
	reset();

	const ChartType *t = idxToChartType(type);
	const StatsType *firstAxisType = idxToFirstAxisType(type, firstAxis);
	const StatsType *secondAxisType = idxToSecondAxisType(type, firstAxis, secondAxis);
	if (!t || !firstAxisType)
		return;
	if (t->secondAxisTypes.size() > 0 && !secondAxisType)
		return;

	ChartSubType subType = subTypeIdx >= 0 && subTypeIdx < (int)t->subtypes.size() ?
		t->subtypes[subTypeIdx] : ChartSubType::Vertical;
	const StatsBinner *firstAxisBinner = t->firstAxisBinned ? firstAxisType->getBinner(firstAxisBin) : nullptr;
	const StatsBinner *secondAxisBinner = t->secondAxisBinned ? secondAxisType->getBinner(secondAxisBin) : nullptr;

	const std::vector<dive *> dives = DiveFilter::instance()->visibleDives();
	switch (t->id) {
	case ChartTypeId::DiscreteBar:
		return plotBarChart(dives, subType, firstAxisType, firstAxisBinner, secondAxisType, secondAxisBinner);
	case ChartTypeId::DiscreteValue:
		return plotValueChart(dives, subType, firstAxisType, firstAxisBinner, secondAxisType,
				      secondAxisType->idxToOperation(secondAxisOperation));
	case ChartTypeId::DiscreteCount:
		return plotDiscreteCountChart(dives, subType, firstAxisType, firstAxisBinner);
	case ChartTypeId::DiscreteBox:
		return plotDiscreteBoxChart(dives, firstAxisType, firstAxisBinner, secondAxisType);
	case ChartTypeId::DiscreteScatter:
		return plotDiscreteScatter(dives, firstAxisType, firstAxisBinner, secondAxisType);
	case ChartTypeId::HistogramCount:
		return plotHistogramCountChart(dives, subType, firstAxisType, firstAxisBinner);
	case ChartTypeId::HistogramBar:
		return plotHistogramBarChart(dives, subType, firstAxisType, firstAxisBinner, secondAxisType,
					     secondAxisType->idxToOperation(secondAxisOperation));
	case ChartTypeId::ScatterPlot:
		return plotScatter(dives, firstAxisType, secondAxisType);
	default:
		qWarning("Unknown chart type: %d", (int)t->id);
		return;
	}
}

static int initCountAxis(QtCharts::QValueAxis *axis, int count)
{
	// TODO: Let the acceptable number of ticks depend on the size of the graph.
	int numTicksMin = 5;
	int numTicksMax = 8;
	int max, numTicks;

	// A fixed number of ticks is not nice if the maximum count not divisible
	// by this number. In the worst case, our maximum count may be prime.
	// Therefore make the maximum divisible by the number of ticks.
	// This is algorithm is very stupid: it increases the maximum until it is
	// a multiple of a "nice" number of ticks.
	if (count <= numTicksMin) {
		max = count;
		numTicks = count + 1;
	} else {
		for (max = count; ; ++max) {
			for (numTicks = numTicksMin; numTicks <= numTicksMax; ++numTicks) {
				if (max % numTicks == 0)
					goto found_ticks;
			}
		}
	found_ticks:
		++numTicks; // There is one more tick than sections.
	}

	axis->setLabelFormat("%.0f");
	axis->setTitleText(StatsView::tr("No. dives"));
	axis->setRange(0, max);
	axis->setTickCount(numTicks);
	return max;
}

template<typename T>
QtCharts::QBarCategoryAxis *StatsView::createCategoryAxis(const StatsBinner &binner, const std::vector<T> &bins)
{
	using QtCharts::QBarCategoryAxis;

	QBarCategoryAxis *axis = makeAxis<QBarCategoryAxis>();
	for (const auto &[bin, dummy]: bins)
		axis->append(binner.format(*bin));
	return axis;
}

void StatsView::plotBarChart(const std::vector<dive *> &dives,
			     ChartSubType subType,
			     const StatsType *categoryType, const StatsBinner *categoryBinner,
			     const StatsType *valueType, const StatsBinner *valueBinner)
{
	using QtCharts::QBarSet;
	using QtCharts::QAbstractBarSeries;
	using QtCharts::QBarCategoryAxis;
	using QtCharts::QValueAxis;

	if (!categoryBinner || !valueBinner)
		return;

	setTitle(valueType->nameWithBinnerUnit(*valueBinner));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	// The problem here is that for different dive sets of the category
	// bins, we might get different value bins. So we have to keep track
	// of our counts and adjust accordingly. That's a bit annoying.
	// Perhaps we should determine the bins of all dives first and then
	// query the counts for precisely those bins?
	using BinCountsPair = std::pair<StatsBinPtr, std::vector<int>>;
	std::vector<BinCountsPair> vbin_counts;
	int catBinNr = 0;
	int maxCount = 0;
	int maxCategoryCount = 0;
	for (const auto &[bin, dives]: categoryBins) {
		int categoryCount = 0;
		for (auto &[vbin, count]: valueBinner->count_dives(dives, false)) {
			// Note: we assume that the bins are sorted!
			auto it = std::lower_bound(vbin_counts.begin(), vbin_counts.end(), vbin,
						   [] (const BinCountsPair &p, const StatsBinPtr &bin)
						   { return *p.first < *bin; });
			if (it == vbin_counts.end() || *it->first != *vbin) {
				// Add a new value bin.
				// Attn: this invalidate "vbin", which must not be used henceforth!
				it = vbin_counts.insert(it, { std::move(vbin), std::vector<int>(categoryBins.size()) });
			}
			it->second[catBinNr] = count;
			categoryCount += count;
			if (count > maxCount)
				maxCount = count;
		}
		if (categoryCount > maxCategoryCount)
			maxCategoryCount = categoryCount;
		++catBinNr;
	}

	bool isStacked = subType == ChartSubType::VerticalStacked || subType == ChartSubType::HorizontalStacked;

	QBarCategoryAxis *catAxis = createCategoryAxis(*categoryBinner, categoryBins);
	catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

	QValueAxis *valAxis = makeAxis<QValueAxis>();
	int maxVal = isStacked ? maxCategoryCount : maxCount;
	initCountAxis(valAxis, maxVal);

	bool isHorizontal = subType == ChartSubType::Horizontal || subType == ChartSubType::HorizontalStacked;
	if (isHorizontal)
		addAxes(valAxis, catAxis);
	else
		addAxes(catAxis, valAxis);
	QAbstractBarSeries *series;
	switch (subType) {
	default:
	case ChartSubType::Vertical:
		series = addSeries<QtCharts::QBarSeries>(valueType->name());
		break;
	case ChartSubType::VerticalStacked:
		series = addSeries<QtCharts::QStackedBarSeries>(valueType->name());
		break;
	case ChartSubType::Horizontal:
		series = addSeries<QtCharts::QHorizontalBarSeries>(valueType->name());
		break;
	case ChartSubType::HorizontalStacked:
		series = addSeries<QtCharts::QHorizontalStackedBarSeries>(valueType->name());
		break;
	}

	for (auto &[vbin, counts]: vbin_counts) {
		QBarSet *set = new QBarSet(valueBinner->format(*vbin));
		for (int count: counts)
			*set << count;
		series->append(set);
	}

	showLegend();
}

static QString makeFormatString(int decimals)
{
	return QStringLiteral("%.%1f").arg(decimals < 0 ? 0 : decimals);
}

QtCharts::QValueAxis *StatsView::createValueAxis(double min, double max, int decimals, bool isHorizontal)
{
	using QtCharts::QValueAxis;

	// Avoid degenerate cases
	if (max - min < 0.0001) {
		max = 0.5;
		min = -0.5;
	}
	// TODO: Let the acceptable number of ticks depend on the size of the graph and font.
	int numTicks = isHorizontal ? 8 : 10;

	QValueAxis *axis = makeAxis<QValueAxis>();
	axis->setRange(min, max);

	// Use full decimal increments:w
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

	axis->setLabelFormat(makeFormatString(decimals));
	axis->setTickInterval(inc);
	axis->setTickAnchor(0.0);
	axis->setTickType(QValueAxis::TicksDynamic);
	return axis;
}

void StatsView::plotValueChart(const std::vector<dive *> &dives,
			       ChartSubType subType,
			       const StatsType *categoryType, const StatsBinner *categoryBinner,
			       const StatsType *valueType, StatsOperation valueAxisOperation)
{
	using QtCharts::QBarSet;
	using QtCharts::QAbstractBarSeries;
	using QtCharts::QBarCategoryAxis;
	using QtCharts::QValueAxis;

	if (!categoryBinner)
		return;

	setTitle(QStringLiteral("%1 (%2)").arg(valueType->name(),
					       StatsType::operationName(valueAxisOperation)));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	std::vector<double> values;
	values.reserve(categoryBins.size());
	for (auto const &[bin, dives]: categoryBins)
		values.push_back(valueType->applyOperation(dives, valueAxisOperation));
	QBarCategoryAxis *catAxis = createCategoryAxis(*categoryBinner, categoryBins);
	catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

	bool isHorizontal = subType == ChartSubType::Horizontal;
	double maxValue = *std::max_element(values.begin(), values.end());
	double chartHeight = maxValue * (1.0 + barTopSpace);
	int decimals = valueType->decimals();
	QValueAxis *valAxis = createValueAxis(0.0, chartHeight, valueType->decimals(), isHorizontal);
	valAxis->setTitleText(valueType->nameWithUnit());

	QAbstractBarSeries *series;
	if (isHorizontal) {
		addAxes(valAxis, catAxis);
		series = addSeries<QtCharts::QHorizontalBarSeries>(valueType->name());
	} else {
		addAxes(catAxis, valAxis);
		series = addSeries<QtCharts::QBarSeries>(valueType->name());
	}

	QBarSet *set = new QBarSet(QString());
	double pos = 0.0;
	for (double value: values) {
		*set << value;
		std::vector<QString> label = { QString("%L1").arg(value, 0, 'f', decimals) };
		barLabels.emplace_back(label, pos, value, isHorizontal, series);
		pos += 1.0;
	}
	series->append(set);

	hideLegend();
}

static int getTotalCount(const std::vector<StatsBinCount> &bins)
{
	int total = 0;
	for (const auto &[bin, count]: bins)
		total += count;
	return total;
}

// Formats "x (y%)" as either a single or two strings for horizontal and non-horizontal cases, respectively.
static std::vector<QString> makePercentageLabels(int count, int total, bool isHorizontal)
{
	double percentage = count * 100.0 / total;
	QString countString = QString("%L1").arg(count);
	QString percentageString = QString("(%L1%)").arg(percentage, 0, 'f', 1);
	if (isHorizontal)
		return { QString("%1 %2").arg(countString, percentageString) };
	else
		return { countString, percentageString };
}

void StatsView::plotDiscreteCountChart(const std::vector<dive *> &dives,
				      ChartSubType subType,
				      const StatsType *categoryType, const StatsBinner *categoryBinner)
{
	using QtCharts::QAbstractBarSeries;
	using QtCharts::QBarCategoryAxis;
	using QtCharts::QBarSet;
	using QtCharts::QPieSeries;
	using QtCharts::QPieSlice;
	using QtCharts::QValueAxis;

	if (!categoryBinner)
		return;

	setTitle(categoryType->nameWithBinnerUnit(*categoryBinner));

	std::vector<StatsBinCount> categoryBins = categoryBinner->count_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	int total = getTotalCount(categoryBins);

	if (subType == ChartSubType::Pie) {
		QPieSeries *series = addSeries<QtCharts::QPieSeries>(categoryType->name());
		QLocale loc;
		for (auto const &[bin, count]: categoryBins) {
			double percentage = count * 100.0 / total;
			QString label = QString("%1 (%2: %3\%)").arg(
						categoryBinner->format(*bin),
						loc.toString(count),
						loc.toString(percentage, 'f', 1));
			QPieSlice *slice = new QPieSlice(label, count);
			slice->setLabelVisible(true);
			series->append(slice);
		}
		showLegend();
	} else {
		QBarCategoryAxis *catAxis = createCategoryAxis(*categoryBinner, categoryBins);
		catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

		QValueAxis *valAxis = makeAxis<QValueAxis>();

		QAbstractBarSeries *series;
		bool isHorizontal = subType != ChartSubType::Vertical;
		if (isHorizontal) {
			addAxes(valAxis, catAxis);
			series = addSeries<QtCharts::QHorizontalBarSeries>(categoryType->name());
		} else {
			addAxes(catAxis, valAxis);
			series = addSeries<QtCharts::QBarSeries>(categoryType->name());
		}
		if (!series)
			return;

		int maxCount = 0;
		QBarSet *set = new QBarSet(QString());
		double pos = 0.0;
		for (auto const &[bin, count]: categoryBins) {
			if (count > maxCount)
				maxCount = count;
			*set << count;
			std::vector<QString> label = makePercentageLabels(count, total, isHorizontal);
			barLabels.emplace_back(label, pos, count, isHorizontal, series);
			pos += 1.0;
		}

		initCountAxis(valAxis, maxCount);

		series->append(set);
		hideLegend();
	}
}

static std::pair<double, double> getMinMaxValue(const std::vector<std::vector<double>> &values)
{
	if (values.empty())
		return { 0.0, 0.0 };
	double min = 1e14, max = 0.0;
	for (const std::vector<double> &v: values) {
		if (v.empty())
			continue;
		if (v.front() < min)
			min = v.front();
		if (v.back() > max)
			max = v.back();
	}
	return { min, max };
}

static std::pair<double, double> getMinMaxValue(const std::vector<std::pair<double, double>> &values)
{
	if (values.empty())
		return { 0.0, 0.0 };
	double min = 1e14, max = 0.0;
	for (auto [dummy, v]: values) {
		if (v < min)
			min = v;
		if (v > max)
			max = v;
	}
	return { min, max };
}

static std::pair<double, double> getMinMaxValue(const std::vector<StatsQuartiles> &values)
{
	if (values.empty())
		return { 0.0, 0.0 };
	double min = 1e14, max = 0.0;
	for (const StatsQuartiles &v: values) {
		if (v.min < min)
			min = v.min;
		if (v.max > max)
			max = v.max;
	}
	return { min, max };
}

void StatsView::plotDiscreteBoxChart(const std::vector<dive *> &dives,
				     const StatsType *categoryType, const StatsBinner *categoryBinner,
				     const StatsType *valueType)
{
	using QtCharts::QBoxPlotSeries;
	using QtCharts::QBoxSet;
	using QtCharts::QBarCategoryAxis;
	using QtCharts::QValueAxis;

	if (!categoryBinner)
		return;

	setTitle(valueType->name());

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	QBarCategoryAxis *catAxis = createCategoryAxis(*categoryBinner, categoryBins);
	catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

	std::vector<StatsQuartiles> quartiles;
	quartiles.reserve(categoryBins.size());
	for (const auto &[bin, dives]: categoryBins)
		quartiles.push_back(valueType->quartiles(dives));

	auto [minY, maxY] = getMinMaxValue(quartiles);
	double chartTop = maxY * (1.0 + barTopSpace);
	double chartBottom = minY * (1.0 - barTopSpace);
	QValueAxis *valueAxis = createValueAxis(chartBottom, chartTop, valueType->decimals(), false);
	valueAxis->setTitleText(valueType->nameWithUnit());

	addAxes(catAxis, valueAxis);

	QBoxPlotSeries *series = addSeries<QBoxPlotSeries>(valueType->name());

	for (const StatsQuartiles &q: quartiles) {
		QBoxSet *set = new QBoxSet(q.min, q.q1, q.q2, q.q3, q.max);
		series->append(set);
	}

	hideLegend();
}

void StatsView::plotDiscreteScatter(const std::vector<dive *> &dives,
				    const StatsType *categoryType, const StatsBinner *categoryBinner,
				    const StatsType *valueType)
{
	using QtCharts::QBarCategoryAxis;
	using QtCharts::QScatterSeries;
	using QtCharts::QValueAxis;

	if (!categoryBinner)
		return;

	setTitle(valueType->name());

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	QBarCategoryAxis *catAxis = createCategoryAxis(*categoryBinner, categoryBins);
	catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

	std::vector<std::vector<double>> values;
	values.reserve(categoryBins.size());
	for (const auto &[dummy, dives]: categoryBins)
		values.push_back(valueType->values(dives));

	auto [minValue, maxValue] = getMinMaxValue(values);

	double chartTop = maxValue * (1.0 + barTopSpace);
	double chartBottom = minValue * (1.0 - barTopSpace);
	QValueAxis *valAxis = createValueAxis(chartBottom, chartTop, valueType->decimals(), false);
	valAxis->setTitleText(valueType->nameWithUnit());

	addAxes(catAxis, valAxis);
	QScatterSeries *series = addSeries<QScatterSeries>(valueType->name());
	QScatterSeries *quartileSeries = addSeries<QScatterSeries>(valueType->name());
	quartileSeries->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
	quartileSeries->setColor(Qt::red);

	double x = 0.0;
	for (const std::vector<double> &array: values) {
		for (double v: array)
			series->append(x, v);
		StatsQuartiles quartiles = StatsType::quartiles(array);
		quartileSeries->append(x, quartiles.q1);
		quartileSeries->append(x, quartiles.q2);
		quartileSeries->append(x, quartiles.q3);
		x += 1.0;
	}

	hideLegend();
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

void StatsView::addLineMarker(double pos, double low, double high, const QPen &pen, bool isHorizontal)
{
	using QtCharts::QLineSeries;

	QLineSeries *series = addSeries<QLineSeries>(QString());
	if(!series)
		return;
	if (isHorizontal) {
		series->append(low, pos);
		series->append(high, pos);
	} else {
		series->append(pos, low);
		series->append(pos, high);
	}
	series->setPen(pen);
}


void StatsView::addBar(double lowerBound, double upperBound, double height, const QBrush &brush, const QPen &pen, bool isHorizontal,
		       const std::vector<QString> &label)
{
	using QtCharts::QAreaSeries;
	using QtCharts::QLineSeries;

	QAreaSeries *series = addSeries<QAreaSeries>(QString());
	QLineSeries *lower = new QLineSeries;
	QLineSeries *upper = new QLineSeries;
	double delta = (upperBound - lowerBound) * histogramBarWidth;
	double from = (lowerBound + upperBound - delta) / 2.0;
	double to = (lowerBound + upperBound + delta) / 2.0;
	if (isHorizontal) {
		lower->append(0.0, from);
		lower->append(0.0, to);
		upper->append(height, from);
		upper->append(height, to);
	} else {
		lower->append(from, 0.0);
		lower->append(to, 0.0);
		upper->append(from, height);
		upper->append(to, height);
	}
	series->setBrush(brush);
	series->setPen(pen);
	series->setLowerSeries(lower);
	series->setUpperSeries(upper);

	// Add label if provided
	if (!label.empty()) {
		double mid = (from + to) / 2.0;
		barLabels.emplace_back(label, mid, height, isHorizontal, series);
	}
}

StatsView::BarLabel::BarLabel(const std::vector<QString> &labels,
			      double value, double height,
			      bool isHorizontal, QtCharts::QAbstractSeries *series) :
	value(value), height(height),
	totalWidth(0.0), totalHeight(0.0),
	isHorizontal(isHorizontal), series(series)
{
	items.reserve(labels.size());
	for (const QString &label: labels) {
		items.emplace_back(new QGraphicsSimpleTextItem(series->chart()));
		items.back()->setText(label);
		items.back()->setZValue(10.0); // ? What is a sensible value here ?
		QRectF rect = items.back()->boundingRect();
		if (rect.width() > totalWidth)
			totalWidth = rect.width();
		totalHeight += rect.height();
	}
	updatePosition();
}

void StatsView::BarLabel::updatePosition()
{
	QtCharts::QChart *chart = series->chart();
	if (!isHorizontal) {
		QPointF pos = chart->mapToPosition(QPointF(value, height), series);
		QPointF lowPos = chart->mapToPosition(QPointF(value, 0.0), series);
		// Attention: the lower position is at the bottom and therefore has the higher y-coordinate on the screen. Ugh.
		double barHeight = lowPos.y() - pos.y();
		// Heuristics: if the label fits nicely into the bar (bar height is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it on top of the bar.
		if (barHeight >= 2.0 * totalHeight)
			pos.ry() = lowPos.y() - (barHeight + totalHeight) / 2.0;
		else
			pos.ry() -= totalHeight + 2.0; // Leave two pixels(?) space
		for (auto &it: items) {
			QPointF itemPos = pos;
			QRectF rect = it->boundingRect();
			itemPos.rx() -= rect.width() / 2.0;
			it->setPos(itemPos);
			pos.ry() += rect.height();
		}
	} else {
		QPointF pos = chart->mapToPosition(QPointF(height, value), series);
		QPointF lowPos = chart->mapToPosition(QPointF(0.0, value), series);
		double barWidth = pos.x() - lowPos.x();
		// Heuristics: if the label fits nicely into the bar (bar width is at least twice the label height),
		// then put the label in the middle of the bar. Otherwise, put it to the right of the bar.
		if (barWidth >= 2.0 * totalWidth)
			pos.rx() = lowPos.x() + (barWidth - totalWidth) / 2.0;
		else
			pos.rx() += totalWidth / 2.0 + 2.0; // Leave two pixels(?) space
		pos.ry() -= totalHeight / 2.0;
		for (auto &it: items) {
			QPointF itemPos = pos;
			QRectF rect = it->boundingRect();
			itemPos.rx() -= rect.width() / 2.0;
			it->setPos(itemPos);
			pos.ry() += rect.height();
		}
	}
}

static void initHistogramAxis(QtCharts::QCategoryAxis *axis, const std::vector<std::pair<QString, double>> &labels, int maxLabels)
{
	using QtCharts::QCategoryAxis;

	if (labels.size() < 2) // Less than two makes no sense -> there must be at least one category
		return;
	if (maxLabels <= 1)
		maxLabels = 2;

	axis->setMin(labels.front().second);
	axis->setMax(labels.back().second);
	axis->setStartValue(labels.front().second);
	int step = ((int)labels.size() - 1) / maxLabels + 1;
	for (int i = 0; i < (int)labels.size(); i += step)
		axis->append(labels[i].first, labels[i].second);
	axis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
}

// Yikes, we get our data in different kinds of (bin, value) pairs.
// To create a category axis from this, we have to templatify the function.
template<typename T>
QtCharts::QCategoryAxis *StatsView::createHistogramAxis(const StatsBinner &binner, const std::vector<T> &bins, bool isHorizontal)
{
	using QtCharts::QCategoryAxis;

	LabelDisambiguator labeler;
	std::vector<std::pair<QString, double>> labelValues;
	for (auto const &[bin, dummy]: bins) {
		QString label = binner.formatLowerBound(*bin);
		double lowerBound = binner.lowerBoundToFloat(*bin);
		labelValues.emplace_back(labeler.transmogrify(label), lowerBound);
	}

	const StatsBin &lastBin = *bins.back().bin;
	QString lastLabel = binner.formatUpperBound(lastBin);
	double upperBound = binner.upperBoundToFloat(lastBin);
	labelValues.emplace_back(labeler.transmogrify(lastLabel), upperBound);

	QCategoryAxis *catAxis = makeAxis<QCategoryAxis>();
	int maxLabels = isHorizontal ? 10 : 15;
	initHistogramAxis(catAxis, labelValues, maxLabels);

	return catAxis;
}

template<typename T>
double getMaxVal(const std::vector<T> &bins)
{
	double res = 0;
	for (auto const &[dummy, val]: bins) {
		if (val > res)
			res = val;
	}
	return res;
}

void StatsView::plotHistogramCountChart(const std::vector<dive *> &dives,
					ChartSubType subType,
					const StatsType *categoryType, const StatsBinner *categoryBinner)
{
	using QtCharts::QAbstractAxis;
	using QtCharts::QCategoryAxis;
	using QtCharts::QValueAxis;

	if (!categoryBinner)
		return;

	setTitle(categoryType->name());

	std::vector<StatsBinCount> categoryBins = categoryBinner->count_dives(dives, true);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	bool isHorizontal = subType == ChartSubType::Horizontal;
	QCategoryAxis *catAxis = createHistogramAxis(*categoryBinner, categoryBins, isHorizontal);
	catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

	double maxCategoryCount = getMaxVal(categoryBins);
	int total = getTotalCount(categoryBins);

	QValueAxis *valAxis = makeAxis<QValueAxis>();
	double chartHeight = initCountAxis(valAxis, lrint(maxCategoryCount));

	QAbstractAxis *xAxis = catAxis;
	QAbstractAxis *yAxis = valAxis;
	if (isHorizontal)
		std::swap(xAxis, yAxis);
	addAxes(xAxis, yAxis);

	for (auto const &[bin, count]: categoryBins) {
		double height = count;
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		std::vector<QString> label = makePercentageLabels(count, total, isHorizontal);

		addBar(lowerBound, upperBound, height, QBrush(Qt::blue), QPen(Qt::white), isHorizontal, label);
	}

	if (categoryType->type() == StatsType::Type::Numeric) {
		double mean = categoryType->mean(dives);
		double median = categoryType->quartiles(dives).q2;
		QPen meanPen(Qt::green);
		meanPen.setWidth(2);
		QPen medianPen(Qt::red);
		medianPen.setWidth(2);
		addLineMarker(mean, 0.0, chartHeight, meanPen, isHorizontal);
		addLineMarker(median, 0.0, chartHeight, medianPen, isHorizontal);
	}

	hideLegend();
}

void StatsView::plotHistogramBarChart(const std::vector<dive *> &dives,
				      ChartSubType subType,
				      const StatsType *categoryType, const StatsBinner *categoryBinner,
				      const StatsType *valueType, StatsOperation valueAxisOperation)
{
	using QtCharts::QAbstractAxis;
	using QtCharts::QCategoryAxis;
	using QtCharts::QValueAxis;

	if (!categoryBinner)
		return;

	setTitle(QStringLiteral("%1 (%2)").arg(valueType->name(),
					       StatsType::operationName(valueAxisOperation)));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, true);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	bool isHorizontal = subType == ChartSubType::Horizontal;
	QCategoryAxis *catAxis = createHistogramAxis(*categoryBinner, categoryBins, isHorizontal);
	catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

	std::vector<double> values;
	values.reserve(categoryBins.size());
	for (const auto &[dummy, dives]: categoryBins)
		values.push_back(valueType->applyOperation(dives, valueAxisOperation));
	double maxValue = *std::max_element(values.begin(), values.end());

	double chartHeight = maxValue * (1.0 + barTopSpace);
	int decimals = valueType->decimals();
	QValueAxis *valAxis = createValueAxis(0.0, chartHeight, decimals, isHorizontal);
	valAxis->setTitleText(valueType->nameWithUnit());

	QAbstractAxis *xAxis = catAxis;
	QAbstractAxis *yAxis = valAxis;
	if (isHorizontal)
		std::swap(xAxis, yAxis);
	addAxes(xAxis, yAxis);

	int i = 0;
	for (auto const &[bin, count]: categoryBins) {
		double height = values[i++];
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		QString label = QString("%L1").arg(height, 0, 'f', decimals);
		addBar(lowerBound, upperBound, height, QBrush(Qt::blue), QPen(Qt::white), isHorizontal, {label});
	}

	hideLegend();
}

// Returns the coefficients [a,b] of the line y = ax + b
// If case of an undetermined regression or one with infinite slope, returns [nan, nan]
static std::pair<double, double> linear_regression(const std::vector<std::pair<double, double>> &v)
{
	const double NaN = std::numeric_limits<double>::quiet_NaN();
	if (v.empty())
		return { NaN, NaN };

	// First, calculate the x and y average
	double avg_x = 0.0, avg_y = 0.0;
	for (auto [x, y]: v) {
		avg_x += x;
		avg_y += y;
	}
	avg_x /= (double)v.size();
	avg_y /= (double)v.size();

	double num = 0.0, denom = 0.0;
	for (auto [x, y]: v) {
		num += (x - avg_x) * (y - avg_y);
		denom += (x - avg_x) * (x - avg_x);
	}
	if (fabs(denom) < 1e-10)
		return { NaN, NaN };
	double a = num / denom;
	double b = avg_y - a * avg_x;
	return { a, b };
}

void StatsView::plotScatter(const std::vector<dive *> &dives, const StatsType *categoryType, const StatsType *valueType)
{
	using QtCharts::QLineSeries;
	using QtCharts::QScatterSeries;
	using QtCharts::QValueAxis;

	setTitle(StatsTranslations::tr("%1 vs. %2").arg(valueType->name(), categoryType->name()));

	std::vector<std::pair<double, double>> points = categoryType->scatter(*valueType, dives);
	if (points.empty())
		return;

	double minX = points.front().first;
	double maxX = points.back().first;
	auto [minY, maxY] = getMinMaxValue(points);

	QValueAxis *axisX = createValueAxis(minX, maxX, categoryType->decimals(), true);
	axisX->setTitleText(categoryType->nameWithUnit());

	double chartTop = maxY * (1.0 + barTopSpace);
	double chartBottom = minY * (1.0 - barTopSpace);
	QValueAxis *axisY = createValueAxis(chartBottom, chartTop, valueType->decimals(), false);
	axisY->setTitleText(valueType->nameWithUnit());

	addAxes(axisX, axisY);
	QScatterSeries *series = addSeries<QScatterSeries>(valueType->name());

	for (auto [x, y]: points)
		series->append(x, y);

	// y = ax + b
	auto [a, b] = linear_regression(points);
	if (!std::isnan(a)) {
		QLineSeries *series = addSeries<QLineSeries>(QString());
		series->setPen(QPen(Qt::red));
		series->append(axisX->min(), a * axisX->min() + b);
		series->append(axisX->max(), a * axisX->max() + b);
	}

	hideLegend();
}
