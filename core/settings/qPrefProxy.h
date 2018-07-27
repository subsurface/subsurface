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
	void loadSync(bool doSync);
	void inline load() { loadSync(false); }
	void inline sync() { loadSync(true); }

public:
	static inline bool proxy_auth() { return prefs.proxy_auth; }
	static inline QString proxy_host() { return prefs.proxy_host; }
	static inline QString proxy_pass() { return prefs.proxy_pass; }
	static inline int proxy_port() { return prefs.proxy_port; }
	static inline int proxy_type() { return prefs.proxy_type; }
	static inline QString proxy_user() { return prefs.proxy_user; }

public slots:
	void set_proxy_auth(bool value);
	void set_proxy_host(const QString &value);
	void set_proxy_pass(const QString &value);
	void set_proxy_port(int value);
	void set_proxy_type(int value);
	void set_proxy_user(const QString &value);

signals:
	void proxy_auth_changed(bool value);
	void proxy_host_changed(const QString &value);
	void proxy_pass_changed(const QString &value);
	void proxy_port_changed(int value);
	void proxy_type_changed(int value);
	void proxy_user_changed(const QString &value);

private:
	void disk_proxy_auth(bool doSync);
	void disk_proxy_host(bool doSync);
	void disk_proxy_pass(bool doSync);
	void disk_proxy_port(bool doSync);
	void disk_proxy_type(bool doSync);
	void disk_proxy_user(bool doSync);
};

#endif
