#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

#include "pref.h"
#include "helpers.h"

#include "checkcloudconnection.h"


CheckCloudConnection::CheckCloudConnection(QObject *parent) : QObject(parent)
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
	QNetworkReply *reply = mgr->get(request);
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	timer.start(2000); // wait two seconds
	loop.exec();
	if (timer.isActive()) {
		// didn't time out, did we get the right response?
		timer.stop();
		if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == HTTP_I_AM_A_TEAPOT &&
		    reply->readAll() == QByteArray(MILK)) {
			reply->deleteLater();
			mgr->deleteLater();
			return true;
		}
		// qDebug() << "did not get expected response - server unreachable" <<
		//	    reply->error() << reply->errorString() <<
		//	    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<
		//	    reply->readAll();
	} else {
		disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
		reply->abort();
	}
	reply->deleteLater();
	mgr->deleteLater();
	return false;
}

// helper to be used from C code
extern "C" bool canReachCloudServer()
{
	return CheckCloudConnection::checkServer();
}
