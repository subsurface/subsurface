// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFSFACEBOOK_H
#define QPREFSFACEBOOK_H
#include "core/pref.h"

#include <QObject>


class qPrefFacebook : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString access_token READ access_token WRITE set_access_token NOTIFY access_token_changed);
	Q_PROPERTY(QString album_id READ album_id WRITE set_album_id NOTIFY album_id_changed);
	Q_PROPERTY(QString user_id READ user_id WRITE set_user_id NOTIFY user_id_changed);

public:
	qPrefFacebook(QObject *parent = NULL);
	static qPrefFacebook *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void inline load() {loadSync(false); }
	void inline sync() {loadSync(true); }

public:
	static inline QString access_token() { return prefs.facebook.access_token; }
	static inline QString album_id() { return prefs.facebook.album_id; }
	static inline QString user_id() { return prefs.facebook.user_id; }

public slots:
	void set_access_token(const QString& value);
	void set_album_id(const QString& value);
	void set_user_id(const QString& value);

signals:
	void access_token_changed(const QString& value);
	void album_id_changed(const QString& value);
	void user_id_changed(const QString& value);

private:
	void disk_access_token(bool doSync);
	void disk_album_id(bool doSync);
	void disk_user_id(bool doSync);
};

#endif
