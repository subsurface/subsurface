// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "testupdatecheck.h"
#include "desktop-widgets/updatecheck.h"

Q_DECLARE_METATYPE(build_kind)

static const char *const kinds[] = { "local", "pr", "ci" };
static const build_kind allKinds[] = { build_kind::local, build_kind::pr, build_kind::ci };

// run the same test for every kind of build, both for automatic and manual checks
static void addKindRows(const char *name, const char *reply)
{
	for (build_kind kind: allKinds) {
		for (bool automatic: { true, false })
			QTest::newRow(qPrintable(QString("%1 %2 %3").arg(name, kinds[(int)kind], automatic ? "automatic" : "manual")))
				<< kind << automatic << QString(reply);
	}
}

static void addKindColumns()
{
	QTest::addColumn<build_kind>("kind");
	QTest::addColumn<bool>("automatic");
	QTest::addColumn<QString>("reply");
}

void TestUpdateCheck::testRequestParameters_data()
{
	QTest::addColumn<build_kind>("kind");
	QTest::addColumn<int>("pr");
	QTest::addColumn<QString>("expected");

	QTest::newRow("local") << build_kind::local << 0 << "&build=local";
	QTest::newRow("local ignores the number") << build_kind::local << 5170 << "&build=local";
	QTest::newRow("ci") << build_kind::ci << 0 << "&build=ci";
	QTest::newRow("ci ignores the number") << build_kind::ci << 5170 << "&build=ci";
	QTest::newRow("pr") << build_kind::pr << 5170 << "&build=pr&pr=5170";
	QTest::newRow("pr unknown number") << build_kind::pr << 0 << "&build=pr";
}

void TestUpdateCheck::testRequestParameters()
{
	QFETCH(build_kind, kind);
	QFETCH(int, pr);
	QFETCH(QString, expected);
	// the number is only reported for pull request builds
	QCOMPARE(updateCheckBuildParameters(kind, pr), expected);
}

void TestUpdateCheck::testUpToDate_data()
{
	addKindColumns();
	addKindRows("ok", "OK");
}

void TestUpdateCheck::testUpToDate()
{
	QFETCH(build_kind, kind);
	QFETCH(bool, automatic);
	QFETCH(QString, reply);
	UpdateCheckOutcome outcome = evaluateUpdateReply(kind, 5170, automatic, reply);
	// silent when checking automatically, an answer when asked
	QCOMPARE(outcome.showPopup, !automatic);
	QVERIFY(outcome.message.contains("You are using the latest version of Subsurface."));
	QVERIFY(!outcome.warning);
}

void TestUpdateCheck::testNewVersionReplies_data()
{
	addKindColumns();
	addKindRows("link", "[\"http://subsurface-divelog.org/download\"]");
	addKindRows("latest", "Latest version is \"6.0.5900\"");
}

void TestUpdateCheck::testNewVersionReplies()
{
	QFETCH(build_kind, kind);
	QFETCH(bool, automatic);
	QFETCH(QString, reply);
	UpdateCheckOutcome outcome = evaluateUpdateReply(kind, 5170, automatic, reply);
	QVERIFY(outcome.showPopup);
	QVERIFY(outcome.message.contains("A new version of Subsurface is available."));
	if (reply.startsWith("[")) {
		QVERIFY(outcome.message.contains("<a href=\"http://subsurface-divelog.org/download\">"));
		QVERIFY(outcome.message.contains("to download it."));
	} else {
		QVERIFY(outcome.message.contains("Latest version is 6.0.5900"));
		QVERIFY(outcome.message.contains("http://subsurface-divelog.org/download"));
	}
}

void TestUpdateCheck::testNewerRelease_data()
{
	addKindColumns();
	addKindRows("plain", "There is a newer release");
	addKindRows("version", "There is a newer release. Newest release version is 6.0.5900");
}

void TestUpdateCheck::testNewerRelease()
{
	QFETCH(build_kind, kind);
	QFETCH(bool, automatic);
	QFETCH(QString, reply);
	UpdateCheckOutcome outcome = evaluateUpdateReply(kind, 5170, automatic, reply);
	// a real newer release is shown on automatic checks for every kind of build
	QVERIFY(outcome.showPopup);
	QVERIFY(outcome.message.contains("A newer release of Subsurface is available."));
	QVERIFY(outcome.message.contains("https://subsurface-divelog.org/download"));
	QCOMPARE(outcome.message.contains("version 6.0.5900"), reply.contains("6.0.5900"));
	// only a pull request build is also told that it is a test build
	QCOMPARE(outcome.message.contains("test build of PR #5170"), kind == build_kind::pr);
}

void TestUpdateCheck::testPullRequestReminder_data()
{
	QTest::addColumn<QString>("reply");
	QTest::addColumn<bool>("automatic");
	QTest::addColumn<int>("pr");
	QTest::addColumn<QString>("version");

	const char *replies[] = {
		"You are running a local build based on the current release",
		"You are running the latest development version",
		"You are running a beta version",
		"You are running a version newer than the latest release",
		"You are running a local build based on the current release. Newest release version is 6.0.5900",
		"You are running the latest development version. Newest release version is 6.0.5900",
	};
	for (const char *reply: replies) {
		for (bool automatic: { true, false }) {
			for (int pr: { 5170, 0 }) {
				QString version = QString(reply).contains("6.0.5900") ? "6.0.5900" : "";
				QTest::newRow(qPrintable(QString("%1 %2 %3").arg(reply).arg(automatic ? "automatic" : "manual").arg(pr)))
					<< QString(reply) << automatic << pr << version;
			}
		}
	}
}

void TestUpdateCheck::testPullRequestReminder()
{
	QFETCH(QString, reply);
	QFETCH(bool, automatic);
	QFETCH(int, pr);
	QFETCH(QString, version);
	UpdateCheckOutcome outcome = evaluateUpdateReply(build_kind::pr, pr, automatic, reply);
	// somebody testing a pull request is reminded on every check
	QVERIFY(outcome.showPopup);
	if (pr > 0)
		QVERIFY(outcome.message.contains("You are running a test build of PR #5170, not the latest version."));
	else
		QVERIFY(outcome.message.contains("You are running a pull request test build, not the latest version."));
	QVERIFY(outcome.message.contains("https://subsurface-divelog.org/download"));
	QVERIFY(outcome.message.contains("https://github.com/subsurface/subsurface/releases"));
	if (version.isEmpty())
		QVERIFY(!outcome.message.contains("The latest release is version"));
	else
		QVERIFY(outcome.message.contains("The latest release is version " + version));
	// the wording does not depend on the prose of the server
	QVERIFY(!outcome.message.contains("The server returned"));
	QVERIFY(!outcome.message.contains(reply));
}

void TestUpdateCheck::testLocalBuild_data()
{
	QTest::addColumn<bool>("automatic");
	QTest::addColumn<QString>("reply");
	for (bool automatic: { true, false }) {
		const char *when = automatic ? "automatic" : "manual";
		QTest::newRow(qPrintable(QString("local based on release %1").arg(when))) << automatic
			<< QString("You are running a local build based on the current release");
		QTest::newRow(qPrintable(QString("development %1").arg(when))) << automatic
			<< QString("You are running the latest development version. Newest release version is 6.0.5900");
	}
}

void TestUpdateCheck::testLocalBuild()
{
	QFETCH(bool, automatic);
	QFETCH(QString, reply);
	UpdateCheckOutcome outcome = evaluateUpdateReply(build_kind::local, 0, automatic, reply);
	// the information is only given when asked for
	QCOMPARE(outcome.showPopup, !automatic);
	QVERIFY(outcome.message.contains("You are running a local build of Subsurface"));
	QVERIFY(outcome.message.contains("https://subsurface-divelog.org/download"));
	QCOMPARE(outcome.message.contains("The latest release is version 6.0.5900"), reply.contains("6.0.5900"));
}

void TestUpdateCheck::testCiBuildInformation_data()
{
	QTest::addColumn<bool>("automatic");
	QTest::addColumn<QString>("reply");
	for (bool automatic: { true, false }) {
		const char *when = automatic ? "automatic" : "manual";
		QTest::newRow(qPrintable(QString("development %1").arg(when))) << automatic
			<< QString("You are running the latest development version");
		QTest::newRow(qPrintable(QString("beta %1").arg(when))) << automatic
			<< QString("You are running a Beta version");
	}
}

void TestUpdateCheck::testCiBuildInformation()
{
	QFETCH(bool, automatic);
	QFETCH(QString, reply);
	UpdateCheckOutcome outcome = evaluateUpdateReply(build_kind::ci, 0, automatic, reply);
	// as before: only shown when asked for, with what the server said
	QCOMPARE(outcome.showPopup, !automatic);
	QVERIFY(outcome.warning);
	QVERIFY(outcome.message.contains("The server returned the following information:"));
	QVERIFY(outcome.message.contains(reply));
}

void TestUpdateCheck::testUnknownReply_data()
{
	addKindColumns();
	addKindRows("garbage", "<html>502 Bad Gateway</html>");
	addKindRows("empty", "");
}

void TestUpdateCheck::testUnknownReply()
{
	QFETCH(build_kind, kind);
	QFETCH(bool, automatic);
	QFETCH(QString, reply);
	UpdateCheckOutcome outcome = evaluateUpdateReply(kind, 5170, automatic, reply);
	// not recognized: as before, shown even on automatic checks
	QVERIFY(outcome.showPopup);
	QVERIFY(outcome.warning);
	QVERIFY(outcome.message.contains("The server returned the following information:"));
	QVERIFY(outcome.message.contains(reply));
}

QTEST_GUILESS_MAIN(TestUpdateCheck)
