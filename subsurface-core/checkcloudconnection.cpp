#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

#include "pref.h"
#include "helpers.h"

#include "checkcloudconnection.h"


CheckCloudConnection::CheckCloudConnection(QObject *parent) :
	QObject(parent),
	reply(0)
{

}

#define TEAPOT "/make-latte?number-of-shots=3"
#define HTTP_I_AM_A_TEAPOT 418
#define MILK "Linus does not like non-fat milk"
bool CheckCloudConnection::checkServer()
{
	QTimer timer;
	timer.setSingleShot(true);
	QEventLoop loop;
	QNetworkRequest request;
	request.setRawHeader("Accept", "text/plain");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	request.setUrl(QString(prefs.cloud_base_url) + TEAPOT);
	QNetworkAccessManager *mgr = new QNetworkAccessManager();
	reply = mgr->get(request);
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
	timer.start(5000); // wait five seconds
	loop.exec();
	if (timer.isActive()) {
		// didn't time out, did we get the right response?
		timer.stop();
		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == HTTP_I_AM_A_TEAPOT &&
		    reply->readAll() == QByteArray(MILK)) {
			reply->deleteLater();
			mgr->deleteLater();
			if (verbose > 1)
				qWarning() << "Cloud storage: successfully checked connection to cloud server";
			return true;
		}
	} else {
		disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		reply->abort();
	}
	if (verbose)
		qDebug() << "connection test to cloud server failed" <<
			    reply->error() << reply->errorString() <<
			    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<
			    reply->readAll();
	reply->deleteLater();
	mgr->deleteLater();
	if (verbose)
		qWarning() << "Cloud storage: unable to connect to cloud server";
	return false;
}

void CheckCloudConnection::sslErrors(QList<QSslError> errorList)
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

// helper to be used from C code
extern "C" bool canReachCloudServer()
{
	if (verbose)
		qWarning() << "Cloud storage: checking connection to cloud server";
	CheckCloudConnection *checker = new CheckCloudConnection;
	return checker->checkServer();
}
