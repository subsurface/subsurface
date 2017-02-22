#ifndef TESTMERGE_H
#define TESTMERGE_H

#include <QtTest>

class TestMerge : public QObject{
	Q_OBJECT
private slots:
	void initTestCase();
	void testMergeEmpty();
	void testMergeBackwards();
};

#endif
