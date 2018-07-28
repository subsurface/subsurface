// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("WebApps/Facebook");

qPrefFacebook::qPrefFacebook(QObject *parent) : QObject(parent)
{
}
qPrefFacebook*qPrefFacebook::instance()
{
	static qPrefFacebook *self = new qPrefFacebook;
	return self;
}

void qPrefFacebook::loadSync(bool doSync)
{
	// Empty, because FB probs are not loaded/synced to disk
}

void qPrefFacebook::set_access_token(const QString &value)
{
	if (value != prefs.facebook.access_token) {
		qPrefPrivate::instance()->copy_txt(&prefs.facebook.access_token, value);
		emit access_token_changed(value);
	}
}

void qPrefFacebook::set_album_id(const QString &value)
{
	if (value != prefs.facebook.album_id) {
		qPrefPrivate::instance()->copy_txt(&prefs.facebook.album_id, value);
		emit album_id_changed(value);
	}
}

void qPrefFacebook::set_user_id(const QString &value)
{
	if (value != prefs.facebook.user_id) {
		qPrefPrivate::instance()->copy_txt(&prefs.facebook.user_id, value);
		emit access_token_changed(value);
	}
}
