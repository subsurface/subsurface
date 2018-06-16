// SPDX-License-Identifier: GPL-2.0
#include "qPrefCS.h"
#include "qPref_private.h"



qPrefCS *qPrefCS::m_instance = NULL;
qPrefCS *qPrefCS::instance()
{
	if (!m_instance)
		m_instance = new qPrefCS;
	return m_instance;
}

void qPrefCS::loadSync(bool doSync)
{
	diskBaseUrl(doSync);
	diskEmail(doSync);
	diskEmailEncoded(doSync);
	diskGitLocalOnly(doSync);
	diskGitUrl(doSync);
	diskPassword(doSync);
	diskPin(doSync);
	diskSavePasswordLocal(doSync);
	diskSaveUserIdLocal(doSync);
	diskTimeout(doSync);
	diskUserId(doSync);
	diskVerificationStatus(doSync);
}


const QString qPrefCS::baseUrl() const
{
	return QString(prefs.cloud_base_url);
}
void qPrefCS::setBaseUrl(const QString& value)
{
	if (value != prefs.cloud_base_url) {
		// only free and set if not default
		if (prefs.cloud_base_url != default_prefs.cloud_base_url) {
			COPY_TXT(cloud_base_url, value);
			COPY_TXT(cloud_git_url, QString(prefs.cloud_base_url) + "/git");
		}

		diskBaseUrl(true);
		emit baseUrlChanged(value);
	}
}
void qPrefCS::diskBaseUrl(bool doSync)
{
	LOADSYNC_TXT("/cloud_base_url", cloud_base_url);
	LOADSYNC_TXT("/cloud_git_url", cloud_git_url);
}


const QString qPrefCS::email() const
{
	return QString(prefs.cloud_storage_email);
}
void qPrefCS::setEmail(const QString& value)
{
	if (value != prefs.cloud_storage_email) {
		COPY_TXT(cloud_storage_email, value);
		diskEmail(true);
		emit emailChanged(value);
	}
}
void qPrefCS::diskEmail(bool doSync)
{
	LOADSYNC_TXT("/email", cloud_storage_email);
}


const QString qPrefCS::emailEncoded() const
{
	return QString(prefs.cloud_storage_email_encoded);
}
void qPrefCS::setEmailEncoded(const QString& value)
{
	if (value != prefs.cloud_storage_email_encoded) {
		COPY_TXT(cloud_storage_email_encoded, value);
		diskEmailEncoded(true);
		emit emailEncodedChanged(value);
	}
}
void qPrefCS::diskEmailEncoded(bool doSync)
{
	LOADSYNC_TXT("/email_encoded", cloud_storage_email_encoded);
}


bool qPrefCS::gitLocalOnly() const
{
	return prefs.git_local_only;
}
void qPrefCS::setGitLocalOnly(bool value)
{
	if (value != prefs.git_local_only) {
		prefs.git_local_only = value;
		diskGitLocalOnly(true);
		emit gitLocalOnlyChanged(value);
	}
}
void qPrefCS::diskGitLocalOnly(bool doSync)
{
	LOADSYNC_BOOL("/git_local_only", git_local_only);
}


const QString qPrefCS::gitUrl() const
{
	return QString(prefs.cloud_git_url);
}
void qPrefCS::setGitUrl(const QString& value)
{
	if (value != prefs.cloud_git_url) {
		// only free and set if not default
		if (prefs.cloud_git_url != default_prefs.cloud_git_url) {
			COPY_TXT(cloud_git_url, value);
		}
		diskGitUrl(true);
		emit gitUrlChanged(value);
	}
}
void qPrefCS::diskGitUrl(bool doSync)
{
	LOADSYNC_TXT("/cloud_git_url", cloud_git_url);
}


const QString qPrefCS::newPassword() const
{
	return QString(prefs.cloud_storage_newpassword);
}
void qPrefCS::setNewPassword(const QString& value)
{
	if (value == prefs.cloud_storage_newpassword)
		return;

	COPY_TXT(cloud_storage_newpassword, value);

	// NOT saved on disk, because it is only temporary
	emit newPasswordChanged(value);
}


const QString qPrefCS::password() const
{
	return QString(prefs.cloud_storage_password);
}
void qPrefCS::setPassword(const QString& value)
{
	if (value != prefs.cloud_storage_password) {
		COPY_TXT(cloud_storage_password,value);
		diskPassword(true);
		emit passwordChanged(value);
	}
}
void qPrefCS::diskPassword(bool doSync)
{
	if (!doSync || !prefs.save_password_local)
		LOADSYNC_TXT("/password", cloud_storage_password);
}


const QString qPrefCS::pin() const
{
	return QString(prefs.cloud_storage_pin);
}
void qPrefCS::setPin(const QString& value)
{
	if (value != prefs.cloud_storage_pin) {
		COPY_TXT(cloud_storage_pin, value);
		diskPin(true);	
		emit pinChanged(value);
	}
}
void qPrefCS::diskPin(bool doSync)
{
	LOADSYNC_TXT("/pin", cloud_storage_pin);
}


bool qPrefCS::savePasswordLocal() const
{
	return prefs.save_password_local;
}
void qPrefCS::setSavePasswordLocal(bool value)
{
	if (value != prefs.save_password_local) {
		prefs.save_password_local = value;
		diskSavePasswordLocal(true);
		emit savePasswordLocalChanged(value);
	}
}
void qPrefCS::diskSavePasswordLocal(bool doSync)
{
	LOADSYNC_BOOL("/save_password_local", save_password_local);
}


bool qPrefCS::saveUserIdLocal() const
{
	return prefs.save_userid_local;
}
void qPrefCS::setSaveUserIdLocal(bool value)
{
	if (value != prefs.save_userid_local) {
		prefs.save_userid_local = value;
		diskSaveUserIdLocal(true);
		emit saveUserIdLocalChanged(value);
	}
}
void qPrefCS::diskSaveUserIdLocal(bool doSync)
{
	LOADSYNC_BOOL("/save_userid_local", save_userid_local);
}


short qPrefCS::timeout() const
{
	return prefs.cloud_timeout;
}
void qPrefCS::setTimeout(short value)
{
	if (value != prefs.cloud_timeout) {
		prefs.cloud_timeout = value;
		diskTimeout(true);
		emit timeoutChanged(value);
	}
}
void qPrefCS::diskTimeout(bool doSync)
{
	LOADSYNC_INT("/timeout", cloud_timeout);
}


const QString qPrefCS::userId() const
{
	return QString(prefs.userid);
}
void qPrefCS::setUserId(const QString& value)
{
	if (value != prefs.userid) {
		COPY_TXT(userid, value);
		diskUserId(true);
		emit userIdChanged(value);
	}
}
void qPrefCS::diskUserId(bool doSync)
{
	//WARNING: UserId is stored outside of any group, but it belongs to Cloud Storage.
	const QString group = QStringLiteral("");
	LOADSYNC_TXT("subsurface_webservice_uid", userid);
}


short qPrefCS::verificationStatus() const
{
	return prefs.cloud_verification_status;
}
void qPrefCS::setVerificationStatus(short value)
{
	if (value != prefs.cloud_verification_status) {
		prefs.cloud_verification_status = value;
		diskVerificationStatus(true);
		emit verificationStatusChanged(value);
	}
}
void qPrefCS::diskVerificationStatus(bool doSync)
{
	LOADSYNC_INT("/cloud_verification_status", cloud_verification_status);
}
