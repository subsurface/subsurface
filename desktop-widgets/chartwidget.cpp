#include "chartwidget.h"
#include "qt-models/datestatsmodel.h"

#include <QChart>
#include <QChartView>
#include <QVXYModelMapper>
#include <QBarSeries>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QVBarModelMapper>
#include <QHBarModelMapper>
#include <QHeaderView>
#include <QBarCategoryAxis>
#include <QValueAxis>

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent,Qt::Window)
{
	ui.setupUi(this);
	DrawYearChart();
	DrawMonthChart();
}

void ChartWidget::DrawYearChart()
{
	int idx;

	// Set up QChart
	QtCharts::QChart *yearChart = new QtCharts::QChart();
	yearChart->setAnimationOptions(QtCharts::QChart::AllAnimations);
	yearChart->legend()->hide();
	yearChart->createDefaultAxes();
	yearChart->setTitle(tr("Years"));

	// Add data series using ModelMapper
	QtCharts::QBarSeries *series = new QtCharts::QBarSeries;
	QtCharts::QHBarModelMapper *mapper = new QtCharts::QHBarModelMapper(this);
	mapper->setFirstBarSetRow(DateStatsTableModel::TOTAL);
	mapper->setLastBarSetRow(DateStatsTableModel::TOTAL);
	mapper->setFirstColumn(0);
	mapper->setColumnCount(model.columnCount());
	mapper->setSeries(series);
	mapper->setModel(&model);
	yearChart->addSeries(series);

	// Add X Axis with labels
	QStringList categories;
	for (idx = 0; idx < model.columnCount(); idx++)
		categories << model.headerData(idx, Qt::Horizontal, Qt::DisplayRole).toString();
	QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
	axisX->append(categories);
	yearChart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	// Add Y Axis with tick marks
	QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
	axisY->setTickCount(model.axisMax(Qt::Horizontal) / 10 + 1);
	axisY->setMax(model.axisMax(Qt::Horizontal));
	axisY->setLabelFormat("%i");
	yearChart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	// create layout
	QtCharts::QChartView *yearChartView = new QtCharts::QChartView(yearChart);
	yearChartView->setRenderHint(QPainter::Antialiasing);
	QGridLayout *layout = new QGridLayout;
	layout->addWidget(yearChartView, 1, 0);
	layout->setColumnStretch(0, 1);
	this->ui.tabYearStats->setLayout(layout);

	// position view to chartview
	this->ui.tabWidget->setCurrentIndex(this->ui.tabWidget->
		indexOf(this->ui.tabYearStats));
}

void ChartWidget::DrawMonthChart()
{
	// Set up QChart
	QtCharts::QChart *monthChart = new QtCharts::QChart();
	monthChart->setAnimationOptions(QtCharts::QChart::AllAnimations);
	monthChart->legend()->hide();
	monthChart->createDefaultAxes();
	monthChart->setTitle(tr("Months"));

	// Add data series using ModelMapper
	QtCharts::QStackedBarSeries *series = new QtCharts::QStackedBarSeries;
	QtCharts::QVBarModelMapper *mapper = new QtCharts::QVBarModelMapper(this);
	mapper->setFirstBarSetColumn(0);
	mapper->setLastBarSetColumn(model.columnCount() - 1);
	mapper->setFirstRow(DateStatsTableModel::JAN);
	mapper->setRowCount(12);
	mapper->setSeries(series);
	mapper->setModel(&model);
	monthChart->addSeries(series);

	// Add X Axis with labels
	QStringList categories;
	categories << tr("January") << tr("February") << tr("March") <<
		tr("April") << tr("May") << tr("June") <<
		tr("July") << tr("August") << tr("September") <<
		tr("October") << tr("November") << tr("December");
	QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
	axisX->append(categories);
	axisX->setLabelsAngle(270);
	monthChart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	// Add Y Axis with tick marks
	QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
	axisY->setTickCount(model.axisMax(Qt::Vertical) / 5 + 1);
	axisY->setMax(model.axisMax(Qt::Vertical));
	axisY->setLabelFormat("%i");
	monthChart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	// Add legend for years
	monthChart->legend()->setVisible(true);
	monthChart->legend()->setAlignment(Qt::AlignBottom);

	// create layout
	QtCharts::QChartView *monthChartView = new QtCharts::QChartView(monthChart);
	monthChartView->setRenderHint(QPainter::Antialiasing);
	QGridLayout *layout = new QGridLayout;
	layout->addWidget(monthChartView, 1, 0);
	layout->setColumnStretch(0, 1);
	this->ui.tabMonthStats->setLayout(layout);
}
