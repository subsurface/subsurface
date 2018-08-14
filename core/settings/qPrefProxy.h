// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFPROXY_H
#define QPREFPROXY_H
#include "core/pref.h"

#include <QObject>


class qPrefProxy : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool proxy_auth READ proxy_auth WRITE set_proxy_auth NOTIFY proxy_auth_changed);
	Q_PROPERTY(QString proxy_host READ proxy_host WRITE set_proxy_host NOTIFY proxy_host_changed);
	Q_PROPERTY(QString proxy_pass READ proxy_pass WRITE set_proxy_pass NOTIFY proxy_pass_changed);
	Q_PROPERTY(int proxy_port READ proxy_port WRITE set_proxy_port NOTIFY proxy_port_changed);
	Q_PROPERTY(int proxy_type READ proxy_type WRITE set_proxy_type NOTIFY proxy_type_changed);
	Q_PROPERTY(QString proxy_user READ proxy_user WRITE set_proxy_user NOTIFY proxy_user_changed);

public:
	qPrefProxy(QObject *parent = NULL);
	static qPrefProxy *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static bool proxy_auth() { return prefs.proxy_auth; }
	static QString proxy_host() { return prefs.proxy_host; }
	static QString proxy_pass() { return prefs.proxy_pass; }
	static int proxy_port() { return prefs.proxy_port; }
	static int proxy_type() { return prefs.proxy_type; }
	static QString proxy_user() { return prefs.proxy_user; }

public slots:
	static void set_proxy_auth(bool value);
	static void set_proxy_host(const QString &value);
	static void set_proxy_pass(const QString &value);
	static void set_proxy_port(int value);
	static void set_proxy_type(int value);
	static void set_proxy_user(const QString &value);

signals:
	void proxy_auth_changed(bool value);
	void proxy_host_changed(const QString &value);
	void proxy_pass_changed(const QString &value);
	void proxy_port_changed(int value);
	void proxy_type_changed(int value);
	void proxy_user_changed(const QString &value);

private:
	static void disk_proxy_auth(bool doSync);
	static void disk_proxy_host(bool doSync);
	static void disk_proxy_pass(bool doSync);
	static void disk_proxy_port(bool doSync);
	static void disk_proxy_type(bool doSync);
	static void disk_proxy_user(bool doSync);
};

#endif
