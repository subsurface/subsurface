#ifndef TESTGITSTORAGE_H
#define TESTGITSTORAGE_H

#include <QTest>

class TestGitStorage : public QObject
{
	Q_OBJECT
private slots:
	void testGitStorageLocal();
};

#endif // TESTGITSTORAGE_H
