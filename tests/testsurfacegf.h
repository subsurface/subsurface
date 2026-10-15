// SPDX-License-Identifier: GPL-2.0
#ifndef TESTSURFACEGF_H
#define TESTSURFACEGF_H

#include "testbase.h"

class TestSurfaceGf : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void testSurfacePressureConsistency();
	void testSurfaceGfCarryover();
	void testSurfaceTailLengthInvarianceOc();
	void testSurfaceTailLengthInvarianceCcr();
	void testCcrPo2CarryoverSensorVsSetpoint();
	void testCcrPo2CarryoverSensorless();
	void testCcrPo2VotingSparseSensors();
};

#endif
