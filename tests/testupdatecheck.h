// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef TESTUPDATECHECK_H
#define TESTUPDATECHECK_H

#include "testbase.h"

class TestUpdateCheck : public TestBase {
	Q_OBJECT
private slots:
	void testRequestParameters_data();
	void testRequestParameters();
	void testUpToDate_data();
	void testUpToDate();
	void testNewVersionReplies_data();
	void testNewVersionReplies();
	void testNewerRelease_data();
	void testNewerRelease();
	void testPullRequestReminder_data();
	void testPullRequestReminder();
	void testLocalBuild_data();
	void testLocalBuild();
	void testCiBuildInformation_data();
	void testCiBuildInformation();
	void testUnknownReply_data();
	void testUnknownReply();
};

#endif
