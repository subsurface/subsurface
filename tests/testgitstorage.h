// SPDX-License-Identifier: GPL-2.0
#ifndef TESTGITSTORAGE_H
#define TESTGITSTORAGE_H

#include "testbase.h"

class TestGitStorage : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanup();
	void cleanupTestCase();

	void testGitStorageLocal_data();
	void testGitStorageLocal();
	void testGitStorageCloud();
	void testGitStorageCloudOfflineSync();
	void testGitStorageCloudMerge();
	void testGitStorageCloudMerge2();
	void testGitStorageCloudMerge3();
};

#endif // TESTGITSTORAGE_H
