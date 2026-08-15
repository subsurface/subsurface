// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLANGUAGE_H
#define QPREFLANGUAGE_H
#include "core/pref.h"

#include <QLocale>
#include <QObject>
#include <QVariantList>

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
	Q_PROPERTY(QString effectiveDateFormat READ effectiveDateFormat NOTIFY dateTimeFormatsChanged)
	Q_PROPERTY(QString effectiveDateFormatShort READ effectiveDateFormatShort NOTIFY dateTimeFormatsChanged)
	Q_PROPERTY(QString effectiveTimeFormat READ effectiveTimeFormat NOTIFY dateTimeFormatsChanged)
	Q_PROPERTY(QString longDatePreview READ longDatePreview NOTIFY dateTimeFormatsChanged)
	Q_PROPERTY(QString shortDatePreview READ shortDatePreview NOTIFY dateTimeFormatsChanged)
	Q_PROPERTY(QString timePreview READ timePreview NOTIFY dateTimeFormatsChanged)
	Q_PROPERTY(QVariantList dateFormatPresets READ dateFormatPresets NOTIFY dateTimeFormatsChanged)
	Q_PROPERTY(QVariantList timeFormatPresets READ timeFormatPresets NOTIFY dateTimeFormatsChanged)

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

	// AI-generated (Claude): Shared date/time preference API for all Qt frontends.
	static QString effectiveDateFormat();
	static QString effectiveDateFormatShort();
	static QString effectiveTimeFormat();
	static QString longDatePreview();
	static QString shortDatePreview();
	static QString timePreview();
	static QVariantList dateFormatPresets();
	static QVariantList timeFormatPresets();

	Q_INVOKABLE void applyDateTimeFormats(const QString &longDateFormat, const QString &shortDateFormat,
				      const QString &timeFormat, bool overrideDate, bool overrideTime);
	Q_INVOKABLE void applyDatePreset(const QString &preset);
	Q_INVOKABLE void applyTimePreset(const QString &preset);
	Q_INVOKABLE void restoreDateTimeDefaults();

	// Apply selected-locale defaults during UI locale initialization.
	static void applyLocaleDefaults(const QLocale &locale);

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
	void dateTimeFormatsChanged();

private:
	qPrefLanguage() {}

	static QLocale preferenceLocale();
	static QString defaultDateFormat(const QLocale &locale);
	static QString defaultShortDateFormat(const QLocale &locale);
	static QString defaultTimeFormat(const QLocale &locale);
	static void applyFormats(const QString &longDateFormat, const QString &shortDateFormat,
				 const QString &timeFormat, bool overrideDate, bool overrideTime);
	static void storeFormats(const QString &longDateFormat, const QString &shortDateFormat,
				 const QString &timeFormat, bool overrideDate, bool overrideTime);

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
