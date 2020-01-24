// SPDX-License-Identifier: GPL-2.0
#include "divesummary.h"

QStringList diveSummary::resultCalculation;

void diveSummary::summaryCalculation(int primaryPeriod, int secondaryPeriod)
{
	resultCalculation.clear();
	resultCalculation << "13/12/2001" << "10/1/2020" <<
						"120" << "8" <<
					   "12"  << "1" <<
					   "50"  << "10" <<
					   "200h" << "10h" <<
					   "2:31" << "0:51" <<
					   "1:15" << "0:41" <<
					   "62m" << "31m" <<
					   "23m" << "29m" <<
					   "11 l/min" << "12 l/min" <<
					   "12 l/min" << "16 l/min" <<
					   "4" << "1";
}

