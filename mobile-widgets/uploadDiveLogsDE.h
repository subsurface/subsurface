// SPDX-License-Identifier: GPL-2.0
#ifndef UPLOADDIVELOGSDE_H
#define UPLOADDIVELOGSDE_H
#include <QFile>
#include <QNetworkReply>
#include <QTimer>


class uploadDiveLogsDE : public QObject {
	Q_OBJECT
	
public:
	static uploadDiveLogsDE *instance();
	void doUpload(bool selected, const QString &userid, const QString &password);

private slots:
	void updateProgress(qint64 current, qint64 total);
	void uploadFinished();
	void uploadTimeout();
	void uploadError(QNetworkReply::NetworkError error);

private:
	uploadDiveLogsDE() {}

	bool prepareDives(bool selected, QString filename);
	void uploadDives(QFile *dldContent, const QString &userid, const QString &password);

	QNetworkReply *reply;
	QTimer timeout;
};




#ifdef JAN
#include <QTemporaryFile>
#include <QTimer>
#include <libxml/tree.h>

class WebServices : public QObject {
	Q_OBJECT
public:
	explicit WebServices(QWidget *parent = 0, Qt::WindowFlags f = 0);
	void hidePassword();
	void hideUpload();
	void hideDownload();

private
slots:
	virtual void startDownload() = 0;
	virtual void startUpload() = 0;
	virtual void buttonClicked(QAbstractButton *button) = 0;
	void downloadTimedOut();

protected
slots:
	void updateProgress(qint64 current, qint64 total);

protected:
	void resetState();
	void connectSignalsForDownload(QNetworkReply *reply);
	void connectSignalsForUpload();

	Ui::WebServices ui;
	QTimer timeout;
	QByteArray downloadedData;
	QString defaultApplyText;
	QString userAgent;
};

class DivelogsDeWebServices : public WebServices {
	Q_OBJECT
public:
	static DivelogsDeWebServices *instance();
	void prepareDivesForUpload(bool selected);

private
slots:
	void startDownload();
	void saveToZipFile();
	void listDownloadFinished();
	void downloadFinished();
	void uploadFinished();
	void downloadError(QNetworkReply::NetworkError error);
	void uploadError(QNetworkReply::NetworkError error);
	void startUpload();

private:

	void setStatusText(int status);
	void download_dialog_traverse_xml(xmlNodePtr node, unsigned int *download_status);
	unsigned int download_dialog_parse_response(const QByteArray &length);

	QTemporaryFile zipFile;
	bool uploadMode;
};
#endif

#endif // UPLOADDIVELOGSDE_H
