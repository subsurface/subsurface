// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "testversion.h"
#include "core/version.h"

Q_DECLARE_METATYPE(build_kind)

void TestVersion::testBuildKind_data()
{
	QTest::addColumn<QByteArray>("version");
	QTest::addColumn<build_kind>("kind");

	QTest::newRow("release") << QByteArray("6.0.5840-CICD-release") << build_kind::ci;
	QTest::newRow("release with patch") << QByteArray("6.0.5840-patch.3.CICD-release") << build_kind::ci;
	QTest::newRow("local") << QByteArray("6.0.5840-local") << build_kind::local;
	QTest::newRow("local with patch") << QByteArray("6.0.5840-patch.3.local") << build_kind::local;
	QTest::newRow("pr with patch") << QByteArray("6.0.5840-patch.3.pull-request.5170") << build_kind::pr;
	QTest::newRow("pr") << QByteArray("6.0.5840-pull-request.5170") << build_kind::pr;
	QTest::newRow("legacy pr branch") << QByteArray("6.0.5840-patch.3.pull-request.feat-foo") << build_kind::pr;
	QTest::newRow("legacy pr empty") << QByteArray("6.0.5840-patch.3.pull-request.") << build_kind::pr;
	QTest::newRow("legacy snap pr") << QByteArray("6.0.100-CICD-pull-request") << build_kind::pr;
	QTest::newRow("adhoc") << QByteArray("6.0.5840-adhoc") << build_kind::pr;
	QTest::newRow("adhoc with patch") << QByteArray("6.0.5840-patch.2.adhoc") << build_kind::pr;
	QTest::newRow("no suffix") << QByteArray("6.0.5840") << build_kind::local;
	QTest::newRow("empty") << QByteArray("") << build_kind::local;
	QTest::newRow("garbage") << QByteArray("garbage-in-garbage-out") << build_kind::local;
	QTest::newRow("only dash") << QByteArray("-") << build_kind::local;
	QTest::newRow("branch with digits") << QByteArray("6.0.5840-pull-request.fix-1234-x") << build_kind::pr;
	QTest::newRow("unknown suffix") << QByteArray("6.0.5840-beta.1") << build_kind::local;
	QTest::newRow("patch without number") << QByteArray("6.0.5840-patch.local") << build_kind::local;
	QTest::newRow("patch without suffix") << QByteArray("6.0.5840-patch.3.") << build_kind::local;
	QTest::newRow("release lookalike") << QByteArray("6.0.5840-CICD-release-x") << build_kind::local;
}

void TestVersion::testBuildKind()
{
	QFETCH(QByteArray, version);
	QFETCH(build_kind, kind);
	QVERIFY(parse_build_kind(version.constData()) == kind);
}

void TestVersion::testPrNumber_data()
{
	QTest::addColumn<QByteArray>("version");
	QTest::addColumn<int>("number");

	QTest::newRow("pr with patch") << QByteArray("6.0.5840-patch.3.pull-request.5170") << 5170;
	QTest::newRow("pr") << QByteArray("6.0.5840-pull-request.5170") << 5170;
	QTest::newRow("release") << QByteArray("6.0.5840-CICD-release") << 0;
	QTest::newRow("local") << QByteArray("6.0.5840-local") << 0;
	QTest::newRow("legacy pr branch") << QByteArray("6.0.5840-patch.3.pull-request.feat-foo") << 0;
	QTest::newRow("legacy pr empty") << QByteArray("6.0.5840-patch.3.pull-request.") << 0;
	QTest::newRow("legacy snap pr") << QByteArray("6.0.100-CICD-pull-request") << 0;
	QTest::newRow("adhoc") << QByteArray("6.0.5840-adhoc") << 0;
	QTest::newRow("no suffix") << QByteArray("6.0.5840") << 0;
	QTest::newRow("empty") << QByteArray("") << 0;
	QTest::newRow("garbage") << QByteArray("garbage") << 0;
	QTest::newRow("branch with digits") << QByteArray("6.0.5840-pull-request.fix-1234-x") << 0;
	QTest::newRow("branch starting with digits") << QByteArray("6.0.5840-pull-request.1234-x") << 0;
	QTest::newRow("too many digits") << QByteArray("6.0.5840-pull-request.12345678901234567890") << 0;
}

void TestVersion::testPrNumber()
{
	QFETCH(QByteArray, version);
	QFETCH(int, number);
	QCOMPARE(parse_pr_number(version.constData()), number);
}

void TestVersion::testOwnVersion()
{
	// The accessors apply the parsers to the version of this binary.
	QVERIFY(subsurface_build_kind() == parse_build_kind(subsurface_canonical_version()));
	QCOMPARE(subsurface_pr_number(), parse_pr_number(subsurface_canonical_version()));
	// null pointers must not crash
	QVERIFY(parse_build_kind(nullptr) == build_kind::local);
	QCOMPARE(parse_pr_number(nullptr), 0);
}

QTEST_GUILESS_MAIN(TestVersion)
