// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include "statsstate.h"
#include <memory>
#include <QFont>
#include <QQuickWidget>

struct dive;
struct StatsBinner;
struct StatsBin;
struct StatsState;
struct StatsVariable;

namespace QtCharts {
	class QAbstractSeries;
	class QChart;
}
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;
class StatsSeries;
class CategoryAxis;
class CountAxis;
class HistogramAxis;
class StatsAxis;
class StatsGrid;
class Legend;

enum class ChartSubType : int;
enum class StatsOperation : int;

class StatsView : public QQuickWidget {
	Q_OBJECT
public:
	StatsView(QWidget *parent = NULL);
	~StatsView();

	void plot(const StatsState &state);
private slots:
	void plotAreaChanged(const QRectF &plotArea);
	void replotIfVisible();
private:
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

	// A short line used to mark quartiles
	struct QuartileMarker {
		std::unique_ptr<QGraphicsLineItem> item;
		StatsAxis *xAxis, *yAxis;
		double pos, value;
		QuartileMarker(double pos, double value, QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis);
		void updatePosition();
	};

	// A general line marker
	struct LineMarker {
		std::unique_ptr<QGraphicsLineItem> item;
		StatsAxis *xAxis, *yAxis;
		QPointF from, to; // In local coordinates
		void updatePosition();
		LineMarker(QPointF from, QPointF to, QPen pen, QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis);
	};

	void addLinearRegression(double a, double b, double minX, double maxX, double minY, double maxY, StatsAxis *xAxis, StatsAxis *yAxis);
	void addHistogramMarker(double pos, double low, double high, const QPen &pen, bool isHorizontal, StatsAxis *xAxis, StatsAxis *yAxis);

	StatsState state;
	QtCharts::QChart *chart;
	QFont titleFont;
	std::vector<std::unique_ptr<StatsAxis>> axes;
	std::unique_ptr<StatsGrid> grid;
	std::vector<std::unique_ptr<StatsSeries>> series;
	std::unique_ptr<Legend> legend;
	std::vector<QuartileMarker> quartileMarkers;
	std::vector<LineMarker> lineMarkers;
	std::unique_ptr<QGraphicsSimpleTextItem> title;
	StatsSeries *highlightedSeries;
	StatsAxis *xAxis, *yAxis;

	// This is unfortunate: we can't derive from QChart, because the chart is allocated by QML.
	// Therefore, we have to listen to hover events using an events-filter.
	// Probably we should try to get rid of the QML ChartView.
	struct EventFilter : public QObject {
		StatsView *view;
		EventFilter(StatsView *view) : view(view) {}
	private:
		bool eventFilter(QObject *o, QEvent *event);
	} eventFilter;
	friend EventFilter;
	void hover(QPointF pos);
};

#endif
