// SPDX-License-Identifier: GPL-2.0
#ifndef CHECKCLOUDCONNECTION_H
#define CHECKCLOUDCONNECTION_H

#include <QObject>
#include <QNetworkReply>
#include <QSsl>

class CheckCloudConnection : public QObject {
	Q_OBJECT
public:
	CheckCloudConnection(QObject *parent = 0);
	bool checkServer();
	void pickServer();
private:
	QNetworkReply *reply;
	bool nextServer();
private
slots:
	void sslErrors(const QList<QSslError> &errorList);
	void gotIP(QNetworkReply *reply);
	void gotContinent(QNetworkReply *reply);
};

#endif // CHECKCLOUDCONNECTION_H
