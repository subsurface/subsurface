// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPROXY_H
#define QPREFPROXY_H

#include <QObject>


class qPrefProxy : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool		auth
				READ	auth
				WRITE	setAuth
				NOTIFY  authChanged);
	Q_PROPERTY(QString	host
				READ	host
				WRITE	setHost
				NOTIFY  hostChanged);
	Q_PROPERTY(QString	pass
				READ	pass
				WRITE	setPass
				NOTIFY  passChanged);
	Q_PROPERTY(int		port
				READ	port
				WRITE	setPort
				NOTIFY  portChanged);
	Q_PROPERTY(int		type
				READ	type
				WRITE	setType
				NOTIFY  typeChanged);
	Q_PROPERTY(QString	user
				READ	user
				WRITE	setUser
				NOTIFY  userChanged);

public:
	qPrefProxy(QObject *parent = NULL) : QObject(parent) {};
	~qPrefProxy() {};
	static qPrefProxy *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	bool auth() const;
	const QString host() const;
	const QString pass() const;
	int port() const;
	int type() const;
	const QString user() const;

public slots:
	void setAuth(bool value);
	void setHost(const QString& value);
	void setPass(const QString& value);
	void setPort(int value);
	void setType(int value);
	void setUser(const QString& value);

signals:
	void authChanged(bool value);
	void hostChanged(const QString& value);
	void passChanged(const QString& value);
	void portChanged(int value);
	void typeChanged(int value);
	void userChanged(const QString& value);

private:
	const QString group = QStringLiteral("Network");
	static qPrefProxy *m_instance;

	void diskAuth(bool doSync);
	void diskHost(bool doSync);
	void diskPass(bool doSync);
	void diskPort(bool doSync);
	void diskType(bool doSync);
	void diskUser(bool doSync);
};

#endif
