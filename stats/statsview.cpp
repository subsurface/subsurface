// SPDX-License-Identifier: GPL-2.0
#include "statsview.h"
#include "statstypes.h"
#include "core/divefilter.h"
#include <QQuickItem>
#include <QAreaSeries>
#include <QBarCategoryAxis>
#include <QBarSet>
#include <QBarSeries>
#include <QCategoryAxis>
#include <QChart>
#include <QHorizontalBarSeries>
#include <QHorizontalStackedBarSeries>
#include <QLineSeries>
#include <QPieSeries>
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
	HistogramCount,
	Histogram
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
		stats_numeric_types,	// supports numeric types as second axis, since we want to average, etc
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
		ChartTypeId::Histogram,
		QT_TRANSLATE_NOOP("StatsTranslations", "Histogram value"),
		stats_continuous_types,	// supports continuous types as first axis
		stats_numeric_types,	// supports numeric types as second axis, since we want to average, etc
		true,
		false,
		false,
		true,
		{ ChartSubType::Vertical, ChartSubType::Horizontal }
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

StatsView::StatsView(QWidget *parent) : QQuickWidget(parent)
{
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(urlStatsView);
}

StatsView::~StatsView()
{
}

// Convenience templates that turn a series-type into a series-id.
// TODO: Qt should provide that, no? Let's inspect the docs more closely...
template<typename Series> constexpr int series_type_id();
template<> constexpr int series_type_id<QtCharts::QAreaSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeArea; }
template<> constexpr int series_type_id<QtCharts::QBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeBar; }
template<> constexpr int series_type_id<QtCharts::QStackedBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeStackedBar; }
template<> constexpr int series_type_id<QtCharts::QHorizontalBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeHorizontalBar; }
template<> constexpr int series_type_id<QtCharts::QHorizontalStackedBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeHorizontalStackedBar; }
template<> constexpr int series_type_id<QtCharts::QLineSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeLine; }
template<> constexpr int series_type_id<QtCharts::QPieSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypePie; }

// Helper function to add a series to a QML ChartView.
// Sadly, accessing QML from C++ is maliciously cumbersome.
template<typename Type>
Type *StatsView::addSeries(const QString &name, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis)
{
	QtCharts::QAbstractSeries *abstract_series = addSeriesHelper(name, series_type_id<Type>(), xAxis, yAxis);
	Type *res = qobject_cast<Type *>(abstract_series);
	if (!res)
		qWarning("Couldn't cast abstract series to %s", typeid(Type).name());

	return res;
}

QtCharts::QAbstractSeries *StatsView::addSeriesHelper(const QString &name, int type, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis)
{
	using QtCharts::QAbstractSeries;
	using QtCharts::QAbstractAxis;

	QQuickItem *chart = rootObject();
	QAbstractSeries *abstract_series;
	if (!QMetaObject::invokeMethod(chart, "createSeries", Qt::AutoConnection,
				       Q_RETURN_ARG(QAbstractSeries *, abstract_series),
				       Q_ARG(int, type),
				       Q_ARG(QString, name),
				       Q_ARG(QAbstractAxis *, xAxis),
				       Q_ARG(QAbstractAxis *, yAxis))) {
		qWarning("Couldn't call createSeries()");
		return nullptr;
	}

	return abstract_series;
}

QtCharts::QLegend *StatsView::getLegend()
{
	using QtCharts::QLegend;
	QQuickItem *chart = rootObject();
	QVariant v = chart->property("legend");
	QLegend *legend = v.value<QLegend *>();
	if (!legend)
		qWarning("Couldn't get legend");
	return legend;
}

void StatsView::showLegend()
{
	QtCharts::QLegend *legend = getLegend();
	if (!legend)
		return;
	legend->setVisible(true);
	legend->setAlignment(Qt::AlignBottom);
}

void StatsView::hideLegend()
{
	QtCharts::QLegend *legend = getLegend();
	if (!legend)
		return;
	legend->setVisible(false);
}

void StatsView::setTitle(const QString &s)
{
	QQuickItem *chart = rootObject();
	chart->setProperty("title", s);
}

template <typename T>
T *StatsView::makeAxis()
{
	T *res = new T;
	axes.emplace_back(res);
	return res;
}

void StatsView::reset()
{
	QQuickItem *chart = rootObject();
	QMetaObject::invokeMethod(chart, "removeAllSeries", Qt::AutoConnection);
	axes.clear();
}

void StatsView::plot(int type, int subTypeIdx,
		     int firstAxis, int firstAxisBin, int firstAxisOperation,
		     int secondAxis, int secondAxisBin, int secondAxisOperation)
{
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
	case ChartTypeId::HistogramCount:
		return plotHistogramCountChart(dives, subType, firstAxisType, firstAxisBinner);
	case ChartTypeId::Histogram:
		return plotHistogramChart(dives, subType, firstAxisType, firstAxisBinner, secondAxisType,
					  secondAxisType->idxToOperation(secondAxisOperation));
	default:
		qWarning("Unknown chart type: %d", (int)t->id);
		return;
	}
}

int initValueAxis(QtCharts::QValueAxis *axis, int count)
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

	QStringList categoryNames;
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
		categoryNames.push_back(categoryBinner->format(*bin));

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

	QBarCategoryAxis *catAxis = makeAxis<QBarCategoryAxis>();
	catAxis->append(categoryNames);
	catAxis->setTitleText(categoryType->nameWithUnit());

	QValueAxis *valAxis = makeAxis<QValueAxis>();
	int maxVal = isStacked ? maxCategoryCount : maxCount;
	initValueAxis(valAxis, maxVal);

	QAbstractBarSeries *series;
	switch (subType) {
	default:
	case ChartSubType::Vertical:
		series = addSeries<QtCharts::QBarSeries>(valueType->name(), catAxis, valAxis);
		break;
	case ChartSubType::VerticalStacked:
		series = addSeries<QtCharts::QStackedBarSeries>(valueType->name(), catAxis, valAxis);
		break;
	case ChartSubType::Horizontal:
		series = addSeries<QtCharts::QHorizontalBarSeries>(valueType->name(), valAxis, catAxis);
		break;
	case ChartSubType::HorizontalStacked:
		series = addSeries<QtCharts::QHorizontalStackedBarSeries>(valueType->name(), valAxis, catAxis);
		break;
	}
	if (!series)
		return;

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
	QStringList categoryNames;
	values.reserve(categoryBins.size());
	for (auto const &[bin, dives]: categoryBins) {
		categoryNames.push_back(categoryBinner->format(*bin));
		values.push_back(valueType->applyOperation(dives, valueAxisOperation));
	}
	QBarCategoryAxis *catAxis = makeAxis<QBarCategoryAxis>();
	catAxis->append(categoryNames);
	catAxis->setTitleText(categoryType->nameWithUnit());

	double maxValue = *std::max_element(values.begin(), values.end());
	double chartHeight = maxValue * (1.0 + barTopSpace);
	QValueAxis *valAxis = makeAxis<QValueAxis>();
	// TODO: round axis labels to "nice" numbers.
	valAxis->setRange(0, chartHeight);
	valAxis->setLabelFormat(makeFormatString(valueType->decimals()));
	valAxis->setTitleText(valueType->nameWithUnit());

	QAbstractBarSeries *series;
	if (subType == ChartSubType::Vertical)
		series = addSeries<QtCharts::QBarSeries>(valueType->name(), catAxis, valAxis);
	else
		series = addSeries<QtCharts::QHorizontalBarSeries>(valueType->name(), valAxis, catAxis);
	if (!series)
		return;

	QBarSet *set = new QBarSet(QString());
	for (double value: values) {
		*set << value;
	}
	series->append(set);

	hideLegend();
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

	if (subType == ChartSubType::Pie) {
		QPieSeries *series = addSeries<QtCharts::QPieSeries>(categoryType->name(), nullptr, nullptr);
		for (auto const &[bin, count]: categoryBins) {
			QPieSlice *slice = new QPieSlice(categoryBinner->format(*bin), count);
			slice->setLabelVisible(true);
			series->append(slice);
		}
		showLegend();
	} else {
		QBarCategoryAxis *catAxis = makeAxis<QBarCategoryAxis>();
		catAxis->setTitleText(categoryType->nameWithBinnerUnit(*categoryBinner));

		QValueAxis *valAxis = makeAxis<QValueAxis>();

		QAbstractBarSeries *series;
		if (subType == ChartSubType::Vertical)
			series = addSeries<QtCharts::QBarSeries>(categoryType->name(), catAxis, valAxis);
		else
			series = addSeries<QtCharts::QHorizontalBarSeries>(categoryType->name(), valAxis, catAxis);
		if (!series)
			return;

		int maxCount = 0;
		QBarSet *set = new QBarSet(QString());
		for (auto const &[bin, count]: categoryBins) {
			catAxis->append(categoryBinner->format(*bin));
			if (count > maxCount)
				maxCount = count;
			*set << count;
		}

		initValueAxis(valAxis, maxCount);

		series->append(set);
		hideLegend();
	}
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

void StatsView::addLineMarker(double pos, double low, double high, const QPen &pen,
			      QtCharts::QAbstractAxis *axisX, QtCharts::QAbstractAxis *axisY,
			      bool isHorizontal)
{
	using QtCharts::QLineSeries;

	QLineSeries *series = addSeries<QLineSeries>(QString(), axisX, axisY);
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


void StatsView::addBar(double lowerBound, double upperBound, double height, const QBrush &brush, const QPen &pen,
		       QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis, bool isHorizontal)
{
	using QtCharts::QAreaSeries;
	using QtCharts::QLineSeries;

	QAreaSeries *series = addSeries<QAreaSeries>(QString(), xAxis, yAxis);
	if (!series)
		return;
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

	int maxCategoryCount = 0;
	QCategoryAxis *catAxis = makeAxis<QCategoryAxis>();
	LabelDisambiguator labeler;
	double lowerBound = categoryBinner->lowerBoundToFloat(*categoryBins.front().bin);
	catAxis->setMin(lowerBound);
	catAxis->setStartValue(lowerBound);
	for (auto const &[bin, count]: categoryBins) {
		if (count > maxCategoryCount)
			maxCategoryCount = count;
		QString label = categoryBinner->formatLowerBound(*bin);
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		catAxis->append(labeler.transmogrify(label), lowerBound);
	}

	const StatsBin &lastBin = *categoryBins.back().bin;
	QString lastLabel = categoryBinner->formatUpperBound(lastBin);
	double upperBound = categoryBinner->upperBoundToFloat(lastBin);
	catAxis->append(labeler.transmogrify(lastLabel), upperBound);
	catAxis->setMax(upperBound);
	catAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	catAxis->setTitleText(categoryType->nameWithUnit());

	QValueAxis *valAxis = makeAxis<QValueAxis>();
	double chartHeight = initValueAxis(valAxis, maxCategoryCount);

	bool isHorizontal = subType == ChartSubType::Horizontal;
	QAbstractAxis *xAxis = catAxis;
	QAbstractAxis *yAxis = valAxis;
	if (isHorizontal)
		std::swap(xAxis, yAxis);
	for (auto const &[bin, count]: categoryBins) {
		double height = count;
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		addBar(lowerBound, upperBound, height, QBrush(Qt::blue), QPen(Qt::red), xAxis, yAxis, isHorizontal);
	}

	if (categoryType->type() == StatsType::Type::Numeric) {
		double average = categoryType->average(dives);
		double median = categoryType->quartiles(dives).q2;
		QPen averagePen(Qt::green);
		averagePen.setWidth(2);
		QPen medianPen(Qt::black);
		medianPen.setWidth(2);
		addLineMarker(average, 0.0, chartHeight, averagePen, xAxis, yAxis, isHorizontal);
		addLineMarker(median, 0.0, chartHeight, medianPen, xAxis, yAxis, isHorizontal);
	}

	hideLegend();
}

void StatsView::plotHistogramChart(const std::vector<dive *> &dives,
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

	QCategoryAxis *catAxis = makeAxis<QCategoryAxis>();
	LabelDisambiguator labeler;
	double lowerBound = categoryBinner->lowerBoundToFloat(*categoryBins.front().bin);
	catAxis->setMin(lowerBound);
	catAxis->setStartValue(lowerBound);
	std::vector<double> values;
	values.reserve(categoryBins.size());
	for (auto const &[bin, dives]: categoryBins) {
		QString label = categoryBinner->formatLowerBound(*bin);
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		catAxis->append(labeler.transmogrify(label), lowerBound);
		values.push_back(valueType->applyOperation(dives, valueAxisOperation));
	}

	const StatsBin &lastBin = *categoryBins.back().bin;
	QString lastLabel = categoryBinner->formatUpperBound(lastBin);
	double upperBound = categoryBinner->upperBoundToFloat(lastBin);
	catAxis->append(labeler.transmogrify(lastLabel), upperBound);
	catAxis->setMax(upperBound);
	catAxis->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
	catAxis->setTitleText(categoryType->nameWithUnit());

	double maxValue = *std::max_element(values.begin(), values.end());
	double chartHeight = maxValue * (1.0 + barTopSpace);
	QValueAxis *valAxis = makeAxis<QValueAxis>();
	// TODO: round axis labels to "nice" numbers.
	valAxis->setRange(0, chartHeight);
	valAxis->setLabelFormat(makeFormatString(valueType->decimals()));
	valAxis->setTitleText(valueType->nameWithUnit());

	bool isHorizontal = subType == ChartSubType::Horizontal;
	QAbstractAxis *xAxis = catAxis;
	QAbstractAxis *yAxis = valAxis;
	if (isHorizontal)
		std::swap(xAxis, yAxis);
	int i = 0;
	for (auto const &[bin, count]: categoryBins) {
		double height = values[i++];
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		addBar(lowerBound, upperBound, height, QBrush(Qt::blue), QPen(Qt::red), xAxis, yAxis, isHorizontal);
	}

	hideLegend();
}
