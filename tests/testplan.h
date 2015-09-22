#ifndef TESTPLAN_H
#define TESTPLAN_H

#include <QTest>

class TestPlan : public QObject {
	Q_OBJECT
private slots:
	void testImperial();
	void testMetric();
	void testVpmbMetric60m30minAir();
	void testVpmbMetric60m30minEan50();
	void testVpmbMetric60m30minTx();
	void testVpmbMetricMultiLevelAir();
	void testVpmbMetric100m60min();
	void testVpmbMetric100m10min();
	void testVpmbMetricRepeat();
};

#endif // TESTPLAN_H
