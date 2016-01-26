#ifndef CLOUD_STORAGE_H
#define CLOUD_STORAGE_H

#include <QObject>
#include <QNetworkReply>

class CloudStorageAuthenticate : public QObject {
	Q_OBJECT
public:
	QNetworkReply* backend(QString email, QString password, QString pin = "", QString newpasswd = "");
	explicit CloudStorageAuthenticate(QObject *parent);
signals:
	void finishedAuthenticate();
	void passwordChangeSuccessful();
private
slots:
	void uploadError(QNetworkReply::NetworkError error);
	void sslErrors(QList<QSslError> errorList);
	void uploadFinished();
private:
	QNetworkReply *reply;
	QString userAgent;
	bool verbose;
};

QNetworkAccessManager *manager();
#endif