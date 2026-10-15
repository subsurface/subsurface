// SPDX-License-Identifier: GPL-2.0
#ifndef TESTOTUCNS_H
#define TESTOTUCNS_H

#include "testbase.h"

class TestOtuCns : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void testOcAirUnchanged();
	void testOcAirShallowSubThreshold();
	void testOcThresholdCrossing();
	void testCcrSensorsAgreeUnchanged();
	void testCcrSetpointNoCellData();
	void testCcrDeclaredSensorsNoCellData();
	void testCcrSensor0SilentOtherSensorsAgree();
	void testCcrSensor0Dropout();
	void testCcrSensor0VotedOutDrift();
	void testCcrNoO2SensorsStrayCell();
	void testCcrAmbientClamp();
	void testPscrSensorsAgreeUnchanged();
	void testPscrNoO2Sensors();
	void testPscrDeclaredSensorsNoCellData();
	void testCcrBailoutSilentCells();
};

#endif
