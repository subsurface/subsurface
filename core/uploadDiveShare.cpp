// SPDX-License-Identifier: GPL-2.0
#include "uploadDiveShare.h"
#include <QDebug>
#include "core/membuffer.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/qthelper.h"
#include "core/cloudstorage.h"
#include "core/save-html.h"
#include "core/errorhelper.h"


uploadDiveShare *uploadDiveShare::instance()
{
	static uploadDiveShare *self = new uploadDiveShare;

	return self;
}


uploadDiveShare::uploadDiveShare():
	reply(NULL)
{
	timeout.setSingleShot(true);
}


void uploadDiveShare::doUpload(bool selected, const QString &uid, bool noPublic)
{
	//generate json
	struct membufferpp buf;
	export_list(&buf, NULL, selected, false);
	QByteArray json_data(buf.buffer, buf.len);

	//Request to server
	QNetworkRequest request;
	if (noPublic)
		request.setUrl(QUrl("http://dive-share.appspot.com/upload?private=true"));
	else
		request.setUrl(QUrl("http://dive-share.appspot.com/upload"));
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	if (uid.length() != 0)
		request.setRawHeader("X-UID", uid.toUtf8());

	// Execute async.
	reply = manager()->put(request, json_data);

	// connect signals from upload process
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	connect(reply, &QNetworkReply::finished, this, &uploadDiveShare::uploadFinishedSlot);
	connect(reply, &QNetworkReply::errorOccurred, this, &uploadDiveShare::uploadErrorSlot);
	connect(reply, &QNetworkReply::uploadProgress, this, &uploadDiveShare::updateProgressSlot);
	connect(&timeout, &QTimer::timeout, this, &uploadDiveShare::uploadTimeoutSlot);
#else
	connect(reply, SIGNAL(finished()), this, SLOT(uploadFinishedSlot()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
			SLOT(uploadErrorSlot(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this,
			SLOT(updateProgressSlot(qint64, qint64)));
	connect(&timeout, SIGNAL(timeout()), this, SLOT(uploadTimeoutSlot()));
#endif
	timeout.start(30000); // 30s
}


void uploadDiveShare::updateProgressSlot(qint64 current, qint64 total)
{
	if (!reply)
		return;
	if (total <= 0 || current <= 0)
		return;

	// Calculate percentage
	// And signal whoever wants to know
	qreal percentage = (float)current / (float)total;
	emit uploadProgress(percentage, 1.0);

	// reset the timer: 30 seconds after we last got any data
	timeout.start();
}


void uploadDiveShare::uploadFinishedSlot()
{
	QByteArray html = reply->readAll();
	reply->deleteLater();
	timeout.stop();
	if (reply->error() != 0)  {
		emit uploadFinish(false, reply->errorString(), html);
	} else {
		emit uploadFinish(true, tr("Upload successful"), html);
	}
}


void uploadDiveShare::uploadTimeoutSlot()
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	QString err(tr("dive-share.com not responding"));
	report_error("%s", qPrintable(err));
	emit uploadFinish(false, err, QByteArray());
}


void uploadDiveShare::uploadErrorSlot(QNetworkReply::NetworkError error)
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	QString err(tr("network error %1").arg(error));
	report_error("%s", qPrintable(err));
	emit uploadFinish(false, err, QByteArray());
}
