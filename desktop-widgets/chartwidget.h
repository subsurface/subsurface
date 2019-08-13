#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QVBarModelMapper>
#include <QHBarModelMapper>
#include "ui_chartwidget.h"
#include "qt-models/buddystatsmodel.h"
#include "qt-models/datestatsmodel.h"
#include "qt-models/tagstatsmodel.h"
#include "core/subsurface-qt/DiveListNotifier.h"

class ChartWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ChartWidget(QWidget *parent = nullptr);
	~ChartWidget();
	QtCharts::QChart *drawYearChart();
	QtCharts::QChart *drawMonthChart();
	QtCharts::QChart *drawTagChart();
	QtCharts::QChart *drawBuddyChart();

public
slots:
	void filterFinished();

private:
	int calcYAxisTicks(int maxDiveCount);
	int calcYAxisMax(int maxDiveCount);
	Ui::ChartWidget ui;
	DateStatsTableModel *model;
	TagStatsListModel *tagModel;
	BuddyStatsListModel *buddyModel;
	QtCharts::QChartView monthChartView;
	QtCharts::QChartView yearChartView;
	QtCharts::QChartView tagChartView;
	QtCharts::QChartView buddyChartView;
	QtCharts::QHBarModelMapper yearMapper;
	QtCharts::QVBarModelMapper monthMapper;
	QtCharts::QVBarModelMapper tagMapper;
	QtCharts::QVBarModelMapper buddyMapper;
};

#endif // STATSCHARTWIDGET_H
