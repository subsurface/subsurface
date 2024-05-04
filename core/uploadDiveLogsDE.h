// SPDX-License-Identifier: GPL-2.0
#ifndef UPLOADDIVELOGSDE_H
#define UPLOADDIVELOGSDE_H
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QTimer>
#include <QFile>

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

signals:
	void uploadFinish(bool success, const QString &text);
	void uploadProgress(qreal percentage, qreal total);
	void uploadStatus(const QString &text);
	
private:
	uploadDiveLogsDE();

	void uploadDives(const QString &filename, const QString &userid, const QString &password);
	void cleanupTempFile();

	// only to be used in desktop-widgets::subsurfacewebservices
	bool prepareDives(const QString &tempfile, bool selected);

	QFile tempFile;
	QNetworkReply *reply;
	QHttpMultiPart *multipart;
	QTimer timeout;
};

#endif // UPLOADDIVELOGSDE_H
