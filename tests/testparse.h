#ifndef TESTPARSE_H
#define TESTPARSE_H

#include <QtTest>

class TestParse : public QObject{
	Q_OBJECT
private slots:
	void initTestCase();
	void testParseCSV();
	void testParseDivingLog();
	void testParseV2NoQuestion();
	void testParseV3();
	void testParseCompareOutput();
	void testParseDM4();
	void testParseCompareDM4Output();
	void testParseHUDC();
	void testParseCompareHUDCOutput();
	void testParseNewFormat();
	void testParseCompareNewFormatOutput();
	void testParseDLD();
	void testParseCompareDLDOutput();
	void testParseMerge();
};

#endif
