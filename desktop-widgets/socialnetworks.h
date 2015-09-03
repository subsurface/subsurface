#ifndef FACEBOOKMANAGER_H
#define FACEBOOKMANAGER_H

#include <QObject>
#include <QUrl>
#include <QDialog>

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

namespace  Ui {
	class SocialnetworksDialog;
}

class SocialNetworkDialog : public QDialog {
	Q_OBJECT
public:
	SocialNetworkDialog(QWidget *parent);
	QString text() const;
	QString album() const;
public slots:
	void selectionChanged();
	void albumChanged();
private:
	Ui::SocialnetworksDialog *ui;
};
#endif // FACEBOOKMANAGER_H
