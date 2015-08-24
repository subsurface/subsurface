#ifndef TESTGITSTORAGE_H
#define TESTGITSTORAGE_H

#include <QTest>

class TestGitStorage : public QObject
{
	Q_OBJECT
private slots:
	void testGitStorageLocal();
	void testGitStorageCloud();
	void testGitStorageCloudOfflineSync();
};

#endif // TESTGITSTORAGE_H
