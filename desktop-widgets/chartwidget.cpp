#include "chartwidget.h"
#include "core/trip.h"
#include "qt-models/filtermodels.h"
#include "qt-models/datestatsmodel.h"

#include <QVXYModelMapper>
#include <QBarSeries>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QHeaderView>
#include <QBarCategoryAxis>
#include <QValueAxis>

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent,Qt::Window)
{
	model = new DateStatsTableModel(this);
	ui.setupUi(this);

	// create year chart layout
	yearChartView.setChart(drawYearChart());
	yearChartView.setRenderHint(QPainter::Antialiasing);
	QGridLayout *yearLayout = new QGridLayout;
	yearLayout->addWidget(&yearChartView, 1, 0);
	yearLayout->setColumnStretch(0, 1);
	this->ui.tabYearStats->setLayout(yearLayout);
	// position view to chartview
	this->ui.tabWidget->setCurrentIndex(this->ui.tabWidget->
		indexOf(this->ui.tabYearStats));

	// create month chart layout
	monthChartView.setChart(drawMonthChart());
	monthChartView.setRenderHint(QPainter::Antialiasing);
	QGridLayout *monthLayout = new QGridLayout;
	monthLayout->addWidget(&monthChartView, 1, 0);
	monthLayout->setColumnStretch(0, 1);
	this->ui.tabMonthStats->setLayout(monthLayout);

	connect(MultiFilterSortModel::instance(), &MultiFilterSortModel::filterFinished, this, &ChartWidget::filterFinished);
	connect(&diveListNotifier, &DiveListNotifier::divesAdded, this, &ChartWidget::filterFinished);
	connect(&diveListNotifier, &DiveListNotifier::divesDeleted, this, &ChartWidget::filterFinished);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &ChartWidget::filterFinished);
}

ChartWidget::~ChartWidget() {
	delete model;
}

QtCharts::QChart *ChartWidget::drawYearChart()
{
	int idx;

	QtCharts::QChart *yearChart = new QtCharts::QChart();

	// Set up QChart
	yearChart->setAnimationOptions(QtCharts::QChart::AllAnimations);
	yearChart->legend()->hide();
	yearChart->createDefaultAxes();
	yearChart->setTitle(tr("Years"));

	// Add data series using ModelMapper
	QtCharts::QBarSeries *series = new QtCharts::QBarSeries;
	yearMapper.setFirstBarSetRow(DateStatsTableModel::TOTAL);
	yearMapper.setLastBarSetRow(DateStatsTableModel::TOTAL);
	yearMapper.setFirstColumn(0);
	yearMapper.setColumnCount(model->columnCount());
	yearMapper.setSeries(series);
	yearMapper.setModel(model);
	yearChart->addSeries(series);

	// Add X Axis with labels
	QStringList categories;
	for (idx = 0; idx < model->columnCount(); idx++)
		categories << model->headerData(idx, Qt::Horizontal, Qt::DisplayRole).toString();
	QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
	axisX->append(categories);
	yearChart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	// Add Y Axis with tick marks
	QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
	axisY->setTickCount(model->axisMax(Qt::Horizontal) / 10 + 1);
	axisY->setMax(model->axisMax(Qt::Horizontal));
	axisY->setLabelFormat("%i");
	yearChart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	return(yearChart);
}

QtCharts::QChart *ChartWidget::drawMonthChart()
{
	QtCharts::QChart *monthChart = new QtCharts::QChart();

	// Set up QChart
	monthChart->setAnimationOptions(QtCharts::QChart::AllAnimations);
	monthChart->legend()->hide();
	monthChart->createDefaultAxes();
	monthChart->setTitle(tr("Months"));

	// Add data series using ModelMapper
	QtCharts::QStackedBarSeries *series = new QtCharts::QStackedBarSeries;
	monthMapper.setFirstBarSetColumn(0);
	monthMapper.setLastBarSetColumn(model->columnCount() - 1);
	monthMapper.setFirstRow(DateStatsTableModel::JAN);
	monthMapper.setRowCount(12);
	monthMapper.setSeries(series);
	monthMapper.setModel(model);
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
	axisY->setTickCount(model->axisMax(Qt::Vertical) / 5 + 1);
	axisY->setMax(model->axisMax(Qt::Vertical));
	axisY->setLabelFormat("%i");
	monthChart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	// Add legend for years
	monthChart->legend()->setVisible(true);
	monthChart->legend()->setAlignment(Qt::AlignBottom);

	return (monthChart);
}

void ChartWidget::filterFinished() {
	DateStatsTableModel *oldModel;
	QtCharts::QChart *oldChart;

	oldModel = model;
	model = new DateStatsTableModel(this);

	oldChart = yearChartView.chart();
	yearChartView.setChart(drawYearChart());
	delete oldChart;

	oldChart = monthChartView.chart();
	monthChartView.setChart(drawMonthChart());
	delete oldChart;

	delete oldModel;
}
