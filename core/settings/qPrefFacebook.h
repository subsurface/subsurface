// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFSFACEBOOK_H
#define QPREFSFACEBOOK_H

#include <QObject>


class qPrefFacebook : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString	access_token
				READ	accessToken
				WRITE	setAccessToken
				NOTIFY  accessTokenChanged);
	Q_PROPERTY(QString	album_id
				READ	albumId
				WRITE	setAlbumId
				NOTIFY  albumIdChanged);
	Q_PROPERTY(QString	user_id
				READ	userId
				WRITE	setUserId
				NOTIFY  userIdChanged);

public:
	qPrefFacebook(QObject *parent = NULL) : QObject(parent) {};
	~qPrefFacebook() {};
	static qPrefFacebook *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	const QString accessToken() const;
	const QString userId() const;
	const QString albumId() const;

public slots:
	void setAccessToken(const QString& value);
	void setUserId(const QString& value);
	void setAlbumId(const QString& value);

signals:
	void accessTokenChanged(const QString& value);
	void userIdChanged(const QString& value);
	void albumIdChanged(const QString& value);

private:
	const QString group = QStringLiteral("Facebook");
	static qPrefFacebook *m_instance;

	void diskAccessToken(bool doSync);
	void diskUserId(bool doSync);
	void diskAlbumId(bool doSync);
};

#endif
