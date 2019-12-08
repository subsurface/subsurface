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
}


void uploadDiveShare::slot_updateProgress(qint64 current, qint64 total)
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


void uploadDiveShare::slot_uploadFinished()
{
	reply->deleteLater();
	timeout.stop();
	if (reply->error() != 0)  {
		emit uploadFinish(false, reply->errorString());
	} else {
		emit uploadFinish(true, tr("Upload successful"));
	}
}


void uploadDiveShare::slot_uploadTimeout()
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	QString err(tr("divelogs.de not responding"));
	report_error(err.toUtf8());
	emit uploadFinish(false, err);
}


void uploadDiveShare::slot_uploadError(QNetworkReply::NetworkError error)
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	QString err(tr("network error %1").arg(error));
	report_error(err.toUtf8());
	emit uploadFinish(false, err);
}
