// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFCLOUDSTORAGE_H
#define QPREFCLOUDSTORAGE_H


#include <QObject>


class qPrefCloudStorage : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString base_url READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged);
	Q_PROPERTY(bool git_local_only READ gitLocalOnly WRITE setGitLocalOnly NOTIFY gitLocalOnlyChanged);
	Q_PROPERTY(QString git_url READ gitUrl WRITE setGitUrl NOTIFY gitUrlChanged);
	Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged);
	Q_PROPERTY(QString email_encoded READ emailEncoded WRITE setEmailEncoded NOTIFY emailEncodedChanged);
	Q_PROPERTY(QString new_password READ newPassword WRITE setNewPassword NOTIFY newPasswordChanged);
	Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged);
	Q_PROPERTY(QString pin READ pin WRITE setPin NOTIFY pinChanged);
	Q_PROPERTY(short timeout READ timeout WRITE setTimeout NOTIFY timeoutChanged);
	Q_PROPERTY(bool save_password_local READ savePasswordLocal WRITE setSavePasswordLocal NOTIFY savePasswordLocalChanged);
	Q_PROPERTY(bool save_userid_local READ saveUserIdLocal WRITE setSaveUserIdLocal NOTIFY saveUserIdLocalChanged);
	Q_PROPERTY(QString userid READ userId WRITE setUserId NOTIFY userIdChanged);
	Q_PROPERTY(short verification_status READ verificationStatus WRITE setVerificationStatus NOTIFY verificationStatusChanged);

public:
	qPrefCloudStorage(QObject *parent = NULL) : QObject(parent) {};
	~qPrefCloudStorage() {};
	static qPrefCloudStorage *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	const QString baseUrl() const;
	const QString email() const;
	const QString emailEncoded() const;
	const QString gitUrl() const;
	bool  gitLocalOnly() const;
	const QString newPassword() const;
	const QString password() const;
	const QString pin() const;
	bool  savePasswordLocal() const;
	bool  saveUserIdLocal() const;
	short timeout() const;
	const QString userId() const;
	short verificationStatus() const;

public slots:
	void setBaseUrl(const QString& value);
	void setEmail(const QString& value);
	void setEmailEncoded(const QString& value);
	void setGitLocalOnly(bool value);
	void setGitUrl(const QString& value);
	void setNewPassword(const QString& value);
	void setPassword(const QString& value);
	void setPin(const QString& value);
	void setSavePasswordLocal(bool value);
	void setSaveUserIdLocal(bool value);
	void setTimeout(short value);
	void setUserId(const QString& value);
	void setVerificationStatus(short value);

signals:
	void baseUrlChanged(const QString& value);
	void emailChanged(const QString& value);
	void emailEncodedChanged(const QString& value);
	void gitLocalOnlyChanged(bool value);
	void gitUrlChanged(const QString& value);
	void newPasswordChanged(const QString& value);
	void passwordChanged(const QString& value);
	void pinChanged(const QString& value);
	void savePasswordLocalChanged(bool value);
	void saveUserIdLocalChanged(bool value);
	void timeoutChanged(short value);
	void userIdChanged(const QString& value);
	void verificationStatusChanged(short value);

private:
	const QString group = QStringLiteral("CloudStorage");
	static qPrefCloudStorage *m_instance;

	// functions to load/sync variable with disk
	void diskBaseUrl(bool doSync);
	void diskEmail(bool doSync);
	void diskEmailEncoded(bool doSync);
	void diskGitLocalOnly(bool doSync);
	void diskGitUrl(bool doSync);
	void diskNewPassword(bool doSync);
	void diskPassword(bool doSync);
	void diskPin(bool doSync);
	void diskSavePasswordLocal(bool doSync);
	void diskSaveUserIdLocal(bool doSync);
	void diskTimeout(bool doSync);
	void diskUserId(bool doSync);
	void diskVerificationStatus(bool doSync);
};

#endif
