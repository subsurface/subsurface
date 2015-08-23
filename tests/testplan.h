#ifndef TESTPLAN_H
#define TESTPLAN_H

#include <QTest>

class TestPlan : public QObject {
	Q_OBJECT
private slots:
	void testImperial();
	void testMetric();
	void testVpmbMetric();
};

#endif // TESTPLAN_H
