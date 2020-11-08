// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include <memory>
#include <QQuickWidget>

struct dive;
struct StatsType;
struct StatsBinner;
struct StatsBin;

namespace QtCharts {
	class QAbstractAxis;
	class QAbstractSeries;
	class QBarCategoryAxis;
	class QCategoryAxis;
	class QLegend;
	class QValueAxis;
}

enum class ChartSubType : int;
enum class StatsOperation : int;

class StatsView : public QQuickWidget {
	Q_OBJECT
public:
	StatsView(QWidget *parent = NULL);
	~StatsView();

	// Integers are indexes of the combobox entries retrieved with the functions below
	void plot(int type, int subType,
		  int firstAxis, int firstAxisBin, int firstAxisOperation,
		  int secondAxis, int secondAxisBin, int secondAxisOperation);

	// Functions to construct the UI comboboxes
	static QStringList getChartTypes();
	static QStringList getChartSubTypes(int chartType);
	static QStringList getFirstAxisTypes(int chartType);
	static QStringList getFirstAxisBins(int chartType, int firstAxis);
	static QStringList getFirstAxisOperations(int chartType, int secondAxis);
	static QStringList getSecondAxisTypes(int chartType, int firstAxis);
	static QStringList getSecondAxisBins(int chartType, int firstAxis, int secondAxis);
	static QStringList getSecondAxisOperations(int chartType, int firstAxis, int secondAxis);
private:
	void reset(); // clears all series and axes
	void plotBarChart(const std::vector<dive *> &dives,
			  ChartSubType subType,
			  const StatsType *categoryType, const StatsBinner *categoryBinner,
			  const StatsType *valueType, const StatsBinner *valueBinner);
	void plotValueChart(const std::vector<dive *> &dives,
			    ChartSubType subType,
			    const StatsType *categoryType, const StatsBinner *categoryBinner,
			    const StatsType *valueType, StatsOperation valueAxisOperation);
	void plotDiscreteCountChart(const std::vector<dive *> &dives,
				    ChartSubType subType,
				    const StatsType *categoryType, const StatsBinner *categoryBinner);
	void plotDiscreteScatter(const std::vector<dive *> &dives,
				 const StatsType *categoryType, const StatsBinner *categoryBinner,
				 const StatsType *valueType);
	void plotHistogramCountChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsType *categoryType, const StatsBinner *categoryBinner);
	void plotHistogramBarChart(const std::vector<dive *> &dives,
				   ChartSubType subType,
				   const StatsType *categoryType, const StatsBinner *categoryBinner,
				   const StatsType *valueType, StatsOperation valueAxisOperation);
	void plotScatter(const std::vector<dive *> &dives, const StatsType *categoryType, const StatsType *valueType);
	void setTitle(const QString &);
	QtCharts::QLegend *getLegend();
	void showLegend();
	void hideLegend();

	template <typename T>
	T *makeAxis();

	template <typename T>
	T *addSeries(const QString &name, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis);
	QtCharts::QAbstractSeries *addSeriesHelper(const QString &name, int type, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis);

	template<typename T>
	QtCharts::QBarCategoryAxis *createCategoryAxis(const StatsBinner &binner, const std::vector<T> &bins);
	template<typename T>
	QtCharts::QCategoryAxis *createHistogramAxis(const StatsBinner &binner,
						     const std::vector<T> &bins,
						     bool isHorizontal);
	QtCharts::QValueAxis *createValueAxis(double min, double max, int decimals, bool isHorizontal);

	// Helper functions to add feature to the chart
	void addLineMarker(double pos, double low, double high, const QPen &pen,
			   QtCharts::QAbstractAxis *axisX, QtCharts::QAbstractAxis *axisY,
			   bool isHorizontal);
	void addBar(double from, double to, double height, const QBrush &brush, const QPen &pen,
		    QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis, bool isHorizontal);

	std::vector<std::unique_ptr<QtCharts::QAbstractAxis>> axes;
};

#endif
