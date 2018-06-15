// SPDX-License-Identifier: GPL-2.0
#include "qPrefFacebook.h"
#include "qPref_private.h"


qPrefFacebook *qPrefFacebook::m_instance = NULL;
qPrefFacebook *qPrefFacebook::instance()
{
	if (!m_instance)
		m_instance = new qPrefFacebook;
	return m_instance;
}


void qPrefFacebook::loadSync(bool doSync)
{
	diskAccessToken(doSync);
	diskUserId(doSync);
	diskAlbumId(doSync);
}


const QString qPrefFacebook::accessToken() const
{
	return prefs.facebook.access_token;
}
void qPrefFacebook::setAccessToken (const QString& value)
{
	if (value != prefs.facebook.access_token) {
		COPY_TXT(facebook.access_token, value);
		diskAccessToken(true);
		emit accessTokenChanged(value);
	}
}
void qPrefFacebook::diskAccessToken(bool doSync)
{
#if SAVE_FB_CREDENTIALS
	LOADSYNC_TXT("/access_token", facebook.access_token)
#endif
}


const QString qPrefFacebook::userId() const
{
	return QString(prefs.facebook.user_id);
}
void qPrefFacebook::setUserId(const QString& value)
{
	if (value != prefs.facebook.user_id) {
		COPY_TXT(facebook.user_id, value);
		diskUserId(true);
		emit userIdChanged(value);
	}
}
void qPrefFacebook::diskUserId(bool doSync)
{
#if SAVE_FB_CREDENTIALS
	LOADSYNC_TXT("/user_id", facebook.user_id)
#endif
}


const QString qPrefFacebook::albumId() const
{
	return QString(prefs.facebook.album_id);
}
void qPrefFacebook::setAlbumId(const QString& value)
{
	if (value != prefs.facebook.album_id) {
		COPY_TXT(facebook.album_id, value);
		diskAlbumId(true);
		emit albumIdChanged(value);
	}
}
void qPrefFacebook::diskAlbumId(bool doSync)
{
#if SAVE_FB_CREDENTIALS
	LOADSYNC_TXT("/album_id", facebook.album_id)
#endif
}
