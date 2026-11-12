// SPDX-License-Identifier: GPL-2.0
#include "testgpscoords.h"
#include "core/pref.h"
#include "core/string-format.h"
#include "core/units.h"
#include <cstdlib>

//unit under test
extern bool parseGpsText(const QString &gps_text, double *latitude, double *longitude);


void TestGpsCoords::testISO6709DParse()
{
	testParseOK("52°49'02.388\"N 1°36'17.388\"E",
		    coord2double(52, 49, 2.388), coord2double(1, 36, 17.388));
}

void TestGpsCoords::testNegativeISO6709DParse()
{
	testParseOK("52°49'02.388\"S 1°36'17.388\"W",
		    coord2double(-52, -49, -2.388), coord2double(-1, -36, -17.388));
}

void TestGpsCoords::testSpaceISO6709DParse()
{
	testParseOK("52 ° 49 ' 02.388 \" N  1 ° 36 ' 17.388 \" E",
		    coord2double(52, 49, 2.388), coord2double(1, 36, 17.388));
}

void TestGpsCoords::testSecondsParse()
{
	testParseOK("N52°49'02.388\" E1°36'17.388\"",
		    coord2double(52, 49, 2.388), coord2double(1, 36, 17.388));
}

void TestGpsCoords::testSpaceSecondsParse()
{
	testParseOK(" N 52 ° 49 ' 02.388 \"  E 1 ° 36 ' 17.388 \"",
		    coord2double(52, 49, 2.388), coord2double(1, 36, 17.388));
}

void TestGpsCoords::testNegativeSecondsParse()
{
	testParseOK("-52°49'02.388\" -1°36'17.388\"",
		    coord2double(-52, -49, -2.388), coord2double(-1, -36, -17.388));
}

void TestGpsCoords::testMinutesParse()
{
	testParseOK("N52°49.03' E1d36.23'",
		    coord2double(52, 49.03), coord2double(1, 36.23));
}

void TestGpsCoords::testSpaceMinutesParse()
{
	testParseOK(" N 52 ° 49.03 '  E 1 ° 36.23 ' ",
		    coord2double(52, 49.03), coord2double(1, 36.23));
}

void TestGpsCoords::testMinutesInversedParse()
{
	testParseOK("2° 53.836' N 73° 32.839' E",
		    coord2double(2, 53.836), coord2double(73, 32.839));
}

void TestGpsCoords::testDecimalParse()
{
	testParseOK("N52.83° E1.61",
		    coord2double(52.83), coord2double(1.61));
}

void TestGpsCoords::testDecimalInversedParse()
{
	testParseOK("52.83N 1.61E",
		    coord2double(52.83), coord2double(1.61));
}

void TestGpsCoords::testSpaceDecimalParse()
{
	testParseOK(" N 52.83  E 1.61 ",
		    coord2double(52.83), coord2double(1.61));
}

void TestGpsCoords::testXmlFormatParse()
{
	testParseOK("46.473881 6.784696",
		    coord2double(46.473881), coord2double(6.784696));
}

void TestGpsCoords::testNegativeXmlFormatParse()
{
	testParseOK("46.473881 -6.784696",
		    coord2double(46.473881), -coord2double(6.784696));
}

void TestGpsCoords::testNoUnitParse()
{
	testParseOK("48 51.491n 2 17.677e",
		    coord2double(48, 51.491), coord2double(2, 17.677));
}

void TestGpsCoords::testPrefixNoUnitParse()
{
	testParseOK("n48 51.491 w2 17.677",
		    coord2double(48, 51.491), -coord2double(2, 17.677));
}

void TestGpsCoords::testOurWeb()
{
	testParseOK("12° 8' 0.24\" , -68° 16' 58.44\"",
		    coord2double(12, 8, 0.24), -coord2double(68, 16, 58.44));
}

void TestGpsCoords::testGoogle()
{
	testParseOK("12.133400, -68.282900",
		    coord2double(12, 8, 0.24), -coord2double(68, 16, 58.44));
}

// AI-generated (Claude)
void TestGpsCoords::testSwappedRejected()
{
	// Latitude and longitude entered the wrong way round: a latitude
	// of 151 cannot exist, so this must not be accepted.
	testParseFails("151.266423, -33.984807");
	testParseOK("-33.984807, 151.266423", -33.984807, 151.266423);
}

void TestGpsCoords::testOutOfRangeRejected()
{
	testParseFails("90.5 0");
	testParseFails("-91, 10");
	testParseFails("0, 180.5");
	testParseFails("10 -181");
	testParseFails("95°N 10°E");
	testParseFails("10°N 185°W");
}

void TestGpsCoords::testRangeLimitsAccepted()
{
	testParseOK("90, 180", 90.0, 180.0);
	testParseOK("-90 -180", -90.0, -180.0);
}

void TestGpsCoords::testOutOfRangeWithUnitsRejected()
{
	testParseFails("90°00'00.36\"N 10°E");
	testParseFails("91°S 10°E");
	testParseFails("33°59'05.3\"S 180°00'01\"E");
	testParseFails("33.98S 181W");
	testParseFails("N95.5 E10");
}

void TestGpsCoords::testRoundTrip()
{
	// Coordinates shown in the dive site editor must be accepted unchanged
	// when they are edited, in both display formats.
	const location_t locations[] = {
		create_location(-33.984807, 151.266423),
		create_location(12.1334, -68.2829),
		create_location(89.999999, -179.999999),
		create_location(-90.0, 180.0),
		create_location(-0.5, 0.25),
	};
	const bool saved = prefs.coordinates_traditional;
	for (bool traditional: { true, false }) {
		prefs.coordinates_traditional = traditional;
		for (const location_t &loc: locations) {
			const QString text = printGPSCoords(&loc);
			double lat, lon;
			QVERIFY2(parseGpsText(text, &lat, &lon), qPrintable(text));
			const location_t parsed = create_location(lat, lon);
			QVERIFY2(std::abs(parsed.lat.udeg - loc.lat.udeg) <= 1, qPrintable(text));
			QVERIFY2(std::abs(parsed.lon.udeg - loc.lon.udeg) <= 1, qPrintable(text));
		}
	}
	prefs.coordinates_traditional = saved;
}

void TestGpsCoords::testParseFails(const QString &txt)
{
	double lat, lon;
	QVERIFY2(!parseGpsText(txt, &lat, &lon), qPrintable(txt));
}

void TestGpsCoords::testParseOK(const QString &txt, double expectedLat,
				double expectedLon)
{
	double actualLat, actualLon;
	QVERIFY(parseGpsText(txt, &actualLat, &actualLon));
	QCOMPARE(actualLat, expectedLat);
	QCOMPARE(actualLon, expectedLon);
}

double TestGpsCoords::coord2double(double deg, double min, double sec)
{
	return deg + min / 60.0 + sec / 3600.0;
}

QTEST_GUILESS_MAIN(TestGpsCoords)
