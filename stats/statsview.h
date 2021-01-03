// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include "statsstate.h"
#include <memory>
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
class StatsSeries;
class CategoryAxis;
class CountAxis;
class HistogramAxis;
class StatsAxis;
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
	void addAxes(StatsAxis *x, StatsAxis *y); // Add new x- and y-axis
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
		QtCharts::QAbstractSeries *series; // In case we ever support charts with multiple axes
		double pos, value;
		QuartileMarker(double pos, double value, QtCharts::QAbstractSeries *series);
		void updatePosition();
	};

	// A general line marker
	struct LineMarker {
		std::unique_ptr<QGraphicsLineItem> item;
		QtCharts::QAbstractSeries *series; // In case we ever support charts with multiple axes
		QPointF from, to; // In local coordinates
		void updatePosition();
		LineMarker(QPointF from, QPointF to, QPen pen, QtCharts::QAbstractSeries *series);
	};

	void addLinearRegression(double a, double b, double minX, double maxX, double minY, double maxY, QtCharts::QAbstractSeries *series);
	void addHistogramMarker(double pos, double low, double high, const QPen &pen, bool isHorizontal, QtCharts::QAbstractSeries *series);

	StatsState state;
	QtCharts::QChart *chart;
	std::vector<std::unique_ptr<StatsAxis>> axes;
	std::vector<std::unique_ptr<StatsSeries>> series;
	std::unique_ptr<Legend> legend;
	std::vector<QuartileMarker> quartileMarkers;
	std::vector<LineMarker> lineMarkers;
	StatsSeries *highlightedSeries;

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
