// SPDX-License-Identifier: GPL-2.0
#include "statsview.h"
#include "statstypes.h"
#include "core/divefilter.h"
#include <QQuickItem>
#include <QChart>
#include <QBarSet>
#include <QBarSeries>
#include <QStackedBarSeries>
#include <QHorizontalBarSeries>
#include <QHorizontalStackedBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>

// Some of our compilers do not support std::size(). Roll our own for now.
template <typename T, size_t N>
static constexpr size_t array_size(const T (&)[N])
{
	return N;
}

enum class ChartSubType {
	Vertical = 0,
	VerticalStacked,
	Horizontal,
	HorizontalStacked
};

// Attn: The order must correspond to the enum above
static const char *chart_subtype_names[] = {
	QT_TRANSLATE_NOOP("StatsTranslations", "Vertical"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Vertical stacked"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Horizontal"),
	QT_TRANSLATE_NOOP("StatsTranslations", "Horizontal stacked")
};

enum class ChartTypeId {
	DiscreteBar
};

static const struct ChartType {
	ChartTypeId id;
	const char *name;
	const std::vector<const StatsType *> &firstAxisTypes;
	const std::vector<const StatsType *> &secondAxisTypes;
	bool firstAxisBinned;
	bool secondAxisBinned;
	const std::vector<ChartSubType> subtypes;
} chart_types[] = {
	{
		ChartTypeId::DiscreteBar,
		QT_TRANSLATE_NOOP("StatsTranslations", "Discrete Bar"),
		stats_types,	// supports all types as first axis
		stats_types,	// supports all types as second axis
		true,
		true,
		{ ChartSubType::Vertical, ChartSubType::VerticalStacked, ChartSubType::Horizontal, ChartSubType::HorizontalStacked }
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
template<> constexpr int series_type_id<QtCharts::QBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeBar; }
template<> constexpr int series_type_id<QtCharts::QStackedBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeStackedBar; }
template<> constexpr int series_type_id<QtCharts::QHorizontalBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeHorizontalBar; }
template<> constexpr int series_type_id<QtCharts::QHorizontalStackedBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeHorizontalStackedBar; }

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

void StatsView::showLegend()
{
	using QtCharts::QLegend;
	QQuickItem *chart = rootObject();
	QVariant v = chart->property("legend");
	QLegend *legend = v.value<QLegend *>();
	if (!legend) {
		qWarning("Couldn't get legend");
		return;
	}
	legend->setVisible(true);
	legend->setAlignment(Qt::AlignBottom);
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

void StatsView::plot(int type, int subTypeIdx, int firstAxis, int firstAxisBin, int secondAxis, int secondAxisBin)
{
	reset();

	const ChartType *t = idxToChartType(type);
	const StatsType *firstAxisType = idxToFirstAxisType(type, firstAxis);
	const StatsType *secondAxisType = idxToSecondAxisType(type, firstAxis, secondAxis);
	if (!t || !firstAxisType || !secondAxisType)
		return;

	ChartSubType subType = subTypeIdx >= 0 && subTypeIdx < (int)t->subtypes.size() ?
		t->subtypes[subTypeIdx] : ChartSubType::Vertical;
	const StatsBinner *firstAxisBinner = t->firstAxisBinned ? firstAxisType->getBinner(firstAxisBin) : nullptr;
	const StatsBinner *secondAxisBinner = t->secondAxisBinned ? secondAxisType->getBinner(secondAxisBin) : nullptr;

	const std::vector<dive *> dives = DiveFilter::instance()->visibleDives();
	switch (t->id) {
	case ChartTypeId::DiscreteBar:
		return plotBarChart(dives, subType, firstAxisType, firstAxisBinner, secondAxisType, secondAxisBinner);
	default:
		qWarning("Unknown chart type");
		return;
	}
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

	setTitle(valueType->name());

	QStringList categoryNames;
	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives);

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
		categoryNames.push_back(bin->format());

		int categoryCount = 0;
		for (auto &[vbin, count]: valueBinner->count_dives(dives)) {
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
	catAxis->setTitleText(categoryType->name());

	QValueAxis *valAxis = makeAxis<QValueAxis>();
	valAxis->setRange(0, isStacked ? maxCategoryCount : maxCount);
	valAxis->setLabelFormat(QStringLiteral("%d"));
	valAxis->setTitleText(tr("No. dives"));

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
		QBarSet *set = new QBarSet(vbin->format());
		for (int count: counts)
			*set << count;
		series->append(set);
	}

	showLegend();
}
