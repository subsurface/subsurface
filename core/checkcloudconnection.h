// SPDX-License-Identifier: GPL-2.0
#ifndef CHECKCLOUDCONNECTION_H
#define CHECKCLOUDCONNECTION_H

#include <QObject>
#include <QNetworkReply>
#include <QSsl>

class CheckCloudConnection : public QObject {
	Q_OBJECT
public:
	CheckCloudConnection(const QString &baseUrl, QObject *parent = 0);
	bool checkServer();
private:
	QString baseUrl;
	QNetworkReply *reply;
private
slots:
	void sslErrors(QList<QSslError> errorList);
};

#endif // CHECKCLOUDCONNECTION_H
