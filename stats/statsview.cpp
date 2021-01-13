// SPDX-License-Identifier: GPL-2.0
#include "statsview.h"
#include "barseries.h"
#include "boxseries.h"
#include "legend.h"
#include "pieseries.h"
#include "scatterseries.h"
#include "statsaxis.h"
#include "statscolors.h"
#include "statsgrid.h"
#include "statshelper.h"
#include "statsstate.h"
#include "statstranslations.h"
#include "statsvariables.h"
#include "zvalues.h"
#include "core/divefilter.h"
#include "core/subsurface-qt/divelistnotifier.h"

#include <cmath>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>

// Constants that control the graph layouts
static const QColor quartileMarkerColor(Qt::red);
static const double quartileMarkerSize = 15.0;
static const double sceneBorder = 5.0;			// Border between scene edges and statitistics view
static const double titleBorder = 2.0;			// Border between title and chart

StatsView::StatsView(QQuickItem *parent) : QQuickItem(parent),
	highlightedSeries(nullptr),
	xAxis(nullptr),
	yAxis(nullptr),
	draggedItem(nullptr),
	rootNode(nullptr)
{
	setFlag(ItemHasContents, true);

	connect(&diveListNotifier, &DiveListNotifier::numShownChanged, this, &StatsView::replotIfVisible);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);

	QFont font;
	titleFont = QFont(font.family(), font.pointSize(), QFont::Light);	// Make configurable
}

StatsView::StatsView() : StatsView(nullptr)
{
}

StatsView::~StatsView()
{
}

void StatsView::mousePressEvent(QMouseEvent *event)
{
	// Currently, we only support dragging of the legend. If other objects
	// should be made draggable, this needs to be generalized.
	if (legend) {
		QPointF pos = event->localPos();
		QRectF rect = legend->getRect();
		if (legend->getRect().contains(pos)) {
			dragStartMouse = pos;
			dragStartItem = rect.topLeft();
			draggedItem = legend.get();
			grabMouse();
		}
	}
}

void StatsView::mouseReleaseEvent(QMouseEvent *)
{
	if (draggedItem) {
		draggedItem = nullptr;
		ungrabMouse();
	}
}

QSGNode *StatsView::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
	// The QtQuick drawing interface is utterly bizzare with a distinct 1980ies-style memory management.
	// This is just a copy of what is found in Qt's documentation.
	QSGImageNode *n = static_cast<QSGImageNode *>(oldNode);
	if (!n)
		n = rootNode = window()->createImageNode();

	for (ChartItem *item: items) {
		if (item->dirty)
			item->render();
	}

	QRectF rect = boundingRect();
	if (plotRect != rect) {
		plotRect = rect;
		plotAreaChanged(plotRect.size());
	}

	img->fill(backgroundColor);
	scene.render(painter.get());
	texture.reset(window()->createTextureFromImage(*img, QQuickWindow::TextureIsOpaque));
	n->setTexture(texture.get());
	n->setRect(rect);
	return n;
}

void StatsView::addQSGNode(QSGNode *node, int)
{
	rootNode->appendChildNode(node);
}

// Currently this does an inefficient linear search in the chart-item vector.
// The reason is that removing individual chart items is very rare: for now,
// it is only done when hiding an InfoBox. In the future, this might have to
// be improved.
void StatsView::unregisterChartItem(const ChartItem *item)
{
	auto it = std::find(items.begin(), items.end(), item);
	if (it != items.end())
		items.erase(it);
}

QQuickWindow *StatsView::w() const
{
	return window();
}

QSizeF StatsView::size() const
{
	return boundingRect().size();
}

void StatsView::plotAreaChanged(const QSizeF &s)
{
	// Make sure that image is at least one pixel wide / high, otherwise
	// the painter starts acting up.
	int w = std::max(1, static_cast<int>(floor(s.width())));
	int h = std::max(1, static_cast<int>(floor(s.height())));
	scene.setSceneRect(QRectF(0, 0, static_cast<double>(w), static_cast<double>(h)));
	painter.reset();
	img.reset(new QImage(w, h, QImage::Format_RGB32));
	painter.reset(new QPainter(img.get()));
	painter->setRenderHint(QPainter::Antialiasing);

	double left = sceneBorder;
	double top = sceneBorder;
	double right = s.width() - sceneBorder;
	double bottom = s.height() - sceneBorder;
	const double minSize = 30.0;

	if (title)
		top += title->boundingRect().height() + titleBorder;
	// Currently, we only have either none, or an x- and a y-axis
	std::pair<double,double> horizontalSpace{ 0.0, 0.0 };
	if (xAxis) {
		bottom -= xAxis->height();
		horizontalSpace = xAxis->horizontalOverhang();
	}
	if (bottom - top < minSize)
		return;
	if (yAxis) {
		yAxis->setSize(bottom - top);
		horizontalSpace.first = std::max(horizontalSpace.first, yAxis->width());
	}
	left += horizontalSpace.first;
	right -= horizontalSpace.second;
	if (yAxis)
		yAxis->setPos(QPointF(left, bottom));
	if (right - left < minSize)
		return;
	if (xAxis) {
		xAxis->setSize(right - left);
		xAxis->setPos(QPointF(left, bottom));
	}

	if (grid)
		grid->updatePositions();
	for (auto &series: series)
		series->updatePositions();
	for (QuartileMarker &marker: quartileMarkers)
		marker.updatePosition();
	for (RegressionLine &line: regressionLines)
		line.updatePosition();
	for (HistogramMarker &marker: histogramMarkers)
		marker.updatePosition();
	if (legend)
		legend->resize();
	updateTitlePos();
}

void StatsView::replotIfVisible()
{
	if (isVisible())
		plot(state);
}

void StatsView::mouseMoveEvent(QMouseEvent *event)
{
	if (!draggedItem)
		return;

	QSizeF sceneSize = size();
	if (sceneSize.width() <= 1.0 || sceneSize.height() <= 1.0)
		return;
	QPointF pos = event->pos() - dragStartMouse + dragStartItem;;
	QSizeF itemSize = draggedItem->getRect().size();
	double widthHalf = floor(itemSize.width() / 2);
	double heightHalf = floor(itemSize.height() / 2);
	QSizeF itemSizeHalf(floor(itemSize.width() / 2), floor(itemSize.height() / 2));
	QPointF sanitizedPos(std::clamp(pos.x(), -widthHalf, sceneSize.width() - widthHalf - 1.0),
			     std::clamp(pos.y(), -heightHalf, sceneSize.height() - heightHalf - 1.0));
	draggedItem->setPos(sanitizedPos);
	update();
}

void StatsView::hoverEnterEvent(QHoverEvent *)
{
}

void StatsView::hoverMoveEvent(QHoverEvent *event)
{
	QPointF pos = event->pos();

	for (auto &series: series) {
		if (series->hover(pos)) {
			if (series.get() != highlightedSeries) {
				if (highlightedSeries)
					highlightedSeries->unhighlight();
				highlightedSeries = series.get();
			}
			return update();
		}
	}

	// No series was highlighted -> unhighlight any previously highlighted series.
	if (highlightedSeries) {
		highlightedSeries->unhighlight();
		highlightedSeries = nullptr;
		update();
	}
}

template <typename T, class... Args>
T *StatsView::createSeries(Args&&... args)
{
	T *res = new T(&scene, *this, xAxis, yAxis, std::forward<Args>(args)...);
	series.emplace_back(res);
	series.back()->updatePositions();
	return res;
}

void StatsView::setTitle(const QString &s)
{
	if (s.isEmpty()) {
		title.reset();
		return;
	}
	title = createItemPtr<QGraphicsSimpleTextItem>(&scene, s);
	title->setFont(titleFont);
}

void StatsView::updateTitlePos()
{
	if (!title)
		return;
	QRectF rect = scene.sceneRect();
	title->setPos(sceneBorder + (rect.width() - title->boundingRect().width()) / 2.0,
		      sceneBorder);
}

template <typename T, class... Args>
T *StatsView::createAxis(const QString &title, Args&&... args)
{
	T *res = createItem<T>(&scene, title, std::forward<Args>(args)...);
	axes.emplace_back(res);
	return res;
}

template <typename T, class... Args>
std::unique_ptr<T> StatsView::createChartItem(Args&&... args)
{
	std::unique_ptr<T> res(new T(*this, std::forward<Args>(args)...));
	items.push_back(res.get());
	return res;
}

void StatsView::setAxes(StatsAxis *x, StatsAxis *y)
{
	xAxis = x;
	yAxis = y;
	if (x && y)
		grid = std::make_unique<StatsGrid>(&scene, *x, *y);
}

void StatsView::reset()
{
	highlightedSeries = nullptr;
	xAxis = yAxis = nullptr;
	draggedItem = nullptr;
	items.clear(); // non-owning pointers
	legend.reset();
	series.clear();
	quartileMarkers.clear();
	regressionLines.clear();
	histogramMarkers.clear();
	grid.reset();
	axes.clear();
	title.reset();
}

void StatsView::plot(const StatsState &stateIn)
{
	state = stateIn;
	plotChart();
	plotAreaChanged(scene.sceneRect().size());
	update();
}

void StatsView::plotChart()
{
	if (!state.var1)
		return;
	reset();

	const std::vector<dive *> dives = DiveFilter::instance()->visibleDives();
	switch (state.type) {
	case ChartType::DiscreteBar:
		return plotBarChart(dives, state.subtype, state.var1, state.var1Binner, state.var2,
				    state.var2Binner, state.labels, state.legend);
	case ChartType::DiscreteValue:
		return plotValueChart(dives, state.subtype, state.var1, state.var1Binner, state.var2,
				      state.var2Operation, state.labels);
	case ChartType::DiscreteCount:
		return plotDiscreteCountChart(dives, state.subtype, state.var1, state.var1Binner, state.labels);
	case ChartType::Pie:
		return plotPieChart(dives, state.var1, state.var1Binner, state.labels, state.legend);
	case ChartType::DiscreteBox:
		return plotDiscreteBoxChart(dives, state.var1, state.var1Binner, state.var2);
	case ChartType::DiscreteScatter:
		return plotDiscreteScatter(dives, state.var1, state.var1Binner, state.var2, state.quartiles);
	case ChartType::HistogramCount:
		return plotHistogramCountChart(dives, state.subtype, state.var1, state.var1Binner,
					       state.labels, state.median, state.mean);
	case ChartType::HistogramValue:
		return plotHistogramValueChart(dives, state.subtype, state.var1, state.var1Binner, state.var2,
					       state.var2Operation, state.labels);
	case ChartType::HistogramStacked:
		return plotHistogramStackedChart(dives, state.subtype, state.var1, state.var1Binner,
						 state.var2, state.var2Binner, state.labels, state.legend);
	case ChartType::HistogramBox:
		return plotHistogramBoxChart(dives, state.var1, state.var1Binner, state.var2);
	case ChartType::ScatterPlot:
		return plotScatter(dives, state.var1, state.var2);
	case ChartType::Invalid:
		return;
	default:
		qWarning("Unknown chart type: %d", (int)state.type);
		return;
	}
}

template<typename T>
CategoryAxis *StatsView::createCategoryAxis(const QString &name, const StatsBinner &binner,
					    const std::vector<T> &bins, bool isHorizontal)
{
	std::vector<QString> labels;
	labels.reserve(bins.size());
	for (const auto &[bin, dummy]: bins)
		labels.push_back(binner.format(*bin));
	return createAxis<CategoryAxis>(name, labels, isHorizontal);
}

CountAxis *StatsView::createCountAxis(int maxVal, bool isHorizontal)
{
	return createAxis<CountAxis>(StatsTranslations::tr("No. dives"), maxVal, isHorizontal);
}

// For "two-dimensionally" binned plots (eg. stacked bar or grouped bar):
// Counts for each bin on the independent variable, including the total counts for that bin.
struct BinCounts {
	StatsBinPtr bin;
	std::vector<int> counts;
	int total;
};

// The problem with bar plots is that for different category
// bins, we might get different value bins. So we have to keep track
// of our counts and adjust accordingly. That's a bit annoying.
// Perhaps we should determine the bins of all dives first and then
// query the counts for precisely those bins?
struct BarPlotData {
	std::vector<BinCounts> hbin_counts; // For each category bin the counts for all value bins
	std::vector<StatsBinPtr> vbins;
	std::vector<QString> vbinNames;
	int maxCount;				// Highest count of any bin-combination
	int maxCategoryCount;			// Highest count of any category bin
	// Attention: categoryBin argument will be consumed!
	BarPlotData(std::vector<StatsBinDives> &categoryBins, const StatsBinner &valuebinner);
};

BarPlotData::BarPlotData(std::vector<StatsBinDives> &categoryBins, const StatsBinner &valueBinner) :
	maxCount(0), maxCategoryCount(0)
{
	for (auto &[bin, dives]: categoryBins) {
		// This moves the bin - the original pointer is invalidated
		hbin_counts.push_back({ std::move(bin), std::vector<int>(vbins.size(), 0), 0 });
		for (auto &[vbin, count]: valueBinner.count_dives(dives, false)) {
			// Note: we assume that the bins are sorted!
			auto it = std::lower_bound(vbins.begin(), vbins.end(), vbin,
						   [] (const StatsBinPtr &p, const StatsBinPtr &bin)
						   { return *p < *bin; });
			ssize_t pos = it - vbins.begin();
			if (it == vbins.end() || **it != *vbin) {
				// Add a new value bin.
				// Attn: this invalidates "vbin", which must not be used henceforth!
				vbins.insert(it, std::move(vbin));
				// Fix the old arrays
				for (auto &[bin, v, total]: hbin_counts)
					v.insert(v.begin() + pos, 0);
			}
			hbin_counts.back().counts[pos] = count;
			hbin_counts.back().total += count;
			if (count > maxCount)
				maxCount = count;
		}
		maxCategoryCount = std::max(maxCategoryCount, hbin_counts.back().total);
	}

	vbinNames.reserve(vbins.size());
	for (const auto &vbin: vbins)
		vbinNames.push_back(valueBinner.formatWithUnit(*vbin));
}

// Formats "x (y%)" as either a single or two strings for horizontal and non-horizontal cases, respectively.
static std::vector<QString> makePercentageLabels(int count, int total, bool isHorizontal)
{
	double percentage = count * 100.0 / total;
	QString countString = QString("%L1").arg(count);
	QString percentageString = QString("%L1%").arg(percentage, 0, 'f', 1);
	if (isHorizontal)
		return { QString("%1 (%2)").arg(countString, percentageString) };
	else
		return { countString, percentageString };
}

// From a list of counts, make (count, label) pairs, where the label
// formats the total number and the percentage of dives.
static std::vector<std::pair<int, std::vector<QString>>> makeCountLabels(const std::vector<int> &counts, int total,
									 bool labels, bool isHorizontal)
{
	std::vector<std::pair<int, std::vector<QString>>> count_labels;
	count_labels.reserve(counts.size());
	for (int count: counts) {
		std::vector<QString> label = labels ? makePercentageLabels(count, total, isHorizontal)
						    : std::vector<QString>();
		count_labels.push_back(std::make_pair(count, label));
	}
	return count_labels;
}

void StatsView::plotBarChart(const std::vector<dive *> &dives,
			     ChartSubType subType,
			     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			     const StatsVariable *valueVariable, const StatsBinner *valueBinner, bool labels, bool showLegend)
{
	if (!categoryBinner || !valueBinner)
		return;

	setTitle(valueVariable->nameWithBinnerUnit(*valueBinner));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	bool isStacked = subType == ChartSubType::VerticalStacked || subType == ChartSubType::HorizontalStacked;
	bool isHorizontal = subType == ChartSubType::HorizontalGrouped || subType == ChartSubType::HorizontalStacked;

	// Construct the histogram axis now, because the pointers to the bins
	// will be moved away when constructing BarPlotData below.
	CategoryAxis *catAxis = createCategoryAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						   *categoryBinner, categoryBins, !isHorizontal);

	BarPlotData data(categoryBins, *valueBinner);

	int maxVal = isStacked ? data.maxCategoryCount : data.maxCount;
	CountAxis *valAxis = createCountAxis(maxVal, isHorizontal);

	if (isHorizontal)
		setAxes(valAxis, catAxis);
	else
		setAxes(catAxis, valAxis);

	// Paint legend first, because the bin-names will be moved away from.
	if (showLegend)
		legend = createChartItem<Legend>(data.vbinNames);

	std::vector<BarSeries::MultiItem> items;
	items.reserve(data.hbin_counts.size());
	double pos = 0.0;
	for (auto &[hbin, counts, total]: data.hbin_counts) {
		items.push_back({ pos - 0.5, pos + 0.5, makeCountLabels(counts, total, labels, isHorizontal),
				  categoryBinner->formatWithUnit(*hbin) });
		pos += 1.0;
	}

	createSeries<BarSeries>(isHorizontal, isStacked, categoryVariable->name(), valueVariable, std::move(data.vbinNames), items);
}

const double NaN = std::numeric_limits<double>::quiet_NaN();

// These templates are used to extract min and max y-values of various lists.
// A bit too convoluted for my tastes - can we make that simpler?
static std::pair<double, double> getMinMaxValueBase(const std::vector<StatsValue> &values)
{
	// Attention: this supposes that the list is sorted!
	return values.empty() ? std::make_pair(NaN, NaN) : std::make_pair(values.front().v, values.back().v);
}
static std::pair<double, double> getMinMaxValueBase(double v)
{
	return { v, v };
}
static std::pair<double, double> getMinMaxValueBase(const StatsQuartiles &q)
{
	return { q.min, q.max };
}
static std::pair<double, double> getMinMaxValueBase(const StatsScatterItem &s)
{
	return { s.y, s.y };
}
template <typename T1, typename T2>
static std::pair<double, double> getMinMaxValueBase(const std::pair<T1, T2> &p)
{
	return getMinMaxValueBase(p.second);
}
template <typename T>
static std::pair<double, double> getMinMaxValueBase(const StatsBinValue<T> &v)
{
	return getMinMaxValueBase(v.value);
}

template <typename T>
static void updateMinMax(double &min, double &max, bool &found, const T &v)
{
	const auto [mi, ma] = getMinMaxValueBase(v);
	if (!std::isnan(mi) && mi < min)
		min = mi;
	if (!std::isnan(ma) && ma > max)
		max = ma;
	if (!std::isnan(mi) || !std::isnan(ma))
		found = true;
}

template <typename T>
static std::pair<double, double> getMinMaxValue(const std::vector<T> &values)
{
	double min = 1e14, max = 0.0;
	bool found = false;
	for (const T &v: values)
		updateMinMax(min, max, found, v);
	return found ? std::make_pair(min, max) : std::make_pair(0.0, 0.0);
}

static std::pair<double, double> getMinMaxValue(const std::vector<StatsBinOp> &bins, StatsOperation op)
{
	double min = 1e14, max = 0.0;
	bool found = false;
	for (auto &[bin, res]: bins) {
		if (!res.isValid())
			continue;
		updateMinMax(min, max, found, res.get(op));
	}
	return found ? std::make_pair(min, max) : std::make_pair(0.0, 0.0);
}

void StatsView::plotValueChart(const std::vector<dive *> &dives,
			       ChartSubType subType,
			       const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			       const StatsVariable *valueVariable, StatsOperation valueAxisOperation,
			       bool labels)
{
	if (!categoryBinner)
		return;

	setTitle(QStringLiteral("%1 (%2)").arg(valueVariable->name(), StatsVariable::operationName(valueAxisOperation)));

	std::vector<StatsBinOp> categoryBins = valueVariable->bin_operations(*categoryBinner, dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;


	bool isHorizontal = subType == ChartSubType::Horizontal;
	const auto [minValue, maxValue] = getMinMaxValue(categoryBins, valueAxisOperation);
	int decimals = valueVariable->decimals();
	CategoryAxis *catAxis = createCategoryAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						   *categoryBinner, categoryBins, !isHorizontal);
	ValueAxis *valAxis = createAxis<ValueAxis>(valueVariable->nameWithUnit(),
						   0.0, maxValue, valueVariable->decimals(), isHorizontal);

	if (isHorizontal)
		setAxes(valAxis, catAxis);
	else
		setAxes(catAxis, valAxis);

	std::vector<BarSeries::ValueItem> items;
	items.reserve(categoryBins.size());
	double pos = 0.0;
	QString unit = valueVariable->unitSymbol();
	for (auto &[bin, res]: categoryBins) {
		if (res.isValid()) {
			double height = res.get(valueAxisOperation);
			QString value = QString("%L1").arg(height, 0, 'f', decimals);
			std::vector<QString> label = labels ? std::vector<QString> { value }
							    : std::vector<QString>();
			items.push_back({ pos - 0.5, pos + 0.5, height, label,
					  categoryBinner->formatWithUnit(*bin), res });
		}
		pos += 1.0;
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), valueVariable, items);
}

static int getTotalCount(const std::vector<StatsBinCount> &bins)
{
	int total = 0;
	for (const auto &[bin, count]: bins)
		total += count;
	return total;
}

template<typename T>
static int getMaxCount(const std::vector<T> &bins)
{
	int res = 0;
	for (auto const &[dummy, val]: bins) {
		if (val > res)
			res = val;
	}
	return res;
}

void StatsView::plotDiscreteCountChart(const std::vector<dive *> &dives,
				      ChartSubType subType,
				      const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				      bool labels)
{
	if (!categoryBinner)
		return;

	setTitle(categoryVariable->nameWithBinnerUnit(*categoryBinner));

	std::vector<StatsBinCount> categoryBins = categoryBinner->count_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	int total = getTotalCount(categoryBins);
	bool isHorizontal = subType != ChartSubType::Vertical;

	CategoryAxis *catAxis = createCategoryAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						   *categoryBinner, categoryBins, !isHorizontal);

	int maxCount = getMaxCount(categoryBins);
	CountAxis *valAxis = createCountAxis(maxCount, isHorizontal);

	if (isHorizontal)
		setAxes(valAxis, catAxis);
	else
		setAxes(catAxis, valAxis);

	std::vector<BarSeries::CountItem> items;
	items.reserve(categoryBins.size());
	double pos = 0.0;
	for (auto const &[bin, count]: categoryBins) {
		std::vector<QString> label = labels ? makePercentageLabels(count, total, isHorizontal)
						    : std::vector<QString>();
		items.push_back({ pos - 0.5, pos + 0.5, count, label,
				  categoryBinner->formatWithUnit(*bin), total });
		pos += 1.0;
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), items);
}

void StatsView::plotPieChart(const std::vector<dive *> &dives,
			     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			     bool labels, bool showLegend)
{
	if (!categoryBinner)
		return;

	setTitle(categoryVariable->nameWithBinnerUnit(*categoryBinner));

	std::vector<StatsBinCount> categoryBins = categoryBinner->count_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	std::vector<std::pair<QString, int>> data;
	data.reserve(categoryBins.size());
	for (auto const &[bin, count]: categoryBins)
		data.emplace_back(categoryBinner->formatWithUnit(*bin), count);

	bool keepOrder = categoryVariable->type() != StatsVariable::Type::Discrete;
	PieSeries *series = createSeries<PieSeries>(categoryVariable->name(), data, keepOrder, labels);

	if (showLegend)
		legend = createChartItem<Legend>(series->binNames());
}

void StatsView::plotDiscreteBoxChart(const std::vector<dive *> &dives,
				     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				     const StatsVariable *valueVariable)
{
	if (!categoryBinner)
		return;

	setTitle(valueVariable->name());

	std::vector<StatsBinQuartiles> categoryBins = valueVariable->bin_quartiles(*categoryBinner, dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	CategoryAxis *catAxis = createCategoryAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						   *categoryBinner, categoryBins, true);

	auto [minY, maxY] = getMinMaxValue(categoryBins);
	ValueAxis *valueAxis = createAxis<ValueAxis>(valueVariable->nameWithUnit(),
						     minY, maxY, valueVariable->decimals(), false);

	setAxes(catAxis, valueAxis);

	BoxSeries *series = createSeries<BoxSeries>(valueVariable->name(), valueVariable->unitSymbol(), valueVariable->decimals());

	double pos = 0.0;
	for (auto &[bin, q]: categoryBins) {
		if (q.isValid())
			series->append(pos - 0.5, pos + 0.5, q, categoryBinner->formatWithUnit(*bin));
		pos += 1.0;
	}
}

void StatsView::plotDiscreteScatter(const std::vector<dive *> &dives,
				    const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				    const StatsVariable *valueVariable, bool quartiles)
{
	if (!categoryBinner)
		return;

	setTitle(valueVariable->name());

	std::vector<StatsBinValues> categoryBins = valueVariable->bin_values(*categoryBinner, dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	CategoryAxis *catAxis = createCategoryAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
			 			   *categoryBinner, categoryBins, true);

	auto [minValue, maxValue] = getMinMaxValue(categoryBins);

	ValueAxis *valAxis = createAxis<ValueAxis>(valueVariable->nameWithUnit(),
						   minValue, maxValue, valueVariable->decimals(), false);

	setAxes(catAxis, valAxis);
	ScatterSeries *series = createSeries<ScatterSeries>(*categoryVariable, *valueVariable);

	double x = 0.0;
	for (const auto &[bin, array]: categoryBins) {
		for (auto [v, d]: array)
			series->append(d, x, v);
		if (quartiles) {
			StatsQuartiles quartiles = StatsVariable::quartiles(array);
			if (quartiles.isValid()) {
				quartileMarkers.emplace_back(x, quartiles.q1, &scene, catAxis, valAxis);
				quartileMarkers.emplace_back(x, quartiles.q2, &scene, catAxis, valAxis);
				quartileMarkers.emplace_back(x, quartiles.q3, &scene, catAxis, valAxis);
			}
		}
		x += 1.0;
	}
}

StatsView::QuartileMarker::QuartileMarker(double pos, double value, QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis) :
	item(createItemPtr<QGraphicsLineItem>(scene)),
	xAxis(xAxis), yAxis(yAxis),
	pos(pos),
	value(value)
{
	item->setZValue(ZValues::chartFeatures);
	item->setPen(QPen(quartileMarkerColor, 2.0));
	updatePosition();
}

void StatsView::QuartileMarker::updatePosition()
{
	if (!xAxis || !yAxis)
		return;
	double x = xAxis->toScreen(pos);
	double y = yAxis->toScreen(value);
	item->setLine(x - quartileMarkerSize / 2.0, y,
		      x + quartileMarkerSize / 2.0, y);
}

StatsView::RegressionLine::RegressionLine(const struct regression_data reg, QBrush brush, QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis) :
	item(createItemPtr<QGraphicsPolygonItem>(scene)),
	central(createItemPtr<QGraphicsPolygonItem>(scene)),
	xAxis(xAxis), yAxis(yAxis),
	reg(reg)
{
	item->setZValue(ZValues::chartFeatures);
	item->setPen(Qt::NoPen);
	item->setBrush(brush);

	central->setZValue(ZValues::chartFeatures+1);
	central->setPen(QPen(Qt::red));
}

void StatsView::RegressionLine::updatePosition()
{
	if (!xAxis || !yAxis)
		return;
	auto [minX, maxX] = xAxis->minMax();
	auto [minY, maxY] = yAxis->minMax();

	QPolygonF line;
	line << QPoint(xAxis->toScreen(minX), yAxis->toScreen(reg.a * minX + reg.b))
		<< QPoint(xAxis->toScreen(maxX), yAxis->toScreen(reg.a * maxX + reg.b));

	// Draw the confidence interval according to http://www2.stat.duke.edu/~tjl13/s101/slides/unit6lec3H.pdf p.5 with t*=2 for 95% confidence
	QPolygonF poly;
	for (double x = minX; x <= maxX + 1; x += (maxX - minX) / 100)
		poly << QPointF(xAxis->toScreen(x),
			yAxis->toScreen(reg.a * x + reg.b + 2.0 * sqrt(reg.res2 / (reg.n - 2)  * (1.0 / reg.n + (x - reg.xavg) * (x - reg.xavg) / (reg.n - 1) * (reg.n -2) / reg.sx2))));
	for (double x = maxX; x >= minX - 1; x -= (maxX - minX) / 100)
		poly << QPointF(xAxis->toScreen(x),
			yAxis->toScreen(reg.a * x + reg.b - 2.0 * sqrt(reg.res2 / (reg.n - 2)  * (1.0 / reg.n + (x - reg.xavg) * (x - reg.xavg) / (reg.n - 1) * (reg.n -2) / reg.sx2))));
	QRectF box(QPoint(xAxis->toScreen(minX), yAxis->toScreen(minY)), QPoint(xAxis->toScreen(maxX), yAxis->toScreen(maxY)));

	item->setPolygon(poly.intersected(box));
	central->setPolygon(line.intersected(box));
}

StatsView::HistogramMarker::HistogramMarker(double val, bool horizontal, QPen pen, QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis) :
	item(createItemPtr<QGraphicsLineItem>(scene)),
	xAxis(xAxis), yAxis(yAxis),
	val(val), horizontal(horizontal)
{
	item->setZValue(ZValues::chartFeatures);
	item->setPen(pen);
}

void StatsView::HistogramMarker::updatePosition()
{
	if (!xAxis || !yAxis)
		return;
	if (horizontal) {
		double y = yAxis->toScreen(val);
		auto [x1, x2] = xAxis->minMaxScreen();
		item->setLine(x1, y, x2, y);
	} else {
		double x = xAxis->toScreen(val);
		auto [y1, y2] = yAxis->minMaxScreen();
		item->setLine(x, y1, x, y2);
	}
}

void StatsView::addHistogramMarker(double pos, const QPen &pen, bool isHorizontal, StatsAxis *xAxis, StatsAxis *yAxis)
{
	histogramMarkers.emplace_back(pos, isHorizontal, pen, &scene, xAxis, yAxis);
}

void StatsView::addLinearRegression(const struct regression_data reg, StatsAxis *xAxis, StatsAxis *yAxis)
{
	QColor red = QColor(Qt::red);
	red.setAlphaF(reg.r2);
	QPen pen(red);
	QBrush brush(red);
	brush.setStyle(Qt::SolidPattern);

	regressionLines.emplace_back(reg, brush, &scene, xAxis, yAxis);
}

// Yikes, we get our data in different kinds of (bin, value) pairs.
// To create a category axis from this, we have to templatify the function.
template<typename T>
HistogramAxis *StatsView::createHistogramAxis(const QString &name, const StatsBinner &binner,
					      const std::vector<T> &bins, bool isHorizontal)
{
	std::vector<HistogramAxisEntry> labels;
	for (auto const &[bin, dummy]: bins) {
		QString label = binner.formatLowerBound(*bin);
		double lowerBound = binner.lowerBoundToFloat(*bin);
		bool prefer = binner.preferBin(*bin);
		labels.push_back({ label, lowerBound, prefer });
	}

	const StatsBin &lastBin = *bins.back().bin;
	QString lastLabel = binner.formatUpperBound(lastBin);
	double upperBound = binner.upperBoundToFloat(lastBin);
	labels.push_back({ lastLabel, upperBound, false });

	return createAxis<HistogramAxis>(name, std::move(labels), isHorizontal);
}

void StatsView::plotHistogramCountChart(const std::vector<dive *> &dives,
					ChartSubType subType,
					const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
					bool labels, bool showMedian, bool showMean)
{
	if (!categoryBinner)
		return;

	setTitle(categoryVariable->name());

	std::vector<StatsBinCount> categoryBins = categoryBinner->count_dives(dives, true);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	bool isHorizontal = subType == ChartSubType::Horizontal;
	HistogramAxis *catAxis = createHistogramAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						     *categoryBinner, categoryBins, !isHorizontal);

	int maxCategoryCount = getMaxCount(categoryBins);
	int total = getTotalCount(categoryBins);

	StatsAxis *valAxis = createCountAxis(maxCategoryCount, isHorizontal);

	if (isHorizontal)
		setAxes(valAxis, catAxis);
	else
		setAxes(catAxis, valAxis);

	std::vector<BarSeries::CountItem> items;
	items.reserve(categoryBins.size());

	for (auto const &[bin, count]: categoryBins) {
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		std::vector<QString> label = labels ? makePercentageLabels(count, total, isHorizontal)
						    : std::vector<QString>();

		items.push_back({ lowerBound, upperBound, count, label,
				  categoryBinner->formatWithUnit(*bin), total });
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), items);

	if (categoryVariable->type() == StatsVariable::Type::Numeric) {
		if (showMean) {
			double mean = categoryVariable->mean(dives);
			QPen meanPen(Qt::green);
			meanPen.setWidth(2);
			if (!std::isnan(mean))
				addHistogramMarker(mean, meanPen, isHorizontal, xAxis, yAxis);
		}
		if (showMedian) {
			double median = categoryVariable->quartiles(dives).q2;
			QPen medianPen(Qt::red);
			medianPen.setWidth(2);
			if (!std::isnan(median))
				addHistogramMarker(median, medianPen, isHorizontal, xAxis, yAxis);
		}
	}
}

void StatsView::plotHistogramValueChart(const std::vector<dive *> &dives,
					ChartSubType subType,
					const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
					const StatsVariable *valueVariable, StatsOperation valueAxisOperation,
					bool labels)
{
	if (!categoryBinner)
		return;

	setTitle(QStringLiteral("%1 (%2)").arg(valueVariable->name(), StatsVariable::operationName(valueAxisOperation)));

	std::vector<StatsBinOp> categoryBins = valueVariable->bin_operations(*categoryBinner, dives, true);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	bool isHorizontal = subType == ChartSubType::Horizontal;
	HistogramAxis *catAxis = createHistogramAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						     *categoryBinner, categoryBins, !isHorizontal);

	const auto [minValue, maxValue] = getMinMaxValue(categoryBins, valueAxisOperation);

	int decimals = valueVariable->decimals();
	ValueAxis *valAxis = createAxis<ValueAxis>(valueVariable->nameWithUnit(),
						   0.0, maxValue, decimals, isHorizontal);

	if (isHorizontal)
		setAxes(valAxis, catAxis);
	else
		setAxes(catAxis, valAxis);

	std::vector<BarSeries::ValueItem> items;
	items.reserve(categoryBins.size());

	QString unit = valueVariable->unitSymbol();
	for (auto const &[bin, res]: categoryBins) {
		if (!res.isValid())
			continue;
		double height = res.get(valueAxisOperation);
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		QString value = QString("%L1").arg(height, 0, 'f', decimals);
		std::vector<QString> label = labels ? std::vector<QString> { value }
						    : std::vector<QString>();
		items.push_back({ lowerBound, upperBound, height, label,
				  categoryBinner->formatWithUnit(*bin), res });
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), valueVariable, items);
}

void StatsView::plotHistogramStackedChart(const std::vector<dive *> &dives,
					  ChartSubType subType,
					  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
					  const StatsVariable *valueVariable, const StatsBinner *valueBinner, bool labels, bool showLegend)
{
	if (!categoryBinner || !valueBinner)
		return;

	setTitle(valueVariable->nameWithBinnerUnit(*valueBinner));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, true);

	// Construct the histogram axis now, because the pointers to the bins
	// will be moved away when constructing BarPlotData below.
	bool isHorizontal = subType == ChartSubType::HorizontalStacked;
	HistogramAxis *catAxis = createHistogramAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						     *categoryBinner, categoryBins, !isHorizontal);

	BarPlotData data(categoryBins, *valueBinner);
	if (showLegend)
		legend = createChartItem<Legend>(data.vbinNames);

	CountAxis *valAxis = createCountAxis(data.maxCategoryCount, isHorizontal);

	if (isHorizontal)
		setAxes(valAxis, catAxis);
	else
		setAxes(catAxis, valAxis);

	std::vector<BarSeries::MultiItem> items;
	items.reserve(data.hbin_counts.size());

	for (auto &[hbin, counts, total]: data.hbin_counts) {
		double lowerBound = categoryBinner->lowerBoundToFloat(*hbin);
		double upperBound = categoryBinner->upperBoundToFloat(*hbin);
		items.push_back({ lowerBound, upperBound, makeCountLabels(counts, total, labels, isHorizontal),
				  categoryBinner->formatWithUnit(*hbin) });
	}

	createSeries<BarSeries>(isHorizontal, true, categoryVariable->name(), valueVariable, std::move(data.vbinNames), items);
}

void StatsView::plotHistogramBoxChart(const std::vector<dive *> &dives,
				      const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				      const StatsVariable *valueVariable)
{
	if (!categoryBinner)
		return;

	setTitle(valueVariable->name());

	std::vector<StatsBinQuartiles> categoryBins = valueVariable->bin_quartiles(*categoryBinner, dives, true);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	HistogramAxis *catAxis = createHistogramAxis(categoryVariable->nameWithBinnerUnit(*categoryBinner),
						     *categoryBinner, categoryBins, true);

	auto [minY, maxY] = getMinMaxValue(categoryBins);
	ValueAxis *valueAxis = createAxis<ValueAxis>(valueVariable->nameWithUnit(),
						     minY, maxY, valueVariable->decimals(), false);

	setAxes(catAxis, valueAxis);

	BoxSeries *series = createSeries<BoxSeries>(valueVariable->name(), valueVariable->unitSymbol(), valueVariable->decimals());

	for (auto &[bin, q]: categoryBins) {
		if (!q.isValid())
			continue;
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		series->append(lowerBound, upperBound, q, categoryBinner->formatWithUnit(*bin));
	}
}

static bool is_linear_regression(int sample_size, double cov, double sx2, double sy2)
{
	// One point never, two points always form a line
	if (sample_size < 2)
		return false;
	if (sample_size <= 2)
		return true;

	const double tval[] = { 12.709, 4.303, 3.182, 2.776, 2.571, 2.447, 2.201, 2.120, 2.080,  2.056, 2.021, 1.960,  1.960 };
	const int t_df[] =    { 1,      2,     3,     4,     5,     6,     11,    16,    21,     26,    40,    100,   100000 };
	int df = sample_size - 2;   // Following is the one-tailed t-value at p < 0.05 and [sample_size - 2] degrees of freedom for the dive data:
	double t = (cov / sx2) / sqrt(((sy2 - cov * cov / sx2) / (double)df) / sx2);
	for (int i = std::size(tval) - 2; i >= 0; i--) {   // We do linear interpolation rather than having a large lookup table.
		if (df >= t_df[i]) {    // Look up the appropriate reference t-value at p < 0.05 and df degrees of freedom
			double t_lookup = tval[i] - (tval[i] - tval[i+1]) * (df - t_df[i]) / (t_df[i+1] - t_df[i]);
			return abs(t) >= t_lookup;
		}
	}

	return true; // can't happen, as we tested for sample_size above.
}

// Returns the coefficients a,b of the line y = ax + b
// as well as the variance of the residuals (averaged residual squared) as res2
// and r^2 = 1.0 - variance of data / res2 which is the fraction of the variance of
// the data that is explained by the linear regression.
// If case of an undetermined regression or one with infinite slope, returns {nan, nan, 0.0, 0.0}

static struct regression_data linear_regression(const std::vector<StatsScatterItem> &v)
{
	struct regression_data ret = { .a = NaN, .b = NaN, .res2 = 0.0, .r2 = 0.0, .sx2 = 0.0, .xavg = 0.0};
	ret.n = v.size();
	if (ret.n < 2)
		return ret;
	// First, calculate the x and y average
	double avg_x = 0.0, avg_y = 0.0;
	for (auto [x, y, d]: v) {
		avg_x += x;
		avg_y += y;
	}
	avg_x /= ret.n;
	avg_y /= ret.n;

	double cov = 0.0, sx2 = 0.0, sy2 = 0.0;
	for (auto [x, y, d]: v) {
		cov += (x - avg_x) * (y - avg_y);
		sx2 += (x - avg_x) * (x - avg_x);
		sy2 += (y - avg_y) * (y - avg_y);
	}

	bool is_linear = is_linear_regression((int)v.size(), cov, sx2, sy2);

	if (fabs(sx2) < 1e-10 || !is_linear) // If t is not statistically significant, do not plot the regression line.
		return ret;
	ret.xavg = avg_x;
	ret.sx2 = sx2;
	ret.a = cov / sx2;
	ret.b = avg_y - ret.a * avg_x;

	for (auto [x, y, d]: v)
		ret.res2 += (y - ret.a * x - ret.b) * (y - ret.a * x - ret.b);
	ret.r2 = sy2 > 0.0 ? 1.0 - ret.res2 / sy2 : 1.0;
	return ret;
}

void StatsView::plotScatter(const std::vector<dive *> &dives, const StatsVariable *categoryVariable, const StatsVariable *valueVariable)
{
	setTitle(StatsTranslations::tr("%1 vs. %2").arg(valueVariable->name(), categoryVariable->name()));

	std::vector<StatsScatterItem> points = categoryVariable->scatter(*valueVariable, dives);
	if (points.empty())
		return;

	double minX = points.front().x;
	double maxX = points.back().x;
	auto [minY, maxY] = getMinMaxValue(points);

	StatsAxis *axisX = categoryVariable->type() == StatsVariable::Type::Continuous ?
		static_cast<StatsAxis *>(createAxis<DateAxis>(categoryVariable->nameWithUnit(),
							      minX, maxX, true)) :
		static_cast<StatsAxis *>(createAxis<ValueAxis>(categoryVariable->nameWithUnit(),
							       minX, maxX, categoryVariable->decimals(), true));

	StatsAxis *axisY = createAxis<ValueAxis>(valueVariable->nameWithUnit(), minY, maxY, valueVariable->decimals(), false);

	setAxes(axisX, axisY);
	ScatterSeries *series = createSeries<ScatterSeries>(*categoryVariable, *valueVariable);

	for (auto [x, y, dive]: points)
		series->append(dive, x, y);

	// y = ax + b
	struct regression_data reg = linear_regression(points);
	if (!std::isnan(reg.a))
		addLinearRegression(reg, xAxis, yAxis);
}
