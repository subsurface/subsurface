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
	void testEndSurfaceGfSquareDive();
	void testEndSurfaceGfRepetitiveDive();
	void testEndSurfaceGfNoSamples();
	void testEndSurfaceGfOverlappingDives();
	void testEndSurfaceGfOverlappingDivesEarlierMember();
	void testEndSurfaceGfNonAdjacentOverlap();
	void testEndSurfaceGfOneDiveInsideAnother();
	void testEndSurfaceGfPredecessorsOverlapSameTrip();
	void testEndSurfaceGfOtherTripOverlapDoesNotAffectDive();
	void testEndSurfaceGfCrossTripWithin48hUnaffected();
	void testEndSurfaceGfNotInTableOverlapsTableEntry();
	void testEndSurfaceGfSimultaneousDives();
	void testEndSurfaceGfUnaccountedMultiDcTail();
	void testSurfaceTailLengthInvarianceOc();
	void testSurfaceTailLengthInvarianceCcr();
	void testCcrPo2CarryoverSensorVsSetpoint();
	void testCcrPo2CarryoverSensorless();
	void testCcrPo2VotingSparseSensors();
};

#endif
