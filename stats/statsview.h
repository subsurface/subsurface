// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include "statsstate.h"
#include "statshelper.h"
#include "statsselection.h"
#include <memory>
#include <QImage>
#include <QPainter>
#include <QQuickItem>

struct dive;
struct StatsBinner;
struct StatsBin;
struct StatsState;
struct StatsVariable;

class StatsSeries;
class CategoryAxis;
class ChartItem;
class ChartRectLineItem;
class ChartTextItem;
class CountAxis;
class HistogramAxis;
class HistogramMarker;
class QuartileMarker;
class RegressionItem;
class StatsAxis;
class StatsGrid;
class StatsTheme;
class Legend;
class QSGTexture;
class RootNode;	// Internal implementation detail

enum class ChartSubType : int;
enum class ChartZValue : int;
enum class StatsOperation : int;
enum class ChartSortMode : int;

class StatsView : public QQuickItem {
	Q_OBJECT
public:
	StatsView();
	StatsView(QQuickItem *parent);
	~StatsView();

	void plot(const StatsState &state);
	void updateFeatures(const StatsState &state);	// Updates the visibility of chart features, such as legend, regression, etc.
	void restrictToSelection();
	void unrestrict();
	int restrictionCount() const;			// <0: no restriction
	QQuickWindow *w() const;			// Make window available to items
	QSizeF size() const;
	QRectF plotArea() const;
	void setBackgroundColor(QColor color);		// Chart must be replot for color to become effective.
	void setTheme(bool dark);			// Chart must be replot for theme to become effective.
	const StatsTheme &getCurrentTheme() const;
	void addQSGNode(QSGNode *node, ChartZValue z);	// Must only be called in render thread!
	void registerChartItem(ChartItem &item);
	void registerDirtyChartItem(ChartItem &item);
	void emergencyShutdown();			// Called when QQuick decides to delete our root node.

	// Create a chart item and add it to the scene.
	// The item must not be deleted by the caller, but can be
	// scheduled for deletion using deleteChartItem() below.
	// Most items can be made invisible, which is preferred over deletion.
	// All items on the scene will be deleted once the chart is reset.
	template <typename T, class... Args>
	ChartItemPtr<T> createChartItem(Args&&... args);

	template <typename T>
	void deleteChartItem(ChartItemPtr<T> &item);
private slots:
	void replotIfVisible();
	void divesSelected(const QVector<dive *> &dives);
private:
	// QtQuick related things
	bool backgroundDirty;
	QRectF plotRect;
	QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif
	void plotAreaChanged(const QSizeF &size);
	void reset(); // clears all series and axes
	void setAxes(StatsAxis *x, StatsAxis *y);
	void plotBarChart(const std::vector<dive *> &dives,
			  ChartSubType subType, ChartSortMode sortMode,
			  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			  const StatsVariable *valueVariable, const StatsBinner *valueBinner);
	void plotValueChart(const std::vector<dive *> &dives,
			    ChartSubType subType, ChartSortMode sortMode,
			    const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			    const StatsVariable *valueVariable, StatsOperation valueAxisOperation);
	void plotDiscreteCountChart(const std::vector<dive *> &dives,
				    ChartSubType subType, ChartSortMode sortMode,
				    const StatsVariable *categoryVariable, const StatsBinner *categoryBinner);
	void plotPieChart(const std::vector<dive *> &dives, ChartSortMode sortMode,
			  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner);
	void plotDiscreteBoxChart(const std::vector<dive *> &dives,
				  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, const StatsVariable *valueVariable);
	void plotDiscreteScatter(const std::vector<dive *> &dives,
				 const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				 const StatsVariable *valueVariable);
	void plotHistogramCountChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner);
	void plotHistogramValueChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				     const StatsVariable *valueVariable, StatsOperation valueAxisOperation);
	void plotHistogramStackedChart(const std::vector<dive *> &dives,
				       ChartSubType subType,
				       const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				       const StatsVariable *valueVariable, const StatsBinner *valueBinner);
	void plotHistogramBoxChart(const std::vector<dive *> &dives,
				   const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, const StatsVariable *valueVariable);
	void plotScatter(const std::vector<dive *> &dives, const StatsVariable *categoryVariable, const StatsVariable *valueVariable);
	void setTitle(const QString &);
	void updateTitlePos(); // After resizing, set title to correct position
	void plotChart();
	void updateFeatures(); // Updates the visibility of chart features, such as legend, regression, etc.

	template <typename T, class... Args>
	T *createSeries(Args&&... args);

	template <typename T, class... Args>
	T *createAxis(const QString &title, Args&&... args);

	template<typename T>
	CategoryAxis *createCategoryAxis(const QString &title, const StatsBinner &binner,
					 const std::vector<T> &bins, bool isHorizontal);
	template<typename T>
	HistogramAxis *createHistogramAxis(const QString &title, const StatsBinner &binner,
					   const std::vector<T> &bins, bool isHorizontal);
	CountAxis *createCountAxis(int maxVal, bool isHorizontal);

	// Helper functions to add feature to the chart
	void addLineMarker(double pos, double low, double high, const QPen &pen, bool isHorizontal);

	StatsState state;
	const StatsTheme *currentTheme;
	QColor backgroundColor;
	std::vector<std::unique_ptr<StatsSeries>> series;
	std::unique_ptr<StatsGrid> grid;
	std::vector<ChartItemPtr<QuartileMarker>> quartileMarkers;
	ChartItemPtr<HistogramMarker> medianMarker, meanMarker;
	StatsSeries *highlightedSeries;
	StatsAxis *xAxis, *yAxis;
	ChartItemPtr<ChartTextItem> title;
	ChartItemPtr<Legend> legend;
	Legend *draggedItem;
	ChartItemPtr<RegressionItem> regressionItem;
	ChartItemPtr<ChartRectLineItem> selectionRect;
	QPointF dragStartMouse, dragStartItem;
	SelectionModifier selectionModifier;
	std::vector<dive *> oldSelection;
	bool restrictDives;
	std::vector<dive *> restrictedDives;	// sorted by pointer for quick lookup.

	void hoverEnterEvent(QHoverEvent *event) override;
	void hoverMoveEvent(QHoverEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	RootNode *rootNode;

	// There are three double linked lists of chart items:
	// clean items, dirty items and items to be deleted.
	// Note that only the render thread must delete chart items,
	// and therefore these lists are the only owning pointers
	// to chart items. All other pointers are non-owning and
	// can therefore become stale.
	struct ChartItemList {
		ChartItemList();
		ChartItem *first, *last;
		void append(ChartItem &item);
		void remove(ChartItem &item);
		void clear();
		void splice(ChartItemList &list);
	};
	ChartItemList cleanItems, dirtyItems, deletedItems;
	void deleteChartItemInternal(ChartItem &item);
	void freeDeletedChartItems();
};

// This implementation detail must be known to users of the class.
// Perhaps move it into a statsview_impl.h file.
template <typename T, class... Args>
ChartItemPtr<T> StatsView::createChartItem(Args&&... args)
{
	return ChartItemPtr(new T(*this, std::forward<Args>(args)...));
}

template <typename T>
void StatsView::deleteChartItem(ChartItemPtr<T> &item)
{
	deleteChartItemInternal(*item);
	item.reset();
}

#endif
