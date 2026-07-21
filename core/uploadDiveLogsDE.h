// SPDX-License-Identifier: GPL-2.0
#ifndef UPLOADDIVELOGSDE_H
#define UPLOADDIVELOGSDE_H
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QTimer>
#include "core/membuffer.h"

class uploadDiveLogsDE : public QObject {
	Q_OBJECT
	
public:
	static uploadDiveLogsDE *instance();
	void doUpload(bool selected, const QString &userid, const QString &password);

private slots:
	void updateProgressSlot(qint64 current, qint64 total);
	void uploadFinishedSlot();
	void uploadTimeoutSlot();
	void uploadErrorSlot(QNetworkReply::NetworkError error);
	void loginFinishedSlot();
	void loginTimeoutSlot();
	void loginErrorSlot(QNetworkReply::NetworkError error);

signals:
	void uploadFinish(bool success, const QString &text);
	void uploadProgress(qreal percentage, qreal total);
	void uploadStatus(const QString &text);
	
private:
	uploadDiveLogsDE();

	void uploadDives(const QString &userid, const QString &password);

	// only to be used in desktop-widgets::subsurfacewebservices
	bool prepareDives(bool selected);

	membuffer mb_json;
	QNetworkReply *reply;
	QHttpMultiPart *multipart;
	QTimer timeout;
};

#endif // UPLOADDIVELOGSDE_H
