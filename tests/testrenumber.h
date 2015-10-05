#ifndef TESTRENUMBER_H
#define TESTRENUMBER_H

#include <QtTest>

class TestRenumber : public QObject
{
	Q_OBJECT
private slots:
	void setup();
	void testMerge();
	void testMergeAndAppend();
};

#endif
