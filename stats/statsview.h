// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include "statsstate.h"
#include <memory>
#include <QFont>
#include <QGraphicsScene>
#include <QImage>
#include <QPainter>
#include <QQuickItem>
#include <QGraphicsPolygonItem>

struct dive;
struct StatsBinner;
struct StatsBin;
struct StatsState;
struct StatsVariable;

class QGraphicsLineItem;
class QGraphicsSimpleTextItem;
class StatsSeries;
class CategoryAxis;
class ChartItem;
class CountAxis;
class HistogramAxis;
class HistogramMarker;
class QuartileMarker;
class StatsAxis;
class StatsGrid;
class Legend;
class QSGTexture;
class RootNode;	// Internal implementation detail

enum class ChartSubType : int;
enum class ChartZValue : int;
enum class StatsOperation : int;

struct regression_data {
	double a,b;
	double res2, r2, sx2, xavg;
	int n;
};


class StatsView : public QQuickItem {
	Q_OBJECT
public:
	StatsView();
	StatsView(QQuickItem *parent);
	~StatsView();

	void plot(const StatsState &state);
	QQuickWindow *w() const;		// Make window available to items
	QSizeF size() const;
	void addQSGNode(QSGNode *node, ChartZValue z);	// Must only be called in render thread!
	void registerDirtyChartItem(ChartItem &item);
	void unregisterDirtyChartItem(ChartItem &item);
	template <typename T, class... Args>
	std::unique_ptr<T> createChartItem(Args&&... args);

private slots:
	void replotIfVisible();
private:
	// QtQuick related things
	QRectF plotRect;
	QSGNode *updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData) override;
	std::unique_ptr<QImage> img;
	std::unique_ptr<QPainter> painter;
	QGraphicsScene scene;
	std::unique_ptr<QSGTexture> texture;

	void plotAreaChanged(const QSizeF &size);
	void reset(); // clears all series and axes
	void setAxes(StatsAxis *x, StatsAxis *y);
	void plotBarChart(const std::vector<dive *> &dives,
			  ChartSubType subType,
			  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			  const StatsVariable *valueVariable, const StatsBinner *valueBinner, bool labels, bool legend);
	void plotValueChart(const std::vector<dive *> &dives,
			    ChartSubType subType,
			    const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			    const StatsVariable *valueVariable, StatsOperation valueAxisOperation, bool labels);
	void plotDiscreteCountChart(const std::vector<dive *> &dives,
				    ChartSubType subType,
				    const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, bool labels);
	void plotPieChart(const std::vector<dive *> &dives,
			  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, bool labels, bool legend);
	void plotDiscreteBoxChart(const std::vector<dive *> &dives,
				  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, const StatsVariable *valueVariable);
	void plotDiscreteScatter(const std::vector<dive *> &dives,
				 const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				 const StatsVariable *valueVariable, bool quartiles);
	void plotHistogramCountChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				     bool labels, bool showMedian, bool showMean);
	void plotHistogramValueChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				     const StatsVariable *valueVariable, StatsOperation valueAxisOperation, bool labels);
	void plotHistogramStackedChart(const std::vector<dive *> &dives,
				       ChartSubType subType,
				       const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				       const StatsVariable *valueVariable, const StatsBinner *valueBinner, bool labels, bool legend);
	void plotHistogramBoxChart(const std::vector<dive *> &dives,
				   const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, const StatsVariable *valueVariable);
	void plotScatter(const std::vector<dive *> &dives, const StatsVariable *categoryVariable, const StatsVariable *valueVariable);
	void setTitle(const QString &);
	void updateTitlePos(); // After resizing, set title to correct position
	void plotChart();

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

	// A regression line
	struct RegressionLine {
		std::unique_ptr<QGraphicsPolygonItem> item;
		std::unique_ptr<QGraphicsPolygonItem> central;
		StatsAxis *xAxis, *yAxis;
		const struct regression_data reg;
		void updatePosition();
		RegressionLine(const struct regression_data reg, QBrush brush, QGraphicsScene *scene, StatsAxis *xAxis, StatsAxis *yAxis);
	};

	void addLinearRegression(const struct regression_data reg, StatsAxis *xAxis, StatsAxis *yAxis);
	void addHistogramMarker(double pos, QColor color, bool isHorizontal, StatsAxis *xAxis, StatsAxis *yAxis);

	StatsState state;
	QFont titleFont;
	std::vector<std::unique_ptr<StatsAxis>> axes;
	std::unique_ptr<StatsGrid> grid;
	std::vector<std::unique_ptr<StatsSeries>> series;
	std::unique_ptr<Legend> legend;
	std::vector<std::unique_ptr<QuartileMarker>> quartileMarkers;
	std::vector<RegressionLine> regressionLines;
	std::vector<std::unique_ptr<HistogramMarker>> histogramMarkers;
	std::unique_ptr<QGraphicsSimpleTextItem> title;
	StatsSeries *highlightedSeries;
	StatsAxis *xAxis, *yAxis;
	Legend *draggedItem;
	QPointF dragStartMouse, dragStartItem;

	void hoverEnterEvent(QHoverEvent *event) override;
	void hoverMoveEvent(QHoverEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	RootNode *rootNode;
	ChartItem *firstDirtyChartItem, *lastDirtyChartItem;
};

// This implementation detail must be known to users of the class.
// Perhaps move it into a statsview_impl.h file.
template <typename T, class... Args>
std::unique_ptr<T> StatsView::createChartItem(Args&&... args)
{
	std::unique_ptr<T> res(new T(*this, std::forward<Args>(args)...));
	return res;
}

#endif
