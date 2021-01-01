// SPDX-License-Identifier: GPL-2.0
#include "statsseries.h"
#include "statsaxis.h"

#include <QChart>

StatsSeries::StatsSeries(QtCharts::QChart *chart, StatsAxis *xAxis, StatsAxis *yAxis)
{
	chart->addSeries(this);
	if (xAxis && yAxis) {
		attachAxis(xAxis->qaxis());
		attachAxis(yAxis->qaxis());
	}
}

StatsSeries::~StatsSeries()
{
}
