// SPDX-License-Identifier: GPL-2.0
#ifndef UPLOADDIVELOGSDE_H
#define UPLOADDIVELOGSDE_H
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QTimer>


class uploadDiveLogsDE : public QObject {
	Q_OBJECT
	
public:
	static uploadDiveLogsDE *instance();
	void doUpload(bool selected, const QString &userid, const QString &password);

	// only to be used in desktop-widgets::subsurfacewebservices
	bool prepareDives(const QString &tempfile, const bool selected);

private slots:
	void updateProgress(qint64 current, qint64 total);
	void uploadFinished();
	void uploadTimeout();
	void uploadError(QNetworkReply::NetworkError error);

signals:
	void uploadFinish(bool success, const QString &text);
	void uploadProgress(qreal percentage);
	
private:
	uploadDiveLogsDE();

	void uploadDives(const QString &filename, const QString &userid, const QString &password);

	QNetworkReply *reply;
	QHttpMultiPart *multipart;
	QTimer timeout;
};
#endif // UPLOADDIVELOGSDE_H
