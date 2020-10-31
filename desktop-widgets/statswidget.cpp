// SPDX-License-Identifier: GPL-2.0
#include "statswidget.h"
#include "mainwindow.h"
#include "stats/statsview.h"

StatsWidget::StatsWidget(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.close, &QToolButton::clicked, this, &StatsWidget::closeStats);
	connect(ui.plot, &QToolButton::clicked, this, &StatsWidget::plot);

	ui.chartType->addItems(StatsView::getChartTypes());
	connect(ui.chartType, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::chartTypeChanged);
	connect(ui.firstAxis, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::firstAxisChanged);
	connect(ui.secondAxis, QOverload<int>::of(&QComboBox::activated), this, &StatsWidget::secondAxisChanged);

	chartTypeChanged(0);
}

void StatsWidget::closeStats()
{
	MainWindow::instance()->setApplicationState(ApplicationState::Default);
}

void StatsWidget::plot()
{
	ui.stats->plot(
		ui.chartType->currentIndex(),
		ui.chartSubType->currentIndex(),
		ui.firstAxis->currentIndex(),
		ui.firstAxisBin->currentIndex(),
		ui.firstAxisOperation->currentIndex(),
		ui.secondAxis->currentIndex(),
		ui.secondAxisBin->currentIndex(),
		ui.secondAxisOperation->currentIndex()
	);
}

void StatsWidget::chartTypeChanged(int idx)
{
	QStringList subTypes = StatsView::getChartSubTypes(idx);
	ui.chartSubType->clear();
	if (subTypes.isEmpty()) {
		ui.chartSubType->hide();
	} else {
		ui.chartSubType->addItems(subTypes);
		ui.chartSubType->show();
	}

	QStringList firstAxisTypes = StatsView::getFirstAxisTypes(idx);
	ui.firstAxis->clear();
	if (firstAxisTypes.isEmpty()) {
		ui.firstAxis->hide();
		ui.firstAxisBin->hide();
		ui.firstAxisOperation->hide();
		ui.secondAxis->hide();
		ui.secondAxisBin->hide();
		ui.secondAxisOperation->hide();
		return;
	}

	ui.firstAxis->addItems(firstAxisTypes);
	ui.firstAxis->show();
	firstAxisChanged(0);
}

void StatsWidget::firstAxisChanged(int idx)
{
	int chartType = ui.chartType->currentIndex();
	QStringList firstAxisBins = StatsView::getFirstAxisBins(chartType, idx);
	ui.firstAxisBin->clear();
	if (firstAxisBins.size() <= 1) {
		ui.firstAxisBin->hide();
	} else {
		ui.firstAxisBin->addItems(firstAxisBins);
		ui.firstAxisBin->show();
	}

	QStringList firstAxisOperations = StatsView::getFirstAxisOperations(chartType, idx);
	ui.firstAxisOperation->clear();
	if (firstAxisOperations.size() <= 1) {
		ui.firstAxisOperation->hide();
	} else {
		ui.firstAxisOperation->addItems(firstAxisOperations);
		ui.firstAxisOperation->show();
	}

	QStringList secondAxisTypes = StatsView::getSecondAxisTypes(chartType, idx);
	ui.secondAxis->clear();
	if (secondAxisTypes.isEmpty()) {
		ui.secondAxis->hide();
		ui.secondAxisBin->hide();
		ui.secondAxisOperation->hide();
		return;
	}

	ui.secondAxis->addItems(secondAxisTypes);
	ui.secondAxis->show();
	secondAxisChanged(0);
}

void StatsWidget::secondAxisChanged(int idx)
{
	int chartType = ui.chartType->currentIndex();
	int firstAxisType = ui.firstAxis->currentIndex();
	QStringList secondAxisBins = StatsView::getSecondAxisBins(chartType, firstAxisType, idx);
	ui.secondAxisBin->clear();
	if (secondAxisBins.size() <= 1) {
		ui.secondAxisBin->hide();
	} else {
		ui.secondAxisBin->addItems(secondAxisBins);
		ui.secondAxisBin->show();
	}

	QStringList secondAxisOperations = StatsView::getSecondAxisOperations(chartType, firstAxisType, idx);
	ui.secondAxisOperation->clear();
	if (secondAxisOperations.size() <= 1) {
		ui.secondAxisOperation->hide();
	} else {
		ui.secondAxisOperation->addItems(secondAxisOperations);
		ui.secondAxisOperation->show();
	}

}
