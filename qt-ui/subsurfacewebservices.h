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
	explicit WebServices(QWidget *parent = 0, Qt::WindowFlags f = 0);
	void hidePassword();
	void hideUpload();
	void hideDownload();

	static QNetworkAccessManager *manager();

private
slots:
	virtual void startDownload() = 0;
	virtual void startUpload() = 0;
	virtual void buttonClicked(QAbstractButton *button) = 0;
	virtual void downloadTimedOut();

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

class SubsurfaceWebServices : public WebServices {
	Q_OBJECT
public:
	explicit SubsurfaceWebServices(QWidget *parent = 0, Qt::WindowFlags f = 0);

private
slots:
	void startDownload();
	void buttonClicked(QAbstractButton *button);
	void downloadFinished();
	void downloadError(QNetworkReply::NetworkError error);
	void startUpload()
	{
	} /*no op*/
private:
	void setStatusText(int status);
	void download_dialog_traverse_xml(xmlNodePtr node, unsigned int *download_status);
	unsigned int download_dialog_parse_response(const QByteArray &length);
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
	void uploadFinished();
	void downloadError(QNetworkReply::NetworkError error);
	void uploadError(QNetworkReply::NetworkError error);
	void startUpload();

private:
	void uploadDives(QIODevice *dldContent);
	explicit DivelogsDeWebServices(QWidget *parent = 0, Qt::WindowFlags f = 0);
	void setStatusText(int status);
	bool prepare_dives_for_divelogs(const QString &filename, bool selected);
	void download_dialog_traverse_xml(xmlNodePtr node, unsigned int *download_status);
	unsigned int download_dialog_parse_response(const QByteArray &length);

	QHttpMultiPart *multipart;
	QTemporaryFile zipFile;
	bool uploadMode;
};

class UserSurveyServices : public WebServices {
	Q_OBJECT
public:
	QNetworkReply* sendSurvey(QString values);
	explicit UserSurveyServices(QWidget *parent = 0, Qt::WindowFlags f = 0);
private
slots:
	// need to declare them as no ops or Qt4 is unhappy
	virtual void startDownload() { }
	virtual void startUpload() { }
	virtual void buttonClicked(QAbstractButton *button) { }
};

class CloudStorageAuthenticate : public QObject {
	Q_OBJECT
public:
	QNetworkReply* authenticate(QString email, QString password, QString pin = "");
	explicit CloudStorageAuthenticate(QObject *parent);
signals:
	void finishedAuthenticate();
private
slots:
	void uploadError(QNetworkReply::NetworkError error);
	void sslErrors(QList<QSslError> errorList);
	void uploadFinished();
private:
	QNetworkReply *reply;
	QString userAgent;

};

class CheckCloudConnection : public QObject {
	Q_OBJECT
public:
	explicit CheckCloudConnection(QObject *parent = 0);
	static bool checkServer();
};

#ifdef __cplusplus
extern "C" {
#endif
extern void set_save_userid_local(short value);
extern void set_userid(char *user_id);
#ifdef __cplusplus
}
#endif

#endif // SUBSURFACEWEBSERVICES_H
