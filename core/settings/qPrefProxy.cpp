// SPDX-License-Identifier: GPL-2.0
#include "qPrefProxy.h"
#include "qPrefPrivate.h"

#include <QNetworkProxy>

static const QString group = QStringLiteral("Network");

qPrefProxy *qPrefProxy::instance()
{
	static qPrefProxy *self = new qPrefProxy;
	return self;
}

void qPrefProxy::loadSync(bool doSync)
{
	disk_proxy_auth(doSync);
	disk_proxy_host(doSync);
	disk_proxy_pass(doSync);
	disk_proxy_port(doSync);
	disk_proxy_type(doSync);
	disk_proxy_user(doSync);
}

HANDLE_PREFERENCE_BOOL(Proxy, "proxy_auth", proxy_auth);

HANDLE_PREFERENCE_TXT(Proxy, "proxy_host", proxy_host);

HANDLE_PREFERENCE_TXT(Proxy, "proxy_pass", proxy_pass);

HANDLE_PREFERENCE_INT(Proxy, "proxy_port", proxy_port);

HANDLE_PREFERENCE_INT_DEF(Proxy, "proxy_type", proxy_type, QNetworkProxy::DefaultProxy);

HANDLE_PREFERENCE_TXT(Proxy, "proxy_user", proxy_user);
