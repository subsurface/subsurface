// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLANGUAGE_H
#define QPREFLANGUAGE_H
#include "core/pref.h"

#include <QObject>

class qPrefLanguage : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString date_format READ date_format WRITE set_date_format NOTIFY date_format_changed);
	Q_PROPERTY(bool date_format_override READ date_format_override WRITE set_date_format_override NOTIFY date_format_override_changed);
	Q_PROPERTY(QString date_format_short READ date_format_short WRITE set_date_format_short NOTIFY date_format_short_changed);
	Q_PROPERTY(QString language READ language WRITE set_language NOTIFY language_changed);
	Q_PROPERTY(QString lang_locale READ lang_locale WRITE set_lang_locale NOTIFY lang_locale_changed);
	Q_PROPERTY(QString time_format READ time_format WRITE set_time_format NOTIFY time_format_changed);
	Q_PROPERTY(bool time_format_override READ time_format_override WRITE set_time_format_override NOTIFY time_format_override_changed);
	Q_PROPERTY(bool use_system_language READ use_system_language WRITE set_use_system_language NOTIFY use_system_language_changed);

public:
	qPrefLanguage(QObject *parent = NULL);
	static qPrefLanguage *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	const QString date_format() { return prefs.date_format; }
	bool date_format_override() { return prefs.date_format_override; }
	const QString date_format_short() { return prefs.date_format_short; }
	const QString language() { return prefs.locale.language; }
	const QString lang_locale() { return prefs.locale.lang_locale; }
	const QString time_format() { return prefs.time_format; }
	bool time_format_override() { return prefs.time_format_override; }
	bool use_system_language() { return prefs.locale.use_system_language; }

public slots:
	void set_date_format(const QString& value);
	void set_date_format_override(bool value);
	void set_date_format_short(const QString& value);
	void set_language(const QString& value);
	void set_lang_locale(const QString& value);
	void set_time_format(const QString& value);
	void set_time_format_override(bool value);
	void set_use_system_language(bool value);

signals:
	void date_format_changed(const QString& value);
	void date_format_override_changed(bool value);
	void date_format_short_changed(const QString& value);
	void language_changed(const QString& value);
	void lang_locale_changed(const QString& value);
	void time_format_changed(const QString& value);
	void time_format_override_changed(bool value);
	void use_system_language_changed(bool value);

private:
	void disk_date_format(bool doSync);
	void disk_date_format_override(bool doSync);
	void disk_date_format_short(bool doSync);
	void disk_language(bool doSync);
	void disk_lang_locale(bool doSync);
	void disk_time_format(bool doSync);
	void disk_time_format_override(bool doSync);
	void disk_use_system_language(bool doSync);
};

#endif
