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
	void testGitStorageCloudMerge();
};

#endif // TESTGITSTORAGE_H
