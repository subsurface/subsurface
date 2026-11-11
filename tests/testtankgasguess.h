// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef TESTTANKGASGUESS_H
#define TESTTANKGASGUESS_H

#include "testbase.h"

class TestTankGasGuess : public TestBase {
	Q_OBJECT
private slots:
	void sidemountWithClosedStages();
	void allTanksOnOneGas();
	void unusedStage();
	void stageChargedAndClosed();
	void valveDrill();
	void mostRecentDropsTakenBack();
	void ambiguous();
	void oneSidedSidemount();
	void pairedSidemountBelowMinimum();
	void moreThanTwoSidemount();
	void sidemountOnDifferentGases();
	void inclusiveLimits();
	void gasChangeDuringMissingReadings();
	void noGasMixes();
	void applyGuesses();
};

#endif
