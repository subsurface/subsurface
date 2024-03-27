// SPDX-License-Identifier: GPL-2.0
#include "cloudstorage.h"
#include "pref.h"
#include "qthelper.h"
#include "errorhelper.h"
#include "settings/qPrefCloudStorage.h"
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
#define CLOUDBACKENDDELETE CLOUDURL + "/delete-account"

QNetworkReply* CloudStorageAuthenticate::backend(const QString& email,const QString& password,const QString& pin,const QString& newpasswd)
{
	QString payload(email + QChar(' ') + password);
	QUrl requestUrl;
	if (pin.isEmpty() && newpasswd.isEmpty()) {
		requestUrl = QUrl(CLOUDBACKENDSTORAGE);
	} else if (!newpasswd.isEmpty()) {
		requestUrl = QUrl(CLOUDBACKENDUPDATE);
		payload += QChar(' ') + newpasswd;
		cloudNewPassword = newpasswd;
	} else {
		requestUrl = QUrl(CLOUDBACKENDVERIFY);
		payload += QChar(' ') + pin;
	}
	QNetworkRequest *request = new QNetworkRequest(requestUrl);
	request->setRawHeader("Accept", "text/xml, text/plain");
	request->setRawHeader("User-Agent", userAgent.toUtf8());
	request->setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	reply = manager()->post(*request, qPrintable(payload));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	connect(reply, &QNetworkReply::finished, this, &CloudStorageAuthenticate::uploadFinished);
	connect(reply, &QNetworkReply::sslErrors, this, &CloudStorageAuthenticate::sslErrors);
	connect(reply, &QNetworkReply::errorOccurred, this, &CloudStorageAuthenticate::uploadError);
#else
	connect(reply, SIGNAL(finished()), this, SLOT(uploadFinished()));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
		SLOT(uploadError(QNetworkReply::NetworkError)));
#endif
	return reply;
}

QNetworkReply* CloudStorageAuthenticate::deleteAccount(const QString& email, const QString& password)
{
	QString payload(email + QChar(' ') + password);
	QNetworkRequest *request = new QNetworkRequest(QUrl(CLOUDBACKENDDELETE));
	request->setRawHeader("Accept", "text/xml, text/plain");
	request->setRawHeader("User-Agent", userAgent.toUtf8());
	request->setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	reply = manager()->post(*request, qPrintable(payload));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	connect(reply, &QNetworkReply::finished, this, &CloudStorageAuthenticate::deleteFinished);
	connect(reply, &QNetworkReply::sslErrors, this, &CloudStorageAuthenticate::sslErrors);
	connect(reply, &QNetworkReply::errorOccurred, this, &CloudStorageAuthenticate::uploadError);
#else
	connect(reply, SIGNAL(finished()), this, SLOT(deleteFinished()));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
		SLOT(uploadError(QNetworkReply::NetworkError)));
#endif
	return reply;
}

void CloudStorageAuthenticate::deleteFinished()
{
	std::string cloudAuthReply = reply->readAll().toStdString();
	report_info("Completed connection with cloud storage backend, response %s", cloudAuthReply.c_str());
	emit finishedDelete();
}

void CloudStorageAuthenticate::uploadFinished()
{
	static std::string myLastError;

	std::string cloudAuthReply = reply->readAll().toStdString();
	report_info("Completed connection with cloud storage backend, response %s", cloudAuthReply.c_str());

	if (cloudAuthReply == "[VERIFIED]" || cloudAuthReply == "[OK]") {
		qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_VERIFIED);
		/* TODO: Move this to a correct place
		NotificationWidget *nw = MainWindow::instance()->getNotificationWidget();
		if (nw->getNotificationText().toStdString() == myLastError)
			nw->hideNotification();
		*/
		myLastError.clear();
	} else if (cloudAuthReply == "[VERIFY]" || cloudAuthReply == "Invalid PIN") {
		qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_NEED_TO_VERIFY);
		report_error("%s", qPrintable(tr("Cloud account verification required, enter PIN in preferences")));
	} else if (cloudAuthReply == "[PASSWDCHANGED]") {
		qPrefCloudStorage::set_cloud_storage_password(cloudNewPassword);
		cloudNewPassword.clear();
		emit passwordChangeSuccessful();
		return;
	} else {
		qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_INCORRECT_USER_PASSWD);
		myLastError = cloudAuthReply;
		report_error("%s", cloudAuthReply.c_str());
	}
	emit finishedAuthenticate();
}

void CloudStorageAuthenticate::uploadError(QNetworkReply::NetworkError)
{
	report_info("Received error response from cloud storage backend: %s", qPrintable(reply->errorString()));
}

void CloudStorageAuthenticate::sslErrors(const QList<QSslError> &errorList)
{
	if (verbose) {
		report_info("Received error response trying to set up https connection with cloud storage backend:");
		for (QSslError err: errorList) {
			report_info("%s", qPrintable(err.errorString()));
		}
	}
	QSslConfiguration conf = reply->sslConfiguration();
	QSslCertificate cert = conf.peerCertificate();
	QByteArray hexDigest = cert.digest().toHex();
	report_info("got invalid SSL certificate with hex digest %s", qPrintable(hexDigest));
}

QNetworkAccessManager *manager()
{
	static QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	return manager;
}
