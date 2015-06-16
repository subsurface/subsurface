#ifndef TESTPARSE_H
#define TESTPARSE_H

#include <QtTest>

class TestParse : public QObject{
	Q_OBJECT
private slots:
	void testParseCSV();
	void testParseV2NoQuestion();
	void testParseV3();
	void testParseCompareOutput();
};

#endif
