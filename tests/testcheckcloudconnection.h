// SPDX-License-Identifier: GPL-2.0
#ifndef TESTCHECKCLOUDCONNECTION_H
#define TESTCHECKCLOUDCONNECTION_H

#include <QtTest>
#include <QNetworkProxy>
#include <string>

class QTcpServer;

class TestCheckCloudConnection : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();
	void refusedConnectionReturnsImmediately();
	void finishedResponses_data();
	void finishedResponses();
	void delayedSuccess();
	void stalledRequestsTimeout();
private:
	QTcpServer *server = nullptr;
	QNetworkProxy originalProxy;
	std::string originalUrl;
	int originalTimeout;
	bool originalLocalOnly;
	QByteArray response;
	QStringList requestHosts;
	int responseDelay = 0;
	int liveConnections = 0;
	bool stall = false;
};

#endif
