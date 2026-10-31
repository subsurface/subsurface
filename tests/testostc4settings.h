// SPDX-License-Identifier: GPL-2.0
#ifndef TESTOSTC4SETTINGS_H
#define TESTOSTC4SETTINGS_H
#include "testbase.h"
class TestOstc4Settings : public TestBase {
	Q_OBJECT
private slots:
	void gate();
	void supportedTransactions();
	void bootloaderIdentity();
	void hardwareReplyBoundary();
	void gfRequiresReadablePartner();
	void transaction();
	void backupFailures();
	void recovery();
	void legacyParity();
};
#endif
