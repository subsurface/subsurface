// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESUMMARY_H
#define DIVESUMMARY_H
#include <QStringList>

class diveSummary {

public:
	static void summaryCalculation(int primaryPeriod, int secondaryPeriod);

	static QStringList resultCalculation;

private:
	diveSummary() {}
};
#endif // DIVESUMMARY_H
