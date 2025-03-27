// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPLAN_H
#define TESTPLAN_H

#include "testbase.h"

class TestPlan : public TestBase {
	Q_OBJECT
private slots:
	void testMetric();
	void testImperial();
	void testVpmbMetric45m30minTx();
	void testVpmbMetric60m10minTx();
	void testVpmbMetric60m30minAir();
	void testVpmbMetric60m30minEan50();
	void testVpmbMetric60m30minTx();
	void testVpmbMetric100m60min();
	void testVpmbMetricMultiLevelAir();
	void testVpmbMetric100m10min();
	void testVpmbMetricRepeat();
	void testMultipleGases();
	void testCcrBailoutGasSelection();
};

#endif // TESTPLAN_H
