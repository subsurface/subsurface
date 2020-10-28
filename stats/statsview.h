// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include <memory>
#include <QQuickWidget>

namespace QtCharts {
	class QAbstractAxis;
	class QAbstractSeries;
}

class StatsView : public QQuickWidget {
	Q_OBJECT
public:
	StatsView(QWidget *parent = NULL);
	~StatsView();
	void plot();
private:
	void reset(); // clears all series and axes

	template <typename T>
	T *makeAxis();

	template <typename T>
	T *addSeries(const QString &name, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis);
	QtCharts::QAbstractSeries *addSeriesHelper(const QString &name, int type, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis);

	std::vector<std::unique_ptr<QtCharts::QAbstractAxis>> axes;
};

#endif
