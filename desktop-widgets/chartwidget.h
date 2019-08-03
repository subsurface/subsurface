#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QChart>
#include <QChartView>
#include <QVBarModelMapper>
#include <QHBarModelMapper>
#include "ui_chartwidget.h"
#include "qt-models/datestatsmodel.h"
#include "core/subsurface-qt/DiveListNotifier.h"

class ChartWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ChartWidget(QWidget *parent = nullptr);
	~ChartWidget();
	QtCharts::QChart *drawYearChart();
	QtCharts::QChart *drawMonthChart();

public
slots:
	void filterFinished();

private:
	Ui::ChartWidget ui;
	DateStatsTableModel *model;
	QtCharts::QChartView monthChartView;
	QtCharts::QChartView yearChartView;
	QtCharts::QHBarModelMapper yearMapper;
	QtCharts::QVBarModelMapper monthMapper;
};

#endif // STATSCHARTWIDGET_H
