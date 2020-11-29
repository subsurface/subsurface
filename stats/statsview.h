// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include <memory>
#include <QQuickWidget>

struct dive;
struct StatsType;
struct StatsBinner;
struct StatsBin;
struct StatsState;

namespace QtCharts {
	class QAbstractAxis;
	class QAbstractSeries;
	class QBarCategoryAxis;
	class QCategoryAxis;
	class QChart;
	class QLegend;
	class QValueAxis;
}
class QGraphicsLineItem;
class QGraphicsPixmapItem;
class QGraphicsSimpleTextItem;
class ScatterSeries;

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
	void plotScatter(const std::vector<dive *> &dives, const StatsType *categoryType, const StatsType *valueType);
	void setTitle(const QString &);
	void showLegend();
	void hideLegend();

	template <typename T>
	T *makeAxis();

	template <typename T>
	T *addSeries(const QString &name);
	ScatterSeries *addScatterSeries(const QString &name, const StatsType &typeX, const StatsType &typeY);
	void initSeries(QtCharts::QAbstractSeries *series, const QString &name);

	template<typename T>
	QtCharts::QBarCategoryAxis *createCategoryAxis(const StatsBinner &binner, const std::vector<T> &bins);
	template<typename T>
	QtCharts::QCategoryAxis *createHistogramAxis(const StatsBinner &binner,
						     const std::vector<T> &bins,
						     bool isHorizontal);
	QtCharts::QValueAxis *createValueAxis(double min, double max, int decimals, bool isHorizontal);
	QtCharts::QValueAxis *createCountAxis(int count, bool isHorizontal);

	// Helper functions to add feature to the chart
	void addLineMarker(double pos, double low, double high, const QPen &pen, bool isHorizontal);
	void addBar(double from, double to, double height, bool isHorizontal,
		    const std::vector<QString> &label);

	// A label that is composed of multiple lines
	struct BarLabel {
		std::vector<std::unique_ptr<QGraphicsSimpleTextItem>> items;
		double value, height; // Position and size of bar in graph
		double totalWidth, totalHeight; // Size of the item
		bool isHorizontal;
		QtCharts::QAbstractSeries *series; // In case we ever support charts with multiple axes
		BarLabel(const std::vector<QString> &labels, double value, double height, bool isHorizontal, QtCharts::QAbstractSeries *series);
		void updatePosition();
	};

	// A short line used to mark quartiles
	struct QuartileMarker {
		std::unique_ptr<QGraphicsLineItem> item;
		QtCharts::QAbstractSeries *series; // In case we ever support charts with multiple axes
		double pos, value;
		QuartileMarker(double pos, double value, QtCharts::QAbstractSeries *series);
		void updatePosition();
	};

	QtCharts::QChart *chart;
	std::vector<std::unique_ptr<QtCharts::QAbstractAxis>> axes;
	std::vector<BarLabel> barLabels;
	std::vector<std::unique_ptr<ScatterSeries>> scatterSeries;
	std::vector<QuartileMarker> quartileMarkers;
	ScatterSeries *highlightedScatterSeries;	// scatter series with highlighted element

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
