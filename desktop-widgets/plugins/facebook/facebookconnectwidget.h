#ifndef FACEBOOKCONNECTWIDGET_H
#define FACEBOOKCONNECTWIDGET_H

#include <QDialog>
class QWebView;
namespace Ui {
  class FacebookConnectWidget;
  class SocialnetworksDialog;
}

class FacebookManager : public QObject
{
	Q_OBJECT
public:
	static FacebookManager *instance();
	void requestAlbumId();
	void requestUserId();
	void sync();
	QUrl connectUrl();
	bool loggedIn();
signals:
	void justLoggedIn(bool triggererd);
	void justLoggedOut(bool triggered);

public slots:
	void tryLogin(const QUrl& loginResponse);
	void logout();
	void setDesiredAlbumName(const QString& albumName);
	void sendDive();

private:
	explicit FacebookManager(QObject *parent = 0);
	QString albumName;
};


class FacebookConnectWidget : public QDialog {
  Q_OBJECT
public:
	explicit FacebookConnectWidget(QWidget* parent = 0);
	void facebookLoggedIn();
	void facebookDisconnect();
private:
	Ui::FacebookConnectWidget *ui;
	QWebView *facebookWebView;
};

class SocialNetworkDialog : public QDialog {
	Q_OBJECT
public:
	SocialNetworkDialog(QWidget *parent = 0);
	QString text() const;
	QString album() const;
public slots:
	void selectionChanged();
	void albumChanged();
private:
	Ui::SocialnetworksDialog *ui;
};

#endif