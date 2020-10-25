// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACEWEBSERVICES_H
#define SUBSURFACEWEBSERVICES_H

#include <QDialog>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <QTimer>
#include <libxml/tree.h>

#include "ui_webservices.h"

class QAbstractButton;
class QHttpMultiPart;

class WebServices : public QDialog {
	Q_OBJECT
public:
	explicit WebServices(QWidget *parent = 0);
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
	QNetworkReply *reply;
	QTimer timeout;
	QByteArray downloadedData;
	QString defaultApplyText;
	QString userAgent;
};

class DivelogsDeWebServices : public WebServices {
	Q_OBJECT
public:
	static DivelogsDeWebServices *instance();
	void downloadDives();
	void prepareDivesForUpload(bool selected);

private
slots:
	void startDownload();
	void buttonClicked(QAbstractButton *button);
	void saveToZipFile();
	void listDownloadFinished();
	void downloadFinished();
	void uploadFinished(bool success, const QString &text);
	void downloadError(QNetworkReply::NetworkError error);
	void startUpload();
	void updateProgress(qreal current, qreal total);
	void uploadStatus(const QString &text);

private:
	explicit DivelogsDeWebServices(QWidget *parent = 0);
	void setStatusText(int status);
	void download_dialog_traverse_xml(xmlNodePtr node, unsigned int *download_status);
	unsigned int download_dialog_parse_response(const QByteArray &length);

	QTemporaryFile zipFile;
	bool uploadMode;
	bool useSelectedDives;
};

#endif // SUBSURFACEWEBSERVICES_H
