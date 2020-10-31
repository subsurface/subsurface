// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include <memory>
#include <QQuickWidget>

struct dive;
struct StatsType;
struct StatsBinner;

namespace QtCharts {
	class QAbstractAxis;
	class QAbstractSeries;
}

enum class ChartSubType : int;

class StatsView : public QQuickWidget {
	Q_OBJECT
public:
	StatsView(QWidget *parent = NULL);
	~StatsView();

	// Integers are indexes of the combobox entries retrieved with the functions below
	void plot(int type, int subType, int firstAxis, int firstAxisBin, int secondAxis, int secondAxisBin);

	// Functions to construct the UI comboboxes
	static QStringList getChartTypes();
	static QStringList getChartSubTypes(int chartType);
	static QStringList getFirstAxisTypes(int chartType);
	static QStringList getFirstAxisBins(int chartType, int firstAxis);
	static QStringList getSecondAxisTypes(int chartType, int firstAxis);
	static QStringList getSecondAxisBins(int chartType, int firstAxis, int secondAxis);
private:
	void reset(); // clears all series and axes
	void plotBarChart(const std::vector<dive *> &dives,
			  ChartSubType subType,
			  const StatsType *categoryType, const StatsBinner *categoryBinner,
			  const StatsType *valueType, const StatsBinner *valueBinner);
	void setTitle(const QString &);
	void showLegend();

	template <typename T>
	T *makeAxis();

	template <typename T>
	T *addSeries(const QString &name, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis);
	QtCharts::QAbstractSeries *addSeriesHelper(const QString &name, int type, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis);

	std::vector<std::unique_ptr<QtCharts::QAbstractAxis>> axes;
};

#endif
