// SPDX-License-Identifier: GPL-2.0
// AI-generated (Codex)
#ifndef TESTCLOUDSTORAGEAUTHENTICATE_H
#define TESTCLOUDSTORAGEAUTHENTICATE_H

#include <QTcpServer>
#include <QTemporaryDir>
#include <QtTest>
#include <string>

/** Exercise authentication responses without contacting the cloud service. */
class TestCloudStorageAuthenticate : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	void init();
	void cleanup();
	void connectionRefused_data();
	void connectionRefused();
	void cancelledRequest();
	void serviceFailures_data();
	void serviceFailures();
	void retryAfterServiceFailure();
	void authenticationResponses_data();
	void authenticationResponses();

private:
	QTemporaryDir settings;
	QTcpServer server;
	int responseStatus = 200;
	QByteArray responseBody = "[OK]";
	std::string originalUrl;
	std::string originalPassword;
	int originalStatus = 0;
};
#endif
