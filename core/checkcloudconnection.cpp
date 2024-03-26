// SPDX-License-Identifier: GPL-2.0
#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QHostAddress>

#include "pref.h"
#include "qthelper.h"
#include "git-access.h"
#include "errorhelper.h"
#include "core/format.h"
#include "core/subsurface-string.h"
#include "core/membuffer.h"
#include "core/settings/qPrefCloudStorage.h"

#include "checkcloudconnection.h"

CheckCloudConnection::CheckCloudConnection(QObject *parent) :
	QObject(parent),
	reply(0)
{

}

// two free APIs to figure out where we are
#define GET_EXTERNAL_IP_API "http://api.ipify.org"
#define GET_CONTINENT_API "http://ip-api.com/line/%1?fields=continent"

// our own madeup API to make sure we are talking to a Subsurface cloud server
#define TEAPOT "/make-latte?number-of-shots=3"
#define HTTP_I_AM_A_TEAPOT 418
#define MILK "Linus does not like non-fat milk"
bool CheckCloudConnection::checkServer()
{
	if (verbose)
		report_info("Checking cloud connection...");

	QEventLoop loop;
	QNetworkAccessManager *mgr = new QNetworkAccessManager();
	do {
		QNetworkRequest request;
		request.setRawHeader("Accept", "text/plain");
		request.setRawHeader("User-Agent", getUserAgent().toUtf8());
		request.setUrl(QString(prefs.cloud_base_url) + TEAPOT);
		reply = mgr->get(request);
		QTimer timer;
		timer.setSingleShot(true);
		connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
		connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		connect(reply, &QNetworkReply::sslErrors, this, &CheckCloudConnection::sslErrors);
		for (int seconds = 1; seconds <= prefs.cloud_timeout; seconds++) {
			timer.start(1000); // wait the given number of seconds (default 5)
			loop.exec();
			if (timer.isActive()) {
				// didn't time out, did we get the right response?
				timer.stop();
				if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == HTTP_I_AM_A_TEAPOT &&
						reply->readAll() == QByteArray(MILK)) {
					reply->deleteLater();
					mgr->deleteLater();
					if (verbose)
						qWarning() << "Cloud storage: successfully checked connection to cloud server";
					return true;
				}
			} else if (seconds < prefs.cloud_timeout) {
				QString text = tr("Waiting for cloud connection (%n second(s) passed)", "", seconds);
				git_storage_update_progress(qPrintable(text));
			} else {
				disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
				reply->abort();
			}
		}
		if (verbose)
			report_info("connection test to cloud server %s failed %d %s %d %s", prefs.cloud_base_url,
				    static_cast<int>(reply->error()), qPrintable(reply->errorString()),
				    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
				    qPrintable(reply->readAll()));
	} while (nextServer());
	// if none of the servers was reachable, update the user and switch to git_local_only
	git_storage_update_progress(qPrintable(tr("Cloud connection failed")));
	git_local_only = true;
	reply->deleteLater();
	mgr->deleteLater();
	if (verbose)
		qWarning() << "Cloud storage: unable to connect to cloud server";
	return false;
}

void CheckCloudConnection::sslErrors(const QList<QSslError> &errorList)
{
	report_info("Received error response trying to set up https connection with cloud storage backend:");
	for (QSslError err: errorList)
		report_info("%s", qPrintable(err.errorString()));
}

bool CheckCloudConnection::nextServer()
{
	struct serverTried {
		const char *server;
		bool tried;
	};
	static struct serverTried cloudServers[] = {
		{ CLOUD_HOST_EU, false },
		{ CLOUD_HOST_US, false },
		{ CLOUD_HOST_E2, false },
		{ CLOUD_HOST_U2, false }
	};
	const char *server = nullptr;
	for (serverTried &item: cloudServers) {
		if (strstr(prefs.cloud_base_url, item.server))
			item.tried = true;
		else if (item.tried == false)
			server = item.server;
	}
	if (server) {
		int s = strlen(server);
		char *baseurl = (char *)malloc(10 + s);
		strcpy(baseurl, "https://");
		strncat(baseurl, server, s);
		strcat(baseurl, "/");
		report_info("failed to connect to %s next server to try: %s", prefs.cloud_base_url, baseurl);
		prefs.cloud_base_url = baseurl;
		git_storage_update_progress(qPrintable(tr("Trying different cloud server...")));
		return true;
	}
	report_info("failed to connect to any of the Subsurface cloud servers, giving up");
	return false;
}

void CheckCloudConnection::pickServer()
{
	QNetworkRequest request(QString(GET_EXTERNAL_IP_API));
	request.setRawHeader("Accept", "text/plain");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QNetworkAccessManager *mgr = new QNetworkAccessManager();
	connect(mgr, &QNetworkAccessManager::finished, this, &CheckCloudConnection::gotIP);
	mgr->get(request);
}

void CheckCloudConnection::gotIP(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		// whatever, just use the default host
		if (verbose)
			report_info("%s got error reply from ip webservice - not changing cloud host", __func__);
		return;
	}
	QString addressString = reply->readAll();
	// use the QHostAddress constructor as a convenient way to validate that this is indeed an IP address
	// but then don't do annything with the QHostAdress - we need the address string...
	QHostAddress addr(addressString);
	if (addr.isNull()) {
		// this isn't an address, don't try to update the cloud host
		if (verbose)
			report_info("%s returned address doesn't appear to be valid (%s) - not changing cloud host", __func__, qPrintable(addressString));
		return;
	}
	if (verbose)
		report_info("IP used for cloud server access %s", qPrintable(addressString));
	// now figure out which continent we are on
	QNetworkRequest request(QString(GET_CONTINENT_API).arg(addressString));
	request.setRawHeader("Accept", "text/plain");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	QNetworkAccessManager *mgr = new QNetworkAccessManager();
	connect(mgr, &QNetworkAccessManager::finished, this, &CheckCloudConnection::gotContinent);
	mgr->get(request);
}

void CheckCloudConnection::gotContinent(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		// whatever, just use the default host
		if (verbose)
			report_info("%s got error reply from ip location webservice - not changing cloud host", __func__);
		return;
	}
	QString continentString = reply->readAll();
	// in most cases this response comes back too late for us - we may already have
	// started to talk to the cloud server (this certinaly seems to be the case when
	// we use the cloud storage as default file). So instead of potentially changing
	// the server that is used in mid connection, let's just update what's stored in
	// our settings so the next time we'll use the server that's closer.

	// of course, right now the logic for that is very simplistic. Use the US server
	// when in the Americas, the EU server otherwise. This may need a better algorithm
	// at some point, but for now it seems good enough

	const char *base_url;
	if (continentString.contains("America", Qt::CaseInsensitive))
		base_url = "https://" CLOUD_HOST_US "/";
	else
		base_url = "https://" CLOUD_HOST_EU "/";
	if (!same_string(base_url, prefs.cloud_base_url)) {
		if (verbose)
			report_info("remember cloud server %s based on IP location in %s", base_url, qPrintable(continentString));
		qPrefCloudStorage::instance()->store_cloud_base_url(base_url);
	}
}

// helper to be used from C code
extern "C" bool canReachCloudServer(struct git_info *info)
{
	if (verbose)
		qWarning() << "Cloud storage: checking connection to cloud server" << info->url.c_str();
	bool connection = CheckCloudConnection().checkServer();
	if (info->url.find(prefs.cloud_base_url) == std::string::npos) {
		// we switched the cloud URL - likely because we couldn't reach the server passed in
		// the strstr with the offset is designed so we match the right component in the name;
		// the cloud_base_url ends with a '/', so we need the text starting at "git/..."
		size_t pos = info->url.find("org/git/");
		if (pos != std::string::npos) {
			info->url = format_string_std("%s%s", prefs.cloud_base_url, info->url.c_str() + pos + 4);
			if (verbose)
				report_info("updating remote to: %s", info->url.c_str());
		}
	}
	return connection;
}
