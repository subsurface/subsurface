// SPDX-License-Identifier: GPL-2.0
// AI-generated (Codex)
#include "testcloudstorageauthenticate.h"
#include "core/cloudstorage.h"
#include "core/errorhelper.h"
#include "core/pref.h"
#include "core/settings/qPrefCloudStorage.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTcpSocket>
#include <QTimer>

namespace
{
	QString reportedError;
}

/** Keep preference writes inside the test's temporary directory. */
void TestCloudStorageAuthenticate::initTestCase()
{
	QVERIFY(settings.isValid());
	QCoreApplication::setOrganizationName("SubsurfaceTests");
	QCoreApplication::setApplicationName("CloudStorageAuthenticate");
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
}

/** Serve a small, complete HTTP response after reading the POST body. */
void TestCloudStorageAuthenticate::init()
{
	originalUrl = prefs.cloud_base_url;
	originalPassword = prefs.cloud_storage_password;
	originalStatus = prefs.cloud_verification_status;
	prefs.cloud_storage_password = "local-test-password";
	prefs.cloud_verification_status = qPrefCloudStorage::CS_UNKNOWN;
	responseStatus = 200;
	responseBody = "[OK]";
	reportedError.clear();
	set_error_cb([](std::string error) { reportedError = QString::fromStdString(error); });
	QVERIFY(server.listen(QHostAddress::LocalHost));
	prefs.cloud_base_url = QString("http://127.0.0.1:%1").arg(server.serverPort()).toStdString();
	connect(&server, &QTcpServer::newConnection, this, [this] {
		while (auto socket = server.nextPendingConnection()) {
			connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
			connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
				auto request = socket->property("request").toByteArray() + socket->readAll();
				socket->setProperty("request", request);
				const auto end = request.indexOf("\r\n\r\n");
				if (end < 0 || socket->property("handled").toBool())
					return;
				int contentLength = 0;
				for (const auto &line : request.left(end).split('\n')) {
					if (line.toLower().startsWith("content-length:"))
						contentLength = line.mid(sizeof("content-length:") - 1).trimmed().toInt();
				}
				if (request.size() < end + 4 + contentLength)
					return;
				socket->setProperty("handled", true);
				socket->write("HTTP/1.1 " + QByteArray::number(responseStatus) + " Test\r\nContent-Length: " +
					      QByteArray::number(responseBody.size()) + "\r\nConnection: close\r\n\r\n" + responseBody);
				socket->disconnectFromHost();
			});
		}
	});
}

/** Restore the shared state, callbacks, and loopback sockets after each case. */
void TestCloudStorageAuthenticate::cleanup()
{
	server.close();
	server.disconnect();
	qDeleteAll(server.findChildren<QTcpSocket *>());
	prefs.cloud_base_url = originalUrl;
	prefs.cloud_storage_password = originalPassword;
	prefs.cloud_verification_status = originalStatus;
	set_error_cb(nullptr);
}

/** Connection failure does not establish a new verification state. */
void TestCloudStorageAuthenticate::connectionRefused_data()
{
	QTest::addColumn<int>("initialStatus");
	QTest::newRow("unverified") << int(qPrefCloudStorage::CS_UNKNOWN);
	QTest::newRow("previously-verified") << int(qPrefCloudStorage::CS_VERIFIED);
	QTest::newRow("previously-rejected") << int(qPrefCloudStorage::CS_INCORRECT_USER_PASSWD);
}

/** A refused socket must not be reported as an incorrect password. */
void TestCloudStorageAuthenticate::connectionRefused()
{
	QFETCH(int, initialStatus);
	prefs.cloud_verification_status = initialStatus;
	server.close();
	CloudStorageAuthenticate auth(nullptr);
	QSignalSpy finished(&auth, &CloudStorageAuthenticate::finishedAuthenticate);
	auth.backend("test@example.invalid", "test-password");
	QTRY_COMPARE(finished.count(), 1);
	QCOMPARE(prefs.cloud_verification_status, initialStatus);
	QVERIFY(!auth.errorString().isEmpty());
	QVERIFY(!reportedError.isEmpty());
	QCOMPARE(prefs.cloud_storage_password, std::string("local-test-password"));
}

/** Cancelling an outstanding request is also a transport failure. */
void TestCloudStorageAuthenticate::cancelledRequest()
{
	CloudStorageAuthenticate auth(nullptr);
	QSignalSpy finished(&auth, &CloudStorageAuthenticate::finishedAuthenticate);
	auto reply = auth.backend("test@example.invalid", "test-password");
	reply->abort();
	QTRY_COMPARE(finished.count(), 1);
	QCOMPARE(prefs.cloud_verification_status, int(qPrefCloudStorage::CS_UNKNOWN));
	QVERIFY(!auth.errorString().isEmpty());
}

/** Server failures and empty responses do not establish invalid credentials. */
void TestCloudStorageAuthenticate::serviceFailures_data()
{
	QTest::addColumn<int>("status");
	QTest::addColumn<QByteArray>("body");
	QTest::newRow("internal-server-error") << 500 << QByteArray();
	QTest::newRow("unavailable") << 503 << QByteArray();
	QTest::newRow("unavailable-with-body") << 503 << QByteArray("[OK]");
	QTest::newRow("empty-forbidden") << 403 << QByteArray();
	QTest::newRow("empty-success") << 200 << QByteArray();
}

/** Confirm the real network reply finishes and leaves credentials unverified. */
void TestCloudStorageAuthenticate::serviceFailures()
{
	QFETCH(int, status);
	QFETCH(QByteArray, body);
	responseStatus = status;
	responseBody = body;
	CloudStorageAuthenticate auth(nullptr);
	QSignalSpy finished(&auth, &CloudStorageAuthenticate::finishedAuthenticate);
	auth.backend("test@example.invalid", "test-password");
	QTRY_COMPARE(finished.count(), 1);
	QCOMPARE(prefs.cloud_verification_status, int(qPrefCloudStorage::CS_UNKNOWN));
	QVERIFY(!auth.errorString().isEmpty());
	QVERIFY(!reportedError.isEmpty());
}

/** A successful retry on the same object must clear its earlier connection error. */
void TestCloudStorageAuthenticate::retryAfterServiceFailure()
{
	responseStatus = 503;
	responseBody.clear();
	CloudStorageAuthenticate auth(nullptr);
	QSignalSpy finished(&auth, &CloudStorageAuthenticate::finishedAuthenticate);
	auth.backend("test@example.invalid", "test-password");
	QTRY_COMPARE(finished.count(), 1);
	QVERIFY(!auth.errorString().isEmpty());
	responseStatus = 200;
	responseBody = "[OK]";
	finished.clear();
	auth.backend("test@example.invalid", "test-password");
	QTRY_COMPARE(finished.count(), 1);
	QVERIFY(auth.errorString().isEmpty());
	QCOMPARE(prefs.cloud_verification_status, int(qPrefCloudStorage::CS_VERIFIED));
}

/** Preserve successful verification, PIN handling and genuine auth rejection. */
void TestCloudStorageAuthenticate::authenticationResponses_data()
{
	QTest::addColumn<int>("status");
	QTest::addColumn<QByteArray>("body");
	QTest::addColumn<int>("expected");
	QTest::newRow("ok") << 200 << QByteArray("[OK]") << int(qPrefCloudStorage::CS_VERIFIED);
	QTest::newRow("verified") << 200 << QByteArray("[VERIFIED]") << int(qPrefCloudStorage::CS_VERIFIED);
	QTest::newRow("pin-required") << 200 << QByteArray("[VERIFY]") << int(qPrefCloudStorage::CS_NEED_TO_VERIFY);
	QTest::newRow("invalid-pin") << 200 << QByteArray("Invalid PIN") << int(qPrefCloudStorage::CS_NEED_TO_VERIFY);
	QTest::newRow("rejected") << 200 << QByteArray("Invalid credentials") << int(qPrefCloudStorage::CS_INCORRECT_USER_PASSWD);
	QTest::newRow("unauthorized") << 401 << QByteArray() << int(qPrefCloudStorage::CS_INCORRECT_USER_PASSWD);
	QTest::newRow("rejected-with-4xx") << 403 << QByteArray("Invalid credentials") << int(qPrefCloudStorage::CS_INCORRECT_USER_PASSWD);
}

/** Check each existing authentication outcome through the HTTP endpoint. */
void TestCloudStorageAuthenticate::authenticationResponses()
{
	QFETCH(int, status);
	QFETCH(QByteArray, body);
	QFETCH(int, expected);
	responseStatus = status;
	responseBody = body;
	CloudStorageAuthenticate auth(nullptr);
	QSignalSpy finished(&auth, &CloudStorageAuthenticate::finishedAuthenticate);
	auth.backend("test@example.invalid", "test-password");
	QTRY_COMPARE(finished.count(), 1);
	QCOMPARE(prefs.cloud_verification_status, expected);
	QVERIFY(auth.errorString().isEmpty());
}

QTEST_GUILESS_MAIN(TestCloudStorageAuthenticate)
