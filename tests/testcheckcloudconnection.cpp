// SPDX-License-Identifier: GPL-2.0
#include "testcheckcloudconnection.h"
#include "core/checkcloudconnection.h"
#include "core/git-access.h"
#include "core/pref.h"

#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

namespace {
const QByteArray validResponse = "HTTP/1.1 418 I'm a teapot\r\nContent-Length: 32\r\nConnection: close\r\n\r\nLinus does not like non-fat milk";
const QByteArray failedResponse = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
}

/** Save global settings and route test requests through a loopback proxy. */
void TestCheckCloudConnection::init()
{
	originalProxy = QNetworkProxy::applicationProxy();
	originalUrl = prefs.cloud_base_url;
	originalTimeout = prefs.cloud_timeout;
	originalLocalOnly = git_local_only;
	prefs.cloud_base_url = "http://" CLOUD_HOST_EU "/";
	prefs.cloud_timeout = 3;
	git_local_only = false;
	response = failedResponse;
	responseDelay = 0;
	liveConnections = 0;
	stall = false;
	requestHosts.clear();
	server = new QTcpServer(this);
	QVERIFY2(server->listen(QHostAddress::LocalHost), qPrintable(server->errorString()));
	QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", server->serverPort()));
	connect(server, &QTcpServer::newConnection, this, [this] {
		while (auto socket = server->nextPendingConnection()) {
			++liveConnections;
			connect(socket, &QTcpSocket::disconnected, this, [this, socket] {
				--liveConnections;
				socket->deleteLater();
			});
			connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
				auto request = socket->property("request").toByteArray() + socket->readAll();
				socket->setProperty("request", request);
				if (!request.contains("\r\n\r\n") || socket->property("handled").toBool())
					return;
				socket->setProperty("handled", true);
				const auto firstLine = request.left(request.indexOf("\r\n")).split(' ');
				const bool tunnel = firstLine[0] == "CONNECT";
				requestHosts.append(tunnel ? QString::fromLatin1(firstLine[1].split(':').first()) :
					QUrl(QString::fromLatin1(firstLine[1])).host());
				if (stall)
					return;
				// Reject HTTPS tunnels locally too, so fallback never reaches the internet.
				const auto reply = tunnel ? failedResponse : response;
				QTimer::singleShot(responseDelay, socket, [socket, reply] {
					socket->write(reply);
					socket->disconnectFromHost();
				});
			});
		}
	});
}

/** Restore global settings and destroy the local proxy and its connections. */
void TestCheckCloudConnection::cleanup()
{
	delete server;
	server = nullptr;
	QNetworkProxy::setApplicationProxy(originalProxy);
	prefs.cloud_base_url = originalUrl;
	prefs.cloud_timeout = originalTimeout;
	git_local_only = originalLocalOnly;
}

/** Verify connection errors return without waiting out the cloud timeout. */
void TestCheckCloudConnection::refusedConnectionReturnsImmediately()
{
	server->close();
	QElapsedTimer elapsed;
	elapsed.start();
	QVERIFY(!CheckCloudConnection().checkServer());
	QVERIFY(git_local_only);
	QVERIFY2(elapsed.elapsed() < 1000, qPrintable(QString("Failed requests took %1 ms").arg(elapsed.elapsed())));
}

/** Supply valid responses and failures with an incorrect status or body. */
void TestCheckCloudConnection::finishedResponses_data()
{
	QTest::addColumn<QByteArray>("serverResponse");
	QTest::addColumn<bool>("expected");
	QTest::newRow("valid-teapot") << validResponse << true;
	QTest::newRow("wrong-status") << QByteArray("HTTP/1.1 200 OK\r\nContent-Length: 32\r\nConnection: close\r\n\r\nLinus does not like non-fat milk") << false;
	QTest::newRow("wrong-body") << QByteArray("HTTP/1.1 418 I'm a teapot\r\nContent-Length: 5\r\nConnection: close\r\n\r\nwrong") << false;
	QTest::newRow("server-error") << failedResponse << false;
}

/** Verify completed replies are validated promptly and connections close. */
void TestCheckCloudConnection::finishedResponses()
{
	QFETCH(QByteArray, serverResponse);
	QFETCH(bool, expected);
	response = serverResponse;
	QElapsedTimer elapsed;
	elapsed.start();
	QCOMPARE(CheckCloudConnection().checkServer(), expected);
	QCOMPARE(git_local_only, !expected);
	QVERIFY2(elapsed.elapsed() < 1000, qPrintable(QString("Finished requests took %1 ms").arg(elapsed.elapsed())));
	QVERIFY(!requestHosts.isEmpty());
	QCOMPARE(requestHosts.first(), QString(CLOUD_HOST_EU));
	QTRY_COMPARE(liveConnections, 0);
}

/** Verify a pending request can succeed after a progress interval elapses. */
void TestCheckCloudConnection::delayedSuccess()
{
	response = validResponse;
	responseDelay = 1200;
	QElapsedTimer elapsed;
	elapsed.start();
	QVERIFY(CheckCloudConnection().checkServer());
	QVERIFY(!git_local_only);
	QVERIFY(elapsed.elapsed() >= 1000);
	QVERIFY(elapsed.elapsed() < 2500);
	QCOMPARE(requestHosts, QStringList { CLOUD_HOST_EU });
}

/** Verify unanswered requests time out and their connections are released. */
void TestCheckCloudConnection::stalledRequestsTimeout()
{
	prefs.cloud_timeout = 1;
	stall = true;
	QElapsedTimer elapsed;
	elapsed.start();
	QVERIFY(!CheckCloudConnection().checkServer());
	QVERIFY(git_local_only);
	QVERIFY(!requestHosts.isEmpty());
	QVERIFY(elapsed.elapsed() >= requestHosts.size() * 800);
	QVERIFY(elapsed.elapsed() < 7000);
	QCOMPARE(requestHosts.first(), QString(CLOUD_HOST_EU));
	QTRY_COMPARE(liveConnections, 0);
}

QTEST_GUILESS_MAIN(TestCheckCloudConnection)
