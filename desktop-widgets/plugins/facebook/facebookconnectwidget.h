// SPDX-License-Identifier: GPL-2.0
#ifndef FACEBOOKCONNECTWIDGET_H
#define FACEBOOKCONNECTWIDGET_H

#include <QDialog>
#include <QUrl>
#ifdef USE_WEBENGINE
class QWebEngineView;
#else
class QWebView;
#endif
class QNetworkReply;
class QNetworkAccessManager;

namespace Ui {
	class FacebookConnectWidget;
	class SocialnetworksDialog;
}

struct FacebookInfo {
    enum Size {SMALL, MEDIUM, BIG};

    QString bodyText;
    QString albumId;
    Size profileSize;
    QPixmap profileData;
};

class FacebookManager : public QObject
{
	Q_OBJECT
public:
	static FacebookManager *instance();
	void requestAlbumId();
	void requestUserId();
	QUrl connectUrl();
	QUrl albumListUrl();
	bool loggedIn();
	QPixmap grabProfilePixmap();
signals:
	void justLoggedIn(bool triggererd);
	void justLoggedOut(bool triggered);
	void albumIdReceived(const QString& albumId);
	void sendDiveFinished();

public slots:
	void tryLogin(const QUrl& loginResponse);
	void logout();
	void sendDiveInit();
	void sendDiveToAlbum(const QString& album);

	void uploadFinished();
	void albumListReceived();
	void userIdReceived();
	void createFacebookAlbum();
	void facebookAlbumCreated();
private:
	explicit FacebookManager(QObject *parent = 0);
	QString albumName;
	FacebookInfo fbInfo;
	QNetworkAccessManager *manager;
};


class FacebookConnectWidget : public QDialog {
	Q_OBJECT
public:
	explicit FacebookConnectWidget(QWidget* parent = 0);
	void facebookLoggedIn();
	void facebookDisconnect();
private:
	Ui::FacebookConnectWidget *ui;
#ifdef USE_WEBENGINE
	QWebEngineView *facebookWebView;
#else
	QWebView *facebookWebView;
#endif
};

class SocialNetworkDialog : public QDialog {
	Q_OBJECT
public:

	SocialNetworkDialog(QWidget *parent = 0);
	QString text() const;
	QString album() const;
	FacebookInfo::Size profileSize() const;

public slots:
	void selectionChanged();
	void albumChanged();
private:
	Ui::SocialnetworksDialog *ui;
};

#endif
