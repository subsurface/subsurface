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
struct StatsType;

namespace QtCharts {
	class QAbstractAxis;
	class QAbstractSeries;
	class QBarCategoryAxis;
	class QChart;
}
class QGraphicsLineItem;
class BarSeries;
class BoxSeries;
class ScatterSeries;
class StatsAxis;

enum class ChartSubType : int;
enum class StatsOperation : int;

class StatsView : public QQuickWidget {
	Q_OBJECT
public:
	StatsView(QWidget *parent = NULL);
	~StatsView();

	// Integers are indexes of the combobox entries retrieved with the functions below
	void plot(const StatsState &state);
private slots:
	void plotAreaChanged(const QRectF &plotArea);
	void replotIfVisible();
private:
	void reset(); // clears all series and axes
	void addAxes(QtCharts::QAbstractAxis *x, QtCharts::QAbstractAxis *y); // Add new x- and y-axis
	void plotBarChart(const std::vector<dive *> &dives,
			  ChartSubType subType,
			  const StatsType *categoryType, const StatsBinner *categoryBinner,
			  const StatsType *valueType, const StatsBinner *valueBinner);
	void plotValueChart(const std::vector<dive *> &dives,
			    ChartSubType subType,
			    const StatsType *categoryType, const StatsBinner *categoryBinner,
			    const StatsType *valueType, StatsOperation valueAxisOperation, bool labels);
	void plotDiscreteCountChart(const std::vector<dive *> &dives,
				    ChartSubType subType,
				    const StatsType *categoryType, const StatsBinner *categoryBinner, bool labels);
	void plotPieChart(const std::vector<dive *> &dives,
			  const StatsType *categoryType, const StatsBinner *categoryBinner);
	void plotDiscreteBoxChart(const std::vector<dive *> &dives,
				  const StatsType *categoryType, const StatsBinner *categoryBinner, const StatsType *valueType);
	void plotDiscreteScatter(const std::vector<dive *> &dives,
				 const StatsType *categoryType, const StatsBinner *categoryBinner,
				 const StatsType *valueType);
	void plotHistogramCountChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsType *categoryType, const StatsBinner *categoryBinner,
				     bool labels, bool showMedian, bool showMean);
	void plotHistogramBarChart(const std::vector<dive *> &dives,
				   ChartSubType subType,
				   const StatsType *categoryType, const StatsBinner *categoryBinner,
				   const StatsType *valueType, StatsOperation valueAxisOperation, bool labels);
	void plotHistogramBoxChart(const std::vector<dive *> &dives,
				   const StatsType *categoryType, const StatsBinner *categoryBinner, const StatsType *valueType);
	void plotScatter(const std::vector<dive *> &dives, const StatsType *categoryType, const StatsType *valueType);
	void setTitle(const QString &);
	void showLegend();
	void hideLegend();

	template <typename T, class... Args>
	T *createAxis(Args&&... args);

	template <typename T>
	T *addSeries(const QString &name);
	ScatterSeries *addScatterSeries(const QString &name, const StatsType &typeX, const StatsType &typeY);
	BarSeries *addBarSeries(const QString &name, bool horizontal, const QString &categoryName,
				const StatsType *valueType);
	BoxSeries *addBoxSeries(const QString &name, const QString &unit, int decimals);
	void initSeries(QtCharts::QAbstractSeries *series, const QString &name);

	template<typename T>
	QtCharts::QBarCategoryAxis *createCategoryAxis(const StatsBinner &binner, const std::vector<T> &bins, bool isHorizontal);
	template<typename T>
	QtCharts::QAbstractAxis *createHistogramAxis(const StatsBinner &binner,
						     const std::vector<T> &bins,
						     bool isHorizontal);

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

	StatsState state;
	QtCharts::QChart *chart;
	std::vector<std::unique_ptr<StatsAxis>> axes;
	std::vector<std::unique_ptr<ScatterSeries>> scatterSeries;
	std::vector<std::unique_ptr<BarSeries>> barSeries;
	std::vector<std::unique_ptr<BoxSeries>> boxSeries;
	std::vector<QuartileMarker> quartileMarkers;
	ScatterSeries *highlightedScatterSeries;	// scatter series with highlighted element
	BarSeries *highlightedBarSeries;		// bar series with highlighted element
	BoxSeries *highlightedBoxSeries;		// box series with highlighted element

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
	// Generic code to handle the highlighting of a series element
	template<typename Series>
	void handleHover(const std::vector<std::unique_ptr<Series>> &series, Series *&highlighted, QPointF pos);
};

#endif
