// SPDX-License-Identifier: GPL-2.0
#ifndef CLOUD_STORAGE_H
#define CLOUD_STORAGE_H

#include <QObject>
#include <QNetworkReply>

class CloudStorageAuthenticate : public QObject {
	Q_OBJECT
public:
	QNetworkReply* backend(const QString& email,const QString& password,const QString& pin = QString(),const QString& newpasswd = QString());
	QNetworkReply* deleteAccount(const QString& email, const QString &passwd);
	explicit CloudStorageAuthenticate(QObject *parent);
signals:
	void finishedAuthenticate();
	void finishedDelete();
	void passwordChangeSuccessful();
private
slots:
	void uploadError(QNetworkReply::NetworkError error);
	void sslErrors(const QList<QSslError> &errorList);
	void uploadFinished();
	void deleteFinished();
private:
	QNetworkReply *reply;
	QString userAgent;
	QString cloudNewPassword;
};

QNetworkAccessManager *manager();
#endif
