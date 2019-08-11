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
	tagModel = new TagStatsListModel(this);
	buddyModel = new BuddyStatsListModel(this);
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

	// create tag chart layout
	tagChartView.setChart(drawTagChart());
	tagChartView.setRenderHint(QPainter::Antialiasing);
	QGridLayout *tagLayout = new QGridLayout;
	tagLayout->addWidget(&tagChartView, 1, 0);
	tagLayout->setColumnStretch(0, 1);
	this->ui.tabTagStats->setLayout(tagLayout);

	// create buddy chart layout
	buddyChartView.setChart(drawBuddyChart());
	buddyChartView.setRenderHint(QPainter::Antialiasing);
	QGridLayout *buddyLayout = new QGridLayout;
	buddyLayout->addWidget(&buddyChartView, 1, 0);
	buddyLayout->setColumnStretch(0, 1);
	this->ui.tabBuddyStats->setLayout(buddyLayout);

	connect(MultiFilterSortModel::instance(), &MultiFilterSortModel::filterFinished, this, &ChartWidget::filterFinished);
	connect(&diveListNotifier, &DiveListNotifier::divesAdded, this, &ChartWidget::filterFinished);
	connect(&diveListNotifier, &DiveListNotifier::divesDeleted, this, &ChartWidget::filterFinished);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &ChartWidget::filterFinished);
}

ChartWidget::~ChartWidget() {
	delete model;
	delete tagModel;
	delete buddyModel;
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

QtCharts::QChart *ChartWidget::drawTagChart()
{
	int idx;

	QtCharts::QChart *tagChart = new QtCharts::QChart();

	// Set up QChart
	tagChart->setAnimationOptions(QtCharts::QChart::AllAnimations);
	tagChart->legend()->hide();
	tagChart->createDefaultAxes();
	tagChart->setTitle(tr("Tags"));

	// Add data series using ModelMapper
	QtCharts::QBarSeries *series = new QtCharts::QBarSeries;
	tagMapper.setFirstBarSetColumn(0);
	tagMapper.setLastBarSetColumn(0);
	tagMapper.setFirstRow(0);
	tagMapper.setRowCount(tagModel->rowCount());
	tagMapper.setSeries(series);
	tagMapper.setModel(tagModel);
	tagChart->addSeries(series);

	// Add X Axis with labels
	QStringList tags;

	for (idx = 0; idx < tagModel->rowCount(); idx++) {
		tags << tagModel->headerData(idx, Qt::Vertical).toString();
	}
	QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
	axisX->append(tags);
	axisX->setLabelsAngle(270);
	tagChart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	// Add Y Axis with tick marks
	QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
	axisY->setTickCount(tagModel->axisMax() / 10 + 1);
	axisY->setMax(tagModel->axisMax());
	axisY->setLabelFormat("%i");
	tagChart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	return (tagChart);
}

QtCharts::QChart *ChartWidget::drawBuddyChart()
{
	int idx;

	QtCharts::QChart *buddyChart = new QtCharts::QChart();

	// Set up QChart
	buddyChart->setAnimationOptions(QtCharts::QChart::AllAnimations);
	buddyChart->legend()->hide();
	buddyChart->createDefaultAxes();
	buddyChart->setTitle(tr("Buddies"));

	// Add data series using ModelMapper
	QtCharts::QBarSeries *series = new QtCharts::QBarSeries;
	buddyMapper.setFirstBarSetColumn(0);
	buddyMapper.setLastBarSetColumn(0);
	buddyMapper.setFirstRow(0);
	buddyMapper.setRowCount(buddyModel->rowCount());
	buddyMapper.setSeries(series);
	buddyMapper.setModel(buddyModel);
	buddyChart->addSeries(series);

	// Add X Axis with labels
	QStringList buddies;

	for (idx = 0; idx < buddyModel->rowCount(); idx++) {
		buddies << buddyModel->headerData(idx, Qt::Vertical).toString();
	}
	QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
	axisX->append(buddies);
	axisX->setLabelsAngle(270);
	buddyChart->addAxis(axisX, Qt::AlignBottom);
	series->attachAxis(axisX);

	// Add Y Axis with tick marks
	QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
	axisY->setTickCount(buddyModel->axisMax() / 10 + 1);
	axisY->setMax(buddyModel->axisMax());
	axisY->setLabelFormat("%i");
	buddyChart->addAxis(axisY, Qt::AlignLeft);
	series->attachAxis(axisY);

	return (buddyChart);
}

void ChartWidget::filterFinished() {
	DateStatsTableModel *oldDateModel;
	TagStatsListModel *oldTagModel;
	BuddyStatsListModel *oldBuddyModel;
	QtCharts::QChart *oldChart;

	// Replace yearly and monthly charts and destroy old objects
	oldDateModel = model;
	model = new DateStatsTableModel(this);
	oldChart = yearChartView.chart();
	yearChartView.setChart(drawYearChart());
	delete oldChart;
	oldChart = monthChartView.chart();
	monthChartView.setChart(drawMonthChart());
	delete oldChart;
	delete oldDateModel;

	// Replace tags chart and destroy old objects
	oldTagModel = tagModel;
	tagModel = new TagStatsListModel(this);
	oldChart = tagChartView.chart();
	tagChartView.setChart(drawTagChart());
	delete oldChart;
	delete oldTagModel;

	// Replace buddy chart and destroy old objects
	oldBuddyModel = buddyModel;
	buddyModel = new BuddyStatsListModel(this);
	oldChart = buddyChartView.chart();
	buddyChartView.setChart(drawBuddyChart());
	delete oldChart;
	delete oldBuddyModel;
}
