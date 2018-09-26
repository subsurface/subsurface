// SPDX-License-Identifier: GPL-2.0
#include "qPrefLanguage.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("Language");

qPrefLanguage *qPrefLanguage::instance()
{
	static qPrefLanguage *self = new qPrefLanguage;
	return self;
}

void qPrefLanguage::loadSync(bool doSync)
{
	disk_date_format(doSync);
	disk_date_format_override(doSync);
	disk_date_format_short(doSync);
	disk_language(doSync);
	disk_lang_locale(doSync);
	disk_time_format(doSync);
	disk_time_format_override(doSync);
	disk_use_system_language(doSync);
}

HANDLE_PREFERENCE_TXT(Language, "date_format", date_format);

HANDLE_PREFERENCE_BOOL(Language,"date_format_override", date_format_override);

HANDLE_PREFERENCE_TXT(Language, "date_format_short", date_format_short);

HANDLE_PREFERENCE_TXT_EXT(Language, "UiLanguage", language, locale.);

HANDLE_PREFERENCE_TXT_EXT(Language, "UiLangLocale", lang_locale, locale.);

HANDLE_PREFERENCE_TXT(Language, "time_format", time_format);

HANDLE_PREFERENCE_BOOL(Language, "time_format_override", time_format_override);

HANDLE_PREFERENCE_BOOL_EXT(Language, "UseSystemLanguage", use_system_language, locale.);
