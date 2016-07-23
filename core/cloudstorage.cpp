#include "cloudstorage.h"
#include "pref.h"
#include "dive.h"
#include "helpers.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include <QApplication>

CloudStorageAuthenticate::CloudStorageAuthenticate(QObject *parent) :
	QObject(parent),
	reply(NULL)
{
	userAgent = getUserAgent();
}

#define CLOUDURL QString(prefs.cloud_base_url)
#define CLOUDBACKENDSTORAGE CLOUDURL + "/storage"
#define CLOUDBACKENDVERIFY CLOUDURL + "/verify"
#define CLOUDBACKENDUPDATE CLOUDURL + "/update"

QNetworkReply* CloudStorageAuthenticate::backend(const QString& email,const QString& password,const QString& pin,const QString& newpasswd)
{
	QString payload(email + QChar(' ') + password);
	QUrl requestUrl;
	if (pin.isEmpty() && newpasswd.isEmpty()) {
		requestUrl = QUrl(CLOUDBACKENDSTORAGE);
	} else if (!newpasswd.isEmpty()) {
		requestUrl = QUrl(CLOUDBACKENDUPDATE);
		payload += QChar(' ') + newpasswd;
	} else {
		requestUrl = QUrl(CLOUDBACKENDVERIFY);
		payload += QChar(' ') + pin;
	}
	QNetworkRequest *request = new QNetworkRequest(requestUrl);
	request->setRawHeader("Accept", "text/xml, text/plain");
	request->setRawHeader("User-Agent", userAgent.toUtf8());
	request->setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	reply = manager()->post(*request, qPrintable(payload));
	connect(reply, SIGNAL(finished()), this, SLOT(uploadFinished()));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
		SLOT(uploadError(QNetworkReply::NetworkError)));
	return reply;
}

void CloudStorageAuthenticate::uploadFinished()
{
	static QString myLastError;

	QString cloudAuthReply(reply->readAll());
	qDebug() << "Completed connection with cloud storage backend, response" << cloudAuthReply;
	CloudStorageSettings csSettings(parent());

	if (cloudAuthReply == QLatin1String("[VERIFIED]") || cloudAuthReply == QLatin1String("[OK]")) {
		csSettings.setVerificationStatus(CS_VERIFIED);
		/* TODO: Move this to a correct place
		NotificationWidget *nw = MainWindow::instance()->getNotificationWidget();
		if (nw->getNotificationText() == myLastError)
			nw->hideNotification();
		*/
		myLastError.clear();
	} else if (cloudAuthReply == QLatin1String("[VERIFY]")) {
		csSettings.setVerificationStatus(CS_NEED_TO_VERIFY);
	} else if (cloudAuthReply == QLatin1String("[PASSWDCHANGED]")) {
		free(prefs.cloud_storage_password);
		prefs.cloud_storage_password = prefs.cloud_storage_newpassword;
		prefs.cloud_storage_newpassword = NULL;
		emit passwordChangeSuccessful();
		return;
	} else {
		csSettings.setVerificationStatus(CS_INCORRECT_USER_PASSWD);
		myLastError = cloudAuthReply;
		report_error("%s", qPrintable(cloudAuthReply));
		/* TODO: Emit a signal with the error
		MainWindow::instance()->getNotificationWidget()->showNotification(get_error_string(), KMessageWidget::Error);
		*/
	}
	emit finishedAuthenticate();
}

void CloudStorageAuthenticate::uploadError(QNetworkReply::NetworkError)
{
	qDebug() << "Received error response from cloud storage backend:" << reply->errorString();
}

void CloudStorageAuthenticate::sslErrors(QList<QSslError> errorList)
{
	if (verbose) {
		qDebug() << "Received error response trying to set up https connection with cloud storage backend:";
		Q_FOREACH (QSslError err, errorList) {
			qDebug() << err.errorString();
		}
	}
	QSslConfiguration conf = reply->sslConfiguration();
	QSslCertificate cert = conf.peerCertificate();
	QByteArray hexDigest = cert.digest().toHex();
	if (reply->url().toString().contains(prefs.cloud_base_url) &&
	    hexDigest == "13ff44c62996cfa5cd69d6810675490e") {
		if (verbose)
			qDebug() << "Overriding SSL check as I recognize the certificate digest" << hexDigest;
		reply->ignoreSslErrors();
	} else {
		if (verbose)
			qDebug() << "got invalid SSL certificate with hex digest" << hexDigest;
	}
}

QNetworkAccessManager *manager()
{
	static QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	return manager;
}
