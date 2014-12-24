#ifndef FACEBOOKMANAGER_H
#define FACEBOOKMANAGER_H

#include <QObject>
#include <QUrl>

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
	void justLoggedIn();
	void justLoggedOut();

public slots:
	void tryLogin(const QUrl& loginResponse);
	void logout();
	void setDesiredAlbumName(const QString& albumName);

private:
	explicit FacebookManager(QObject *parent = 0);
	QString albumName;
};

#endif // FACEBOOKMANAGER_H
