// SPDX-License-Identifier: GPL-2.0
#include "qPrefLanguage.h"
#include "qPref_private.h"



qPrefLanguage *qPrefLanguage::m_instance = NULL;
qPrefLanguage *qPrefLanguage::instance()
{
	if (!m_instance)
		m_instance = new qPrefLanguage;
	return m_instance;
}


void qPrefLanguage::loadSync(bool doSync)
{
	diskDateFormat(doSync);
	diskDateFormatOverride(doSync);
	diskDateFormatShort(doSync);
	diskLanguage(doSync);
	diskLangLocale(doSync);
	diskTimeFormat(doSync);
	diskTimeFormatOverride(doSync);
	diskUseSystemLanguage(doSync);
}

const QString qPrefLanguage::dateFormat() const
{
	return prefs.date_format;
}
void qPrefLanguage::setDateFormat(const QString& value)
{
	if (value != prefs.date_format) {
		COPY_TXT(date_format, value);
		diskDateFormat(true);
		emit dateFormatChanged(value);
	}
}
void qPrefLanguage::diskDateFormat(bool doSync)
{
	LOADSYNC_TXT("/date_format", date_format);
}


bool qPrefLanguage::dateFormatOverride() const
{
	return prefs.date_format_override;
}
void qPrefLanguage::setDateFormatOverride(bool value)
{
	if (value != prefs.date_format_override) {
		prefs.date_format_override = value;
		diskDateFormatOverride(true);
		emit dateFormatOverrideChanged(value);
	}
}
void qPrefLanguage::diskDateFormatOverride(bool doSync)
{
	LOADSYNC_BOOL("/date_format_override", date_format_override);
}


const QString qPrefLanguage::dateFormatShort() const
{
	return prefs.date_format_short;
}
void qPrefLanguage::setDateFormatShort(const QString& value)
{
	if (value != prefs.date_format_short) {
		COPY_TXT(date_format_short, value);
		diskDateFormatShort(true);
		emit dateFormatShortChanged(value);
	}
}
void qPrefLanguage::diskDateFormatShort(bool doSync)
{
	LOADSYNC_TXT("/date_format_short", date_format_short);
}


const QString qPrefLanguage::language() const
{
	return prefs.locale.language;
}
void qPrefLanguage::setLanguage(const QString& value)
{
	if (value != prefs.locale.language) {
		COPY_TXT(locale.language, value);
		diskLanguage(true);
		emit languageChanged(value);
	}
}
void qPrefLanguage::diskLanguage(bool doSync)
{
	LOADSYNC_TXT("/UiLanguage", locale.language);
}


const QString qPrefLanguage::langLocale() const
{
	return prefs.locale.lang_locale;
}
void qPrefLanguage::setLangLocale(const QString &value)
{
	if (value != prefs.locale.lang_locale) {
		COPY_TXT(locale.lang_locale, value);
		diskLangLocale(true);
		emit langLocaleChanged(value);
	}
}
void qPrefLanguage::diskLangLocale(bool doSync)
{
	LOADSYNC_TXT("/UiLangLocale", locale.lang_locale);
}


const QString qPrefLanguage::timeFormat() const
{
	return prefs.time_format;
}
void qPrefLanguage::setTimeFormat(const QString& value)
{
	if (value != prefs.time_format) {
		COPY_TXT(time_format, value);
		diskTimeFormat(true);
		emit timeFormatChanged(value);
	}
}
void qPrefLanguage::diskTimeFormat(bool doSync)
{
	LOADSYNC_TXT("/time_format", time_format);
}


bool qPrefLanguage::timeFormatOverride() const
{
	return prefs.time_format_override;
}
void qPrefLanguage::setTimeFormatOverride(bool value)
{
	if (value != prefs.time_format_override) {
		prefs.time_format_override = value;
		diskTimeFormatOverride(true);
		emit timeFormatOverrideChanged(value);
	}
}
void qPrefLanguage::diskTimeFormatOverride(bool doSync)
{
	LOADSYNC_BOOL("/time_format_override", time_format_override);
}


bool qPrefLanguage::useSystemLanguage() const
{
	return prefs.locale.use_system_language;
}
void qPrefLanguage::setUseSystemLanguage(bool value)
{
	if (value != prefs.locale.use_system_language) {
		prefs.locale.use_system_language = value;
		diskUseSystemLanguage(true);
		emit useSystemLanguageChanged(true);
	}
}
void qPrefLanguage::diskUseSystemLanguage(bool doSync)
{
	LOADSYNC_BOOL("/UseSystemLanguage", locale.use_system_language);
}
