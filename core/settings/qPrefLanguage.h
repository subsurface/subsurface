// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLANGUAGE_H
#define QPREFLANGUAGE_H
#include "core/pref.h"

#include <QObject>

class qPrefLanguage : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString date_format READ date_format WRITE set_date_format NOTIFY date_formatChanged)
	Q_PROPERTY(bool date_format_override READ date_format_override WRITE set_date_format_override NOTIFY date_format_overrideChanged)
	Q_PROPERTY(QString date_format_short READ date_format_short WRITE set_date_format_short NOTIFY date_format_shortChanged)
	Q_PROPERTY(QString language READ language WRITE set_language NOTIFY languageChanged)
	Q_PROPERTY(QString lang_locale READ lang_locale WRITE set_lang_locale NOTIFY lang_localeChanged)
	Q_PROPERTY(QString time_format READ time_format WRITE set_time_format NOTIFY time_formatChanged)
	Q_PROPERTY(bool time_format_override READ time_format_override WRITE set_time_format_override NOTIFY time_format_overrideChanged)
	Q_PROPERTY(bool use_system_language READ use_system_language WRITE set_use_system_language NOTIFY use_system_languageChanged)

public:
	static qPrefLanguage *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static const QString date_format() { return QString::fromStdString(prefs.date_format); }
	static bool date_format_override() { return prefs.date_format_override; }
	static const QString date_format_short() { return QString::fromStdString(prefs.date_format_short); }
	static const QString language() { return QString::fromStdString(prefs.locale.language); }
	static const QString lang_locale() { return QString::fromStdString(prefs.locale.lang_locale); }
	static const QString time_format() { return QString::fromStdString(prefs.time_format); }
	static bool time_format_override() { return prefs.time_format_override; }
	static bool use_system_language() { return prefs.locale.use_system_language; }

public slots:
	static void set_date_format(const QString& value);
	static void set_date_format_override(bool value);
	static void set_date_format_short(const QString& value);
	static void set_language(const QString& value);
	static void set_lang_locale(const QString& value);
	static void set_time_format(const QString& value);
	static void set_time_format_override(bool value);
	static void set_use_system_language(bool value);

signals:
	void date_formatChanged(const QString& value);
	void date_format_overrideChanged(bool value);
	void date_format_shortChanged(const QString& value);
	void languageChanged(const QString& value);
	void lang_localeChanged(const QString& value);
	void time_formatChanged(const QString& value);
	void time_format_overrideChanged(bool value);
	void use_system_languageChanged(bool value);

private:
	qPrefLanguage() {}

	static void disk_date_format(bool doSync);
	static void disk_date_format_override(bool doSync);
	static void disk_date_format_short(bool doSync);
	static void disk_language(bool doSync);
	static void disk_lang_locale(bool doSync);
	static void disk_time_format(bool doSync);
	static void disk_time_format_override(bool doSync);
	static void disk_use_system_language(bool doSync);
};

#endif
