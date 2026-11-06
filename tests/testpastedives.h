// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPASTEDIVES_H
#define TESTPASTEDIVES_H

#include <QtTest>

// Tests for Command::pasteDives(). This test links the real undo commands (as opposed to the
// stubs used by most other tests) and therefore does not use TestBase.
class TestPasteDives : public QObject {
	Q_OBJECT
private slots:
	void initTestCase();
	void init();
	void cleanup();

	void pasteTimeSingleDive();
	void pasteTimeMultipleDives();
	void pasteTimeFirstDiveOfTripPastOtherTrip();
	void pasteWithoutTimeSendsNoTimeSignal();
};

#endif // TESTPASTEDIVES_H
