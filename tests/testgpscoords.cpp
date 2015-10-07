#include <dive.h>
#include "testgpscoords.h"

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
		    coord2double(12, 8, 0.24 ), -coord2double(68, 16, 58.44));
}

void TestGpsCoords::testGoogle()
{
	testParseOK("12.133400, -68.282900",
		    coord2double(12, 8, 0.24 ), -coord2double(68, 16, 58.44));
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

QTEST_MAIN(TestGpsCoords)
