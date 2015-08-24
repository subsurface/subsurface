#ifndef TESTPLAN_H
#define TESTPLAN_H

#include <QTest>

class TestPlan : public QObject {
	Q_OBJECT
private slots:
	void testImperial();
	void testMetric();
	void testVpmbMetric100m60min();
	void testVpmbMetric100m10min();
};

#endif // TESTPLAN_H
