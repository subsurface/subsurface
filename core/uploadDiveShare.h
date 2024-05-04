// SPDX-License-Identifier: GPL-2.0
#ifndef UPLOADDIVESHARE_H
#define UPLOADDIVESHARE_H
#include <QNetworkReply>
#include <QTimer>

class uploadDiveShare : public QObject {
	Q_OBJECT
	
public:
	static uploadDiveShare *instance();
	void doUpload(bool selected, const QString &uid, bool noPublic);

private slots:
	void updateProgressSlot(qint64 current, qint64 total);
	void uploadFinishedSlot();
	void uploadTimeoutSlot();
	void uploadErrorSlot(QNetworkReply::NetworkError error);

signals:
	void uploadFinish(bool success, const QString &text, const QByteArray &html);
	void uploadProgress(qreal percentage, qreal total);
	void uploadStatus(const QString &text);
	
private:
	uploadDiveShare();

	QNetworkReply *reply;
	QTimer timeout;
};

#endif // UPLOADDIVESHARE_H
