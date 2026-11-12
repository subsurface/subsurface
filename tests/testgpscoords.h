// SPDX-License-Identifier: GPL-2.0
#ifndef TESTGPSCOORDS_H
#define TESTGPSCOORDS_H

#include "testbase.h"

class TestGpsCoords : public TestBase {
	Q_OBJECT
private slots:
	void testISO6709DParse();
	void testNegativeISO6709DParse();
	void testSpaceISO6709DParse();
	void testSecondsParse();
	void testSpaceSecondsParse();
	void testNegativeSecondsParse();
	void testMinutesParse();
	void testSpaceMinutesParse();
	void testMinutesInversedParse();
	void testDecimalParse();
	void testSpaceDecimalParse();
	void testDecimalInversedParse();
	void testXmlFormatParse();
	void testNoUnitParse();
	void testNegativeXmlFormatParse();
	void testPrefixNoUnitParse();
	void testOurWeb();
	void testGoogle();
	void testSwappedCorrected();
	void testSwappedNotCorrectedWithHemispheres();
	void testUnreadableSeparatorRejected();
	void testOutOfRangeRejected();
	void testRangeLimitsAccepted();
	void testSignedPlainPairs();
	void testCommaOnlyPairs();
	void testWrongWayRoundWithUnitsRejected();
	void testOutOfRangeWithUnitsRejected();
	void testRoundTrip();

private:
	static void testParseFails(const QString &txt);
	static void testParseOK(const QString &txt, double expectedLat,
				double expectedLon);
	static double coord2double(double deg, double min = 0.0, double sec = 0.0);
};

#endif
