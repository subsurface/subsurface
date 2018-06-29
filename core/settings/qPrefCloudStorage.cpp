// SPDX-License-Identifier: GPL-2.0
#include "qPrefCloudStorage.h"
#include "qPref_private.h"



qPrefCloudStorage *qPrefCloudStorage::m_instance = NULL;
qPrefCloudStorage *qPrefCloudStorage::instance()
{
	if (!m_instance)
		m_instance = new qPrefCloudStorage;
	return m_instance;
}

void qPrefCloudStorage::loadSync(bool doSync)
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


const QString qPrefCloudStorage::baseUrl() const
{
	return QString(prefs.cloud_base_url);
}
void qPrefCloudStorage::setBaseUrl(const QString& value)
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
void qPrefCloudStorage::diskBaseUrl(bool doSync)
{
	LOADSYNC_TXT("/cloud_base_url", cloud_base_url);
	LOADSYNC_TXT("/cloud_git_url", cloud_git_url);
}


const QString qPrefCloudStorage::email() const
{
	return QString(prefs.cloud_storage_email);
}
void qPrefCloudStorage::setEmail(const QString& value)
{
	if (value != prefs.cloud_storage_email) {
		COPY_TXT(cloud_storage_email, value);
		diskEmail(true);
		emit emailChanged(value);
	}
}
void qPrefCloudStorage::diskEmail(bool doSync)
{
	LOADSYNC_TXT("/email", cloud_storage_email);
}


const QString qPrefCloudStorage::emailEncoded() const
{
	return QString(prefs.cloud_storage_email_encoded);
}
void qPrefCloudStorage::setEmailEncoded(const QString& value)
{
	if (value != prefs.cloud_storage_email_encoded) {
		COPY_TXT(cloud_storage_email_encoded, value);
		diskEmailEncoded(true);
		emit emailEncodedChanged(value);
	}
}
void qPrefCloudStorage::diskEmailEncoded(bool doSync)
{
	LOADSYNC_TXT("/email_encoded", cloud_storage_email_encoded);
}


bool qPrefCloudStorage::gitLocalOnly() const
{
	return prefs.git_local_only;
}
void qPrefCloudStorage::setGitLocalOnly(bool value)
{
	if (value != prefs.git_local_only) {
		prefs.git_local_only = value;
		diskGitLocalOnly(true);
		emit gitLocalOnlyChanged(value);
	}
}
void qPrefCloudStorage::diskGitLocalOnly(bool doSync)
{
	LOADSYNC_BOOL("/git_local_only", git_local_only);
}


const QString qPrefCloudStorage::gitUrl() const
{
	return QString(prefs.cloud_git_url);
}
void qPrefCloudStorage::setGitUrl(const QString& value)
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
void qPrefCloudStorage::diskGitUrl(bool doSync)
{
	LOADSYNC_TXT("/cloud_git_url", cloud_git_url);
}


const QString qPrefCloudStorage::newPassword() const
{
	return QString(prefs.cloud_storage_newpassword);
}
void qPrefCloudStorage::setNewPassword(const QString& value)
{
	if (value == prefs.cloud_storage_newpassword)
		return;

	COPY_TXT(cloud_storage_newpassword, value);

	// NOT saved on disk, because it is only temporary
	emit newPasswordChanged(value);
}


const QString qPrefCloudStorage::password() const
{
	return QString(prefs.cloud_storage_password);
}
void qPrefCloudStorage::setPassword(const QString& value)
{
	if (value != prefs.cloud_storage_password) {
		COPY_TXT(cloud_storage_password,value);
		diskPassword(true);
		emit passwordChanged(value);
	}
}
void qPrefCloudStorage::diskPassword(bool doSync)
{
	if (!doSync || !prefs.save_password_local)
		LOADSYNC_TXT("/password", cloud_storage_password);
}


const QString qPrefCloudStorage::pin() const
{
	return QString(prefs.cloud_storage_pin);
}
void qPrefCloudStorage::setPin(const QString& value)
{
	if (value != prefs.cloud_storage_pin) {
		COPY_TXT(cloud_storage_pin, value);
		diskPin(true);	
		emit pinChanged(value);
	}
}
void qPrefCloudStorage::diskPin(bool doSync)
{
	LOADSYNC_TXT("/pin", cloud_storage_pin);
}


bool qPrefCloudStorage::savePasswordLocal() const
{
	return prefs.save_password_local;
}
void qPrefCloudStorage::setSavePasswordLocal(bool value)
{
	if (value != prefs.save_password_local) {
		prefs.save_password_local = value;
		diskSavePasswordLocal(true);
		emit savePasswordLocalChanged(value);
	}
}
void qPrefCloudStorage::diskSavePasswordLocal(bool doSync)
{
	LOADSYNC_BOOL("/save_password_local", save_password_local);
}


bool qPrefCloudStorage::saveUserIdLocal() const
{
	return prefs.save_userid_local;
}
void qPrefCloudStorage::setSaveUserIdLocal(bool value)
{
	if (value != prefs.save_userid_local) {
		prefs.save_userid_local = value;
		diskSaveUserIdLocal(true);
		emit saveUserIdLocalChanged(value);
	}
}
void qPrefCloudStorage::diskSaveUserIdLocal(bool doSync)
{
	LOADSYNC_BOOL("/save_userid_local", save_userid_local);
}


short qPrefCloudStorage::timeout() const
{
	return prefs.cloud_timeout;
}
void qPrefCloudStorage::setTimeout(short value)
{
	if (value != prefs.cloud_timeout) {
		prefs.cloud_timeout = value;
		diskTimeout(true);
		emit timeoutChanged(value);
	}
}
void qPrefCloudStorage::diskTimeout(bool doSync)
{
	LOADSYNC_INT("/timeout", cloud_timeout);
}


const QString qPrefCloudStorage::userId() const
{
	return QString(prefs.userid);
}
void qPrefCloudStorage::setUserId(const QString& value)
{
	if (value != prefs.userid) {
		COPY_TXT(userid, value);
		diskUserId(true);
		emit userIdChanged(value);
	}
}
void qPrefCloudStorage::diskUserId(bool doSync)
{
	//WARNING: UserId is stored outside of any group, but it belongs to Cloud Storage.
	const QString group = QStringLiteral("");
	LOADSYNC_TXT("subsurface_webservice_uid", userid);
}


short qPrefCloudStorage::verificationStatus() const
{
	return prefs.cloud_verification_status;
}
void qPrefCloudStorage::setVerificationStatus(short value)
{
	if (value != prefs.cloud_verification_status) {
		prefs.cloud_verification_status = value;
		diskVerificationStatus(true);
		emit verificationStatusChanged(value);
	}
}
void qPrefCloudStorage::diskVerificationStatus(bool doSync)
{
	LOADSYNC_INT("/cloud_verification_status", cloud_verification_status);
}
