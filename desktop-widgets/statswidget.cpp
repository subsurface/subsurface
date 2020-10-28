// SPDX-License-Identifier: GPL-2.0
#include "statswidget.h"
#include "mainwindow.h"

StatsWidget::StatsWidget(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.close, &QToolButton::clicked, this, &StatsWidget::closeStats);
	connect(ui.plot, &QToolButton::clicked, this, &StatsWidget::plot);
}

void StatsWidget::closeStats()
{
	MainWindow::instance()->setApplicationState(ApplicationState::Default);
}

void StatsWidget::plot()
{
	ui.stats->plot();
}
