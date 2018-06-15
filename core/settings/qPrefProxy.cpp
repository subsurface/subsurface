// SPDX-License-Identifier: GPL-2.0
#include "qPrefProxy.h"
#include "qPref_private.h"

#include <QNetworkProxy>


qPrefProxy *qPrefProxy::m_instance = NULL;
qPrefProxy *qPrefProxy::instance()
{
	if (!m_instance)
		m_instance = new qPrefProxy;
	return m_instance;
}



void qPrefProxy::loadSync(bool doSync)
{
	diskAuth(doSync);
	diskHost(doSync);
	diskPass(doSync);
	diskPort(doSync);
	diskType(doSync);
	diskUser(doSync);
}

bool qPrefProxy::auth() const
{
	return prefs.proxy_auth;
}
void qPrefProxy::setAuth(bool value)
{
	if (value != prefs.proxy_auth) {
		prefs.proxy_auth = value;
		diskAuth(true);
		emit authChanged(value);
	}
}
void qPrefProxy::diskAuth(bool doSync)
{
	LOADSYNC_BOOL("/proxy_auth", proxy_auth);
}


const QString qPrefProxy::host() const
{
	return prefs.proxy_host;
}
void qPrefProxy::setHost(const QString& value)
{
	if (value != prefs.proxy_host) {
		COPY_TXT(proxy_host, value);
		diskHost(true);
		emit hostChanged(value);
	}
}
void qPrefProxy::diskHost(bool doSync)
{
	LOADSYNC_TXT("/proxy_host", proxy_host);
}


const QString qPrefProxy::pass() const
{
	return prefs.proxy_pass;
}
void qPrefProxy::setPass(const QString& value)
{
	if (value != prefs.proxy_pass) {
		COPY_TXT(proxy_pass, value);
		diskPass(true);
		emit passChanged(value);
	}
}
void qPrefProxy::diskPass(bool doSync)
{
	LOADSYNC_TXT("/proxy_pass", proxy_pass);
}


int qPrefProxy::port() const
{
	return prefs.proxy_port;
}
void qPrefProxy::setPort(int value)
{
	if (value != prefs.proxy_port) {
		prefs.proxy_port = value;
		diskPort(true);
		emit portChanged(value);
	}
}
void qPrefProxy::diskPort(bool doSync)
{
	LOADSYNC_INT("/proxy_port", proxy_port);
}


int qPrefProxy::type() const
{
	return prefs.proxy_type;
}
void qPrefProxy::setType(int value)
{
	if (value != prefs.proxy_type) {
		prefs.proxy_type = value;
		diskType(true);
		emit typeChanged(value);
	}
}
void qPrefProxy::diskType(bool doSync)
{
	LOADSYNC_INT_DEF("/proxy_type", proxy_port, QNetworkProxy::DefaultProxy);
}


const QString qPrefProxy::user() const
{
	return prefs.proxy_user;
}
void qPrefProxy::setUser(const QString& value)
{
	if (value != prefs.proxy_user) {
		COPY_TXT(proxy_user, value);
		diskUser(true);
		emit userChanged(value);
	}
}
void qPrefProxy::diskUser(bool doSync)
{
	LOADSYNC_TXT("/proxy_user", proxy_user);
}
