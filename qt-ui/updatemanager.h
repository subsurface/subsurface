#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

class UpdateManager : public QObject {
	Q_OBJECT
public:
	explicit UpdateManager(QObject *parent = 0);
	void checkForUpdates();

public
slots:
	void requestReceived();
};

#endif // UPDATEMANAGER_H
