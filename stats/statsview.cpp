// SPDX-License-Identifier: GPL-2.0
#include "statsview.h"
#include "barseries.h"
#include "boxseries.h"
#include "histogrammarker.h"
#include "legend.h"
#include "pieseries.h"
#include "quartilemarker.h"
#include "regressionitem.h"
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
#include "core/selection.h"
#include "core/trip.h"

#include <array> // for std::array
#include <cmath>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QSGTexture>

// Constants that control the graph layouts
static const double sceneBorder = 5.0;			// Border between scene edges and statitistics view
static const double titleBorder = 2.0;			// Border between title and chart
static const double selectionLassoWidth = 2.0;		// Border between title and chart

StatsView::StatsView(QQuickItem *parent) : QQuickItem(parent),
	backgroundDirty(true),
	currentTheme(&getStatsTheme(false)),
	highlightedSeries(nullptr),
	xAxis(nullptr),
	yAxis(nullptr),
	draggedItem(nullptr),
	restrictDives(false),
	rootNode(nullptr)
{
	setFlag(ItemHasContents, true);

	connect(&diveListNotifier, &DiveListNotifier::numShownChanged, this, &StatsView::replotIfVisible);
	connect(&diveListNotifier, &DiveListNotifier::divesAdded, this, &StatsView::replotIfVisible);
	connect(&diveListNotifier, &DiveListNotifier::divesDeleted, this, &StatsView::replotIfVisible);
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &StatsView::replotIfVisible);
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &StatsView::replotIfVisible);
	connect(&diveListNotifier, &DiveListNotifier::divesSelected, this, &StatsView::divesSelected);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::LeftButton);
}

StatsView::StatsView() : StatsView(nullptr)
{
}

StatsView::~StatsView()
{
}

void StatsView::mousePressEvent(QMouseEvent *event)
{
	QPointF pos = event->localPos();

	// Currently, we only support dragging of the legend. If other objects
	// should be made draggable, this needs to be generalized.
	if (legend) {
		QRectF rect = legend->getRect();
		if (legend->getRect().contains(pos)) {
			dragStartMouse = pos;
			dragStartItem = rect.topLeft();
			draggedItem = &*legend;
			grabMouse();
			setKeepMouseGrab(true); // don't allow Qt to steal the grab
			return;
		}
	}

	SelectionModifier modifier;
	modifier.shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	modifier.ctrl = (event->modifiers() & Qt::ControlModifier) != 0;
	bool itemSelected = false;
	for (auto &series: series)
		itemSelected |= series->selectItemsUnderMouse(pos, modifier);

	// The user clicked in "empty" space. If there is a series supporting lasso-select,
	// got into lasso mode. For now, we only support a rectangular lasso.
	if (!itemSelected && std::any_of(series.begin(), series.end(),
					 [] (const std::unique_ptr<StatsSeries> &s)
					 { return s->supportsLassoSelection(); })) {
		if (selectionRect)
			deleteChartItem(selectionRect); // Ooops. Already a selection in place.
		dragStartMouse = pos;
		selectionRect = createChartItem<ChartRectLineItem>(ChartZValue::Selection, currentTheme->selectionLassoColor, selectionLassoWidth);
		selectionModifier = modifier;
		oldSelection = modifier.ctrl ? getDiveSelection() : std::vector<dive *>();
		grabMouse();
		setKeepMouseGrab(true); // don't allow Qt to steal the grab
		update();
	}
}

void StatsView::mouseReleaseEvent(QMouseEvent *)
{
	if (draggedItem) {
		draggedItem = nullptr;
		ungrabMouse();
	}

	if (selectionRect) {
		deleteChartItem(selectionRect);
		ungrabMouse();
		update();
	}
}

// Define a hideable dummy QSG node that is used as a parent node to make
// all objects of a z-level visible / invisible.
using ZNode = HideableQSGNode<QSGNode>;

class RootNode : public QSGNode
{
public:
	RootNode(StatsView &view);
	~RootNode();
	StatsView &view;
	std::unique_ptr<QSGRectangleNode> backgroundNode; // solid background
	// We entertain one node per Z-level.
	std::array<std::unique_ptr<ZNode>, (size_t)ChartZValue::Count> zNodes;
};

RootNode::RootNode(StatsView &view) : view(view)
{
	// Add a background rectangle with a solid color. This could
	// also be done on the widget level, but would have to be done
	// separately for desktop and mobile, so do it here.
	backgroundNode.reset(view.w()->createRectangleNode());
	backgroundNode->setColor(view.getCurrentTheme().backgroundColor);
	appendChildNode(backgroundNode.get());

	for (auto &zNode: zNodes) {
		zNode.reset(new ZNode(true));
		appendChildNode(zNode.get());
	}
}

RootNode::~RootNode()
{
	view.emergencyShutdown();
}

void StatsView::freeDeletedChartItems()
{
	ChartItem *nextitem;
	for (ChartItem *item = deletedItems.first; item; item = nextitem) {
		nextitem = item->next;
		delete item;
	}
	deletedItems.clear();
}

QSGNode *StatsView::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
	// The QtQuick drawing interface is utterly bizzare with a distinct 1980ies-style memory management.
	// This is just a copy of what is found in Qt's documentation.
	RootNode *n = static_cast<RootNode *>(oldNode);
	if (!n)
		n = rootNode = new RootNode(*this);

	// Delete all chart items that are marked for deletion.
	freeDeletedChartItems();

	if (backgroundDirty) {
		rootNode->backgroundNode->setRect(plotRect);
		backgroundDirty = false;
	}

	for (ChartItem *item = dirtyItems.first; item; item = item->next) {
		item->render(*currentTheme);
		item->dirty = false;
	}
	dirtyItems.splice(cleanItems);

	return n;
}

// When reparenting the QQuickWidget, QtQuick decides to delete our rootNode
// and with it all the QSG nodes, even though we have *not* given the
// permission to do so! If the widget is reused, we try to delete the
// stale items, whose nodes have already been deleted by QtQuick, leading
// to a double-free(). Instead of searching for the cause of this behavior,
// let's just hook into the rootNodes destructor and delete the objects
// in a controlled manner, so that QtQuick has no more access to them.
void StatsView::emergencyShutdown()
{
	// Mark clean and dirty chart items for deletion...
	cleanItems.splice(deletedItems);
	dirtyItems.splice(deletedItems);

	// ...and delete them.
	freeDeletedChartItems();

	// Now delete all the pointers we might have to chart features,
	// axes, etc. Note that all pointers to chart items are non
	// owning, so this only resets stale references, but does not
	// lead to any additional deletion of chart items.
	reset();

	// The rootNode is being deleted -> remove the reference to that
	rootNode = nullptr;
}

void StatsView::addQSGNode(QSGNode *node, ChartZValue z)
{
	int idx = std::clamp((int)z, 0, (int)ChartZValue::Count - 1);
	rootNode->zNodes[idx]->appendChildNode(node);
}

void StatsView::registerChartItem(ChartItem &item)
{
	cleanItems.append(item);
}

void StatsView::registerDirtyChartItem(ChartItem &item)
{
	if (item.dirty)
		return;
	cleanItems.remove(item);
	dirtyItems.append(item);
	item.dirty = true;
}

void StatsView::deleteChartItemInternal(ChartItem &item)
{
	if (item.dirty)
		dirtyItems.remove(item);
	else
		cleanItems.remove(item);
	deletedItems.append(item);
}

StatsView::ChartItemList::ChartItemList() : first(nullptr), last(nullptr)
{
}

void StatsView::ChartItemList::clear()
{
	first = last = nullptr;
}

void StatsView::ChartItemList::remove(ChartItem &item)
{
	if (item.next)
		item.next->prev = item.prev;
	else
		last = item.prev;
	if (item.prev)
		item.prev->next = item.next;
	else
		first = item.next;
	item.prev = item.next = nullptr;
}

void StatsView::ChartItemList::append(ChartItem &item)
{
	if (!first) {
		first = &item;
	} else {
		item.prev = last;
		last->next = &item;
	}
	last = &item;
}

void StatsView::ChartItemList::splice(ChartItemList &l2)
{
	if (!first) // if list is empty -> nothing to do.
		return;
	if (!l2.first) {
		l2 = *this;
	} else {
		l2.last->next = first;
		first->prev = l2.last;
		l2.last = last;
	}
	clear();
}

QQuickWindow *StatsView::w() const
{
	return window();
}

void StatsView::setTheme(bool dark)
{
	currentTheme = &getStatsTheme(dark);
	rootNode->backgroundNode->setColor(currentTheme->backgroundColor);
}

const StatsTheme &StatsView::getCurrentTheme() const
{
	return *currentTheme;
}

QSizeF StatsView::size() const
{
	return boundingRect().size();
}

QRectF StatsView::plotArea() const
{
	return plotRect;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void StatsView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void StatsView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
	plotRect = QRectF(QPointF(0.0, 0.0), newGeometry.size());
	backgroundDirty = true;
	plotAreaChanged(plotRect.size());

	// Do we need to call the base-class' version of geometryChanged? Probably for QML?
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QQuickItem::geometryChange(newGeometry, oldGeometry);
#else
	QQuickItem::geometryChanged(newGeometry, oldGeometry);
#endif
}

void StatsView::plotAreaChanged(const QSizeF &s)
{
	double left = sceneBorder;
	double top = sceneBorder;
	double right = s.width() - sceneBorder;
	double bottom = s.height() - sceneBorder;
	const double minSize = 30.0;

	if (title)
		top += title->getRect().height() + titleBorder;
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
	for (auto &marker: quartileMarkers)
		marker->updatePosition();
	if (regressionItem)
		regressionItem->updatePosition();
	if (meanMarker)
		meanMarker->updatePosition();
	if (medianMarker)
		medianMarker->updatePosition();
	if (legend)
		legend->resize();
	updateTitlePos();
}

void StatsView::replotIfVisible()
{
	if (isVisible())
		plot(state);
}

void StatsView::divesSelected(const QVector<dive *> &dives)
{
	if (isVisible()) {
		for (auto &series: series)
			series->divesSelected(dives);
	}
	update();
}

void StatsView::mouseMoveEvent(QMouseEvent *event)
{
	if (draggedItem) {
		QSizeF sceneSize = size();
		if (sceneSize.width() <= 1.0 || sceneSize.height() <= 1.0)
			return;
		draggedItem->setPos(event->pos() - dragStartMouse + dragStartItem);
		update();
	}

	if (selectionRect) {
		QPointF p1 = event->pos();
		QPointF p2 = dragStartMouse;
		selectionRect->setLine(p1, p2);
		QRectF rect(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()),
			    fabs(p2.x() - p1.x()), fabs(p2.y() - p1.y()));
		for (auto &series: series) {
			if (series->supportsLassoSelection())
				series->selectItemsInRect(rect, selectionModifier, oldSelection);
		}
		update();
	}
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
	T *res = new T(*this, xAxis, yAxis, std::forward<Args>(args)...);
	series.emplace_back(res);
	series.back()->updatePositions();
	return res;
}

void StatsView::setTitle(const QString &s)
{
	if (title) {
		// Ooops. Currently we do not support setting the title twice.
		return;
	}
	title = createChartItem<ChartTextItem>(ChartZValue::Legend, currentTheme->titleFont, s);
	title->setColor(currentTheme->darkLabelColor);
}

void StatsView::updateTitlePos()
{
	if (!title)
		return;
	QPointF pos(sceneBorder + (boundingRect().width() - title->getRect().width()) / 2.0, sceneBorder);
	title->setPos(roundPos(pos));
}

template <typename T, class... Args>
T *StatsView::createAxis(const QString &title, Args&&... args)
{
	return &*createChartItem<T>(title, std::forward<Args>(args)...);
}

void StatsView::setAxes(StatsAxis *x, StatsAxis *y)
{
	xAxis = x;
	yAxis = y;
	if (x && y)
		grid = std::make_unique<StatsGrid>(*this, *x, *y);
}

void StatsView::reset()
{
	highlightedSeries = nullptr;
	xAxis = yAxis = nullptr;
	draggedItem = nullptr;
	title.reset();
	legend.reset();
	regressionItem.reset();
	meanMarker.reset();
	medianMarker.reset();
	selectionRect.reset();

	// Mark clean and dirty chart items for deletion
	cleanItems.splice(deletedItems);
	dirtyItems.splice(deletedItems);

	series.clear();
	quartileMarkers.clear();
	grid.reset();
}

void StatsView::restrictToSelection()
{
	restrictedDives = getDiveSelection();
	std::sort(restrictedDives.begin(), restrictedDives.end()); // Sort by pointer for quick lookup
	restrictDives = true;
	plot(state);
}

void StatsView::unrestrict()
{
	restrictDives = false;
	plot(state);
}

int StatsView::restrictionCount() const
{
	return restrictDives ? (int)restrictedDives.size() : -1;
}

void StatsView::plot(const StatsState &stateIn)
{
	state = stateIn;
	plotChart();
	updateFeatures(); // Show / hide chart features, such as legend, etc.
	plotAreaChanged(plotRect.size());
	update();
}

void StatsView::updateFeatures(const StatsState &stateIn)
{
	state = stateIn;
	updateFeatures();
	update();
}

void StatsView::plotChart()
{
	if (!state.var1)
		return;
	reset();

	std::vector<dive *> dives;
	if (restrictDives) {
		std::vector<dive *> visible = DiveFilter::instance()->visibleDives();
		dives.reserve(visible.size());
		for (dive *d: visible) {
			// binary search
			auto it = std::lower_bound(restrictedDives.begin(), restrictedDives.end(), d);
			if (it != restrictedDives.end() && *it == d)
				dives.push_back(d);
		}
	} else {
		dives = DiveFilter::instance()->visibleDives();
	}
	switch (state.type) {
	case ChartType::DiscreteBar:
		return plotBarChart(dives, state.subtype, state.sortMode1, state.var1, state.var1Binner,
				    state.var2, state.var2Binner);
	case ChartType::DiscreteValue:
		return plotValueChart(dives, state.subtype, state.sortMode1,
				      state.var1, state.var1Binner, state.var2, state.var2Operation);
	case ChartType::DiscreteCount:
		return plotDiscreteCountChart(dives, state.subtype, state.sortMode1, state.var1, state.var1Binner);
	case ChartType::Pie:
		return plotPieChart(dives, state.sortMode1, state.var1, state.var1Binner);
	case ChartType::DiscreteBox:
		return plotDiscreteBoxChart(dives, state.var1, state.var1Binner, state.var2);
	case ChartType::DiscreteScatter:
		return plotDiscreteScatter(dives, state.var1, state.var1Binner, state.var2);
	case ChartType::HistogramCount:
		return plotHistogramCountChart(dives, state.subtype, state.var1, state.var1Binner);
	case ChartType::HistogramValue:
		return plotHistogramValueChart(dives, state.subtype, state.var1, state.var1Binner, state.var2,
					       state.var2Operation);
	case ChartType::HistogramStacked:
		return plotHistogramStackedChart(dives, state.subtype, state.var1, state.var1Binner,
						 state.var2, state.var2Binner);
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

void StatsView::updateFeatures()
{
	if (legend)
		legend->setVisible(state.legend);

	// For labels, we are brutal: simply show/hide the whole z-level with the labels
	if (rootNode)
		rootNode->zNodes[(int)ChartZValue::SeriesLabels]->setVisible(state.labels);

	if (meanMarker)
		meanMarker->setVisible(state.mean);

	if (medianMarker)
		medianMarker->setVisible(state.median);

	if (regressionItem) {
		regressionItem->setVisible(state.regression || state.confidence);
		if (state.regression || state.confidence)
			regressionItem->setFeatures(state.regression, state.confidence);
	}
	for (ChartItemPtr<QuartileMarker> &marker: quartileMarkers)
		marker->setVisible(state.quartiles);
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
// Dives for each bin on the independent variable, including the total counts for that bin.
struct BinDives {
	StatsBinPtr bin;
	std::vector<std::vector<dive *>> dives;
	int total;
};

// The problem with bar plots is that for different category
// bins, we might get different value bins. So we have to keep track
// of our counts and adjust accordingly. That's a bit annoying.
// Perhaps we should determine the bins of all dives first and then
// query the counts for precisely those bins?
struct BarPlotData {
	std::vector<BinDives> hbins;		// For each category bin the counts for all value bins
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
		hbins.push_back({ std::move(bin), std::vector<std::vector<dive *>>(vbins.size()), 0 });
		for (auto &[vbin, dives]: valueBinner.bin_dives(dives, false)) {
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
				for (auto &[bin, v, total]: hbins)
					v.insert(v.begin() + pos, std::vector<dive *>());
			}
			int count = (int)dives.size();
			hbins.back().dives[pos] = std::move(dives);
			hbins.back().total += count;
			if (count > maxCount)
				maxCount = count;
		}
		maxCategoryCount = std::max(maxCategoryCount, hbins.back().total);
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
		return { std::move(countString), std::move(percentageString) };
}

// From a list of dive bins, make (dives, label) pairs, where the label
// formats the total number and the percentage of dives.
static std::vector<BarSeries::MultiItem::Item> makeMultiItems(std::vector<std::vector<dive *>> bins, int total, bool isHorizontal)
{
	std::vector<BarSeries::MultiItem::Item> res;
	res.reserve(bins.size());
	for (std::vector<dive*> &bin: bins) {
		std::vector<QString> label = makePercentageLabels((int)bin.size(), total, isHorizontal);
		res.push_back({ std::move(bin), std::move(label) });
	}
	return res;
}

void StatsView::plotBarChart(const std::vector<dive *> &dives,
			     ChartSubType subType, ChartSortMode sortMode,
			     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			     const StatsVariable *valueVariable, const StatsBinner *valueBinner)
{
	if (!categoryBinner || !valueBinner)
		return;

	setTitle(valueVariable->nameWithBinnerUnit(*valueBinner));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	if (sortMode == ChartSortMode::Count) {
		// Note: we sort by count in reverse order, as this is probably what the user desires(?).
		std::sort(categoryBins.begin(), categoryBins.end(),
			  [](const StatsBinDives &b1, const StatsBinDives &b2)
			  { return b1.value.size() > b2.value.size(); });
	}

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
	legend = createChartItem<Legend>(data.vbinNames);

	std::vector<BarSeries::MultiItem> items;
	items.reserve(data.hbins.size());
	double pos = 0.0;
	for (auto &[hbin, dives, total]: data.hbins) {
		items.push_back({ pos - 0.5, pos + 0.5, makeMultiItems(std::move(dives), total, isHorizontal),
				  categoryBinner->formatWithUnit(*hbin) });
		pos += 1.0;
	}

	createSeries<BarSeries>(isHorizontal, isStacked, categoryVariable->name(), valueVariable, std::move(data.vbinNames), std::move(items));
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
			       ChartSubType subType, ChartSortMode sortMode,
			       const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			       const StatsVariable *valueVariable, StatsOperation valueAxisOperation)
{
	if (!categoryBinner)
		return;

	setTitle(QStringLiteral("%1 (%2)").arg(valueVariable->name(), StatsVariable::operationName(valueAxisOperation)));

	std::vector<StatsBinOp> categoryBins = valueVariable->bin_operations(*categoryBinner, dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	if (sortMode == ChartSortMode::Count) {
		// Note: we sort by count in reverse order, as this is probably what the user desires(?).
		std::sort(categoryBins.begin(), categoryBins.end(),
			  [](const StatsBinOp &b1, const StatsBinOp &b2)
			  { return b1.value.dives.size() > b2.value.dives.size(); });
	} else if (sortMode == ChartSortMode::Value) {
		std::sort(categoryBins.begin(), categoryBins.end(),
			  [valueAxisOperation](const StatsBinOp &b1, const StatsBinOp &b2)
			  { return b1.value.get(valueAxisOperation) < b2.value.get(valueAxisOperation); });
	}

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
			std::vector<QString> label = std::vector<QString> { std::move(value) };
			items.push_back({ pos - 0.5, pos + 0.5, height, label,
					  categoryBinner->formatWithUnit(*bin), res });
		}
		pos += 1.0;
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), valueVariable, std::move(items));
}

static int getTotalCount(const std::vector<StatsBinDives> &bins)
{
	int total = 0;
	for (const auto &[bin, dives]: bins)
		total += (int)dives.size();
	return total;
}

template<typename T>
static int getMaxCount(const std::vector<T> &bins)
{
	int res = 0;
	for (auto const &[dummy, dives]: bins)
		res = std::max(res, (int)(dives.size()));
	return res;
}

void StatsView::plotDiscreteCountChart(const std::vector<dive *> &dives,
				      ChartSubType subType, ChartSortMode sortMode,
				      const StatsVariable *categoryVariable, const StatsBinner *categoryBinner)
{
	if (!categoryBinner)
		return;

	setTitle(categoryVariable->nameWithBinnerUnit(*categoryBinner));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	if (sortMode == ChartSortMode::Count) {
		// Note: we sort by count in reverse order, as this is probably what the user desires(?).
		std::sort(categoryBins.begin(), categoryBins.end(),
			  [](const StatsBinDives &b1, const StatsBinDives &b2)
			  { return b1.value.size() > b2.value.size(); });
	}

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
	for (auto const &[bin, dives]: categoryBins) {
		std::vector<QString> label = makePercentageLabels((int)dives.size(), total, isHorizontal);
		items.push_back({ pos - 0.5, pos + 0.5, std::move(dives), label,
				  categoryBinner->formatWithUnit(*bin), total });
		pos += 1.0;
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), std::move(items));
}

void StatsView::plotPieChart(const std::vector<dive *> &dives, ChartSortMode sortMode,
			     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner)
{
	if (!categoryBinner)
		return;

	setTitle(categoryVariable->nameWithBinnerUnit(*categoryBinner));

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, false);

	// If there is nothing to display, quit
	if (categoryBins.empty())
		return;

	std::vector<std::pair<QString, std::vector<dive *>>> data;
	data.reserve(categoryBins.size());
	for (auto &[bin, dives]: categoryBins)
		data.emplace_back(categoryBinner->formatWithUnit(*bin), std::move(dives));

	PieSeries *series = createSeries<PieSeries>(categoryVariable->name(), std::move(data), sortMode);

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
				    const StatsVariable *valueVariable)
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
		StatsQuartiles quartiles = StatsVariable::quartiles(array);
		if (quartiles.isValid()) {
			quartileMarkers.push_back(createChartItem<QuartileMarker>(
					x, quartiles.q1, catAxis, valAxis));
			quartileMarkers.push_back(createChartItem<QuartileMarker>(
					x, quartiles.q2, catAxis, valAxis));
			quartileMarkers.push_back(createChartItem<QuartileMarker>(
					x, quartiles.q3, catAxis, valAxis));
		}
		x += 1.0;
	}
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
		labels.push_back({ std::move(label), lowerBound, prefer });
	}

	const StatsBin &lastBin = *bins.back().bin;
	QString lastLabel = binner.formatUpperBound(lastBin);
	double upperBound = binner.upperBoundToFloat(lastBin);
	labels.push_back({ lastLabel, upperBound, false });

	return createAxis<HistogramAxis>(name, std::move(labels), isHorizontal);
}

void StatsView::plotHistogramCountChart(const std::vector<dive *> &dives,
					ChartSubType subType,
					const StatsVariable *categoryVariable, const StatsBinner *categoryBinner)
{
	if (!categoryBinner)
		return;

	setTitle(categoryVariable->name());

	std::vector<StatsBinDives> categoryBins = categoryBinner->bin_dives(dives, true);

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

	// Attention: this moves away the dives
	for (auto &[bin, dives]: categoryBins) {
		double lowerBound = categoryBinner->lowerBoundToFloat(*bin);
		double upperBound = categoryBinner->upperBoundToFloat(*bin);
		std::vector<QString> label = makePercentageLabels((int)dives.size(), total, isHorizontal);

		items.push_back({ lowerBound, upperBound, std::move(dives), std::move(label),
				  categoryBinner->formatWithUnit(*bin), total });
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), std::move(items));

	if (categoryVariable->type() == StatsVariable::Type::Numeric) {
		double mean = categoryVariable->mean(dives);
		if (!std::isnan(mean))
			meanMarker = createChartItem<HistogramMarker>(mean, isHorizontal, currentTheme->meanMarkerColor, xAxis, yAxis);
		double median = categoryVariable->quartiles(dives).q2;
		if (!std::isnan(median))
			medianMarker = createChartItem<HistogramMarker>(median, isHorizontal, currentTheme->medianMarkerColor, xAxis, yAxis);
	}
}

void StatsView::plotHistogramValueChart(const std::vector<dive *> &dives,
					ChartSubType subType,
					const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
					const StatsVariable *valueVariable, StatsOperation valueAxisOperation)
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
		std::vector<QString> label = std::vector<QString> { value };
		items.push_back({ lowerBound, upperBound, height, label,
				  categoryBinner->formatWithUnit(*bin), res });
	}

	createSeries<BarSeries>(isHorizontal, categoryVariable->name(), valueVariable, std::move(items));
}

void StatsView::plotHistogramStackedChart(const std::vector<dive *> &dives,
					  ChartSubType subType,
					  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
					  const StatsVariable *valueVariable, const StatsBinner *valueBinner)
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
	legend = createChartItem<Legend>(data.vbinNames);

	CountAxis *valAxis = createCountAxis(data.maxCategoryCount, isHorizontal);

	if (isHorizontal)
		setAxes(valAxis, catAxis);
	else
		setAxes(catAxis, valAxis);

	std::vector<BarSeries::MultiItem> items;
	items.reserve(data.hbins.size());

	for (auto &[hbin, dives, total]: data.hbins) {
		double lowerBound = categoryBinner->lowerBoundToFloat(*hbin);
		double upperBound = categoryBinner->upperBoundToFloat(*hbin);
		items.push_back({ lowerBound, upperBound, makeMultiItems(std::move(dives), total, isHorizontal),
				  categoryBinner->formatWithUnit(*hbin) });
	}

	createSeries<BarSeries>(isHorizontal, true, categoryVariable->name(), valueVariable, std::move(data.vbinNames), std::move(items));
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
		regressionItem = createChartItem<RegressionItem>(reg, xAxis, yAxis);
}
