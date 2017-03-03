#ifndef TESTMERGE_H
#define TESTMERGE_H

#include <QtTest>

class TestMerge : public QObject{
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanup();

	void testMergeEmpty();
	void testMergeBackwards();
};

#endif
