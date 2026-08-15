// SPDX-License-Identifier: GPL-2.0
#include "qPrefLanguage.h"
#include "qPrefPrivate.h"

#include <QDate>
#include <QTime>
#include <QVariantMap>

static const QString group = QStringLiteral("Language");

static const QDate previewDate(2000, 12, 31);
static const QTime previewTime(13, 45);

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

DISK_LOADSYNC_TXT(Language, "date_format", date_format);

DISK_LOADSYNC_BOOL(Language,"date_format_override", date_format_override);

DISK_LOADSYNC_TXT(Language, "date_format_short", date_format_short);

HANDLE_PREFERENCE_TXT_EXT(Language, "UiLanguage", language, locale.);

DISK_LOADSYNC_TXT_EXT(Language, "UiLangLocale", lang_locale, locale.);

DISK_LOADSYNC_TXT(Language, "time_format", time_format);

DISK_LOADSYNC_BOOL(Language, "time_format_override", time_format_override);

DISK_LOADSYNC_BOOL_EXT(Language, "UseSystemLanguage", use_system_language, locale.);

// AI-generated (Claude): Keep legacy keys while applying related formats as one state change.
QLocale qPrefLanguage::preferenceLocale()
{
	if (use_system_language() && !QLocale().uiLanguages().isEmpty())
		return QLocale(QLocale().uiLanguages().first());
	return QLocale(lang_locale());
}

QString qPrefLanguage::defaultDateFormat(const QLocale &locale)
{
	QString format = locale.dateFormat(QLocale::LongFormat);
	format.replace("dddd,", "ddd").replace("dddd", "ddd").replace("MMMM", "MMM");
	format.replace("'en' 'den' d:'e'", " d");
	return format;
}

QString qPrefLanguage::defaultShortDateFormat(const QLocale &locale)
{
	return locale.dateFormat(QLocale::ShortFormat);
}

QString qPrefLanguage::defaultTimeFormat(const QLocale &locale)
{
	QString format = locale.timeFormat();
	format.replace("(t)", "").replace(" t", "").replace("t", "").replace("hh", "h").replace("HH", "H").replace("'kl'.", "");
	format.replace(".ss", "").replace(":ss", "").replace("ss", "");
	return format;
}

QString qPrefLanguage::effectiveDateFormat()
{
	return date_format_override() && !date_format().isEmpty() ? date_format() : defaultDateFormat(preferenceLocale());
}

QString qPrefLanguage::effectiveDateFormatShort()
{
	return date_format_override() && !date_format_short().isEmpty() ? date_format_short() : defaultShortDateFormat(preferenceLocale());
}

QString qPrefLanguage::effectiveTimeFormat()
{
	return time_format_override() && !time_format().isEmpty() ? time_format() : defaultTimeFormat(preferenceLocale());
}

QString qPrefLanguage::longDatePreview()
{
	return preferenceLocale().toString(previewDate, effectiveDateFormat());
}

QString qPrefLanguage::shortDatePreview()
{
	return preferenceLocale().toString(previewDate, effectiveDateFormatShort());
}

QString qPrefLanguage::timePreview()
{
	return preferenceLocale().toString(previewTime, effectiveTimeFormat());
}

QVariantList qPrefLanguage::dateFormatPresets()
{
	const QLocale locale = preferenceLocale();
	const QVariantList presets = {
		QVariantMap{ { "id", "system" }, { "name", tr("System default") },
			     { "longFormat", defaultDateFormat(locale) }, { "shortFormat", defaultShortDateFormat(locale) } },
		QVariantMap{ { "id", "day-first" }, { "name", tr("Day first") },
			     { "longFormat", "dd.MM.yyyy" }, { "shortFormat", "d.M.yy" } },
		QVariantMap{ { "id", "month-first" }, { "name", tr("Month first") },
			     { "longFormat", "MM/dd/yyyy" }, { "shortFormat", "M/d/yy" } },
		QVariantMap{ { "id", "iso" }, { "name", tr("ISO") },
			     { "longFormat", "yyyy-MM-dd" }, { "shortFormat", "yy-M-d" } }
	};
	QVariantList result;
	for (const QVariant &entry: presets) {
		QVariantMap preset = entry.toMap();
		preset["preview"] = locale.toString(previewDate, preset["longFormat"].toString()) +
			QStringLiteral(" / ") + locale.toString(previewDate, preset["shortFormat"].toString());
		result.append(preset);
	}
	return result;
}

QVariantList qPrefLanguage::timeFormatPresets()
{
	const QLocale locale = preferenceLocale();
	const QVariantList presets = {
		QVariantMap{ { "id", "system" }, { "name", tr("System default") }, { "format", defaultTimeFormat(locale) } },
		QVariantMap{ { "id", "24-hour" }, { "name", tr("24-hour") }, { "format", "hh:mm" } },
		QVariantMap{ { "id", "12-hour" }, { "name", tr("12-hour") }, { "format", "h:mm AP" } }
	};
	QVariantList result;
	for (const QVariant &entry: presets) {
		QVariantMap preset = entry.toMap();
		preset["preview"] = locale.toString(previewTime, preset["format"].toString());
		result.append(preset);
	}
	return result;
}

void qPrefLanguage::applyFormats(const QString &longDateFormat, const QString &shortDateFormat,
				 const QString &timeFormat, bool overrideDate, bool overrideTime)
{
	const QString effectiveLongDate = overrideDate && !longDateFormat.isEmpty() ? longDateFormat : defaultDateFormat(preferenceLocale());
	const QString effectiveShortDate = overrideDate && !shortDateFormat.isEmpty() ? shortDateFormat : defaultShortDateFormat(preferenceLocale());
	const QString effectiveTime = overrideTime && !timeFormat.isEmpty() ? timeFormat : defaultTimeFormat(preferenceLocale());
	storeFormats(effectiveLongDate, effectiveShortDate, effectiveTime, overrideDate, overrideTime);
}

void qPrefLanguage::storeFormats(const QString &longDateFormat, const QString &shortDateFormat,
				 const QString &timeFormat, bool overrideDate, bool overrideTime)
{
	const bool dateChanged = date_format() != longDateFormat;
	const bool shortDateChanged = date_format_short() != shortDateFormat;
	const bool dateOverrideChanged = date_format_override() != overrideDate;
	const bool timeChanged = time_format() != timeFormat;
	const bool timeOverrideChanged = time_format_override() != overrideTime;

	if (!dateChanged && !shortDateChanged && !dateOverrideChanged && !timeChanged && !timeOverrideChanged)
		return;

	prefs.date_format = longDateFormat.toStdString();
	prefs.date_format_short = shortDateFormat.toStdString();
	prefs.date_format_override = overrideDate;
	prefs.time_format = timeFormat.toStdString();
	prefs.time_format_override = overrideTime;
	disk_date_format(true);
	disk_date_format_short(true);
	disk_date_format_override(true);
	disk_time_format(true);
	disk_time_format_override(true);

	qPrefLanguage *language = instance();
	if (dateChanged)
		emit language->date_formatChanged(longDateFormat);
	if (shortDateChanged)
		emit language->date_format_shortChanged(shortDateFormat);
	if (dateOverrideChanged)
		emit language->date_format_overrideChanged(overrideDate);
	if (timeChanged)
		emit language->time_formatChanged(timeFormat);
	if (timeOverrideChanged)
		emit language->time_format_overrideChanged(overrideTime);
	emit language->dateTimeFormatsChanged();
}

void qPrefLanguage::applyDateTimeFormats(const QString &longDateFormat, const QString &shortDateFormat,
					 const QString &timeFormat, bool overrideDate, bool overrideTime)
{
	applyFormats(longDateFormat, shortDateFormat, timeFormat, overrideDate, overrideTime);
}

void qPrefLanguage::applyDatePreset(const QString &preset)
{
	for (const QVariant &entry: dateFormatPresets()) {
		const QVariantMap values = entry.toMap();
		if (values["id"].toString() == preset) {
			applyFormats(values["longFormat"].toString(), values["shortFormat"].toString(),
				     effectiveTimeFormat(), preset != QStringLiteral("system"), time_format_override());
			return;
		}
	}
}

void qPrefLanguage::applyTimePreset(const QString &preset)
{
	for (const QVariant &entry: timeFormatPresets()) {
		const QVariantMap values = entry.toMap();
		if (values["id"].toString() == preset) {
			applyFormats(effectiveDateFormat(), effectiveDateFormatShort(), values["format"].toString(),
				     date_format_override(), preset != QStringLiteral("system"));
			return;
		}
	}
}

void qPrefLanguage::restoreDateTimeDefaults()
{
	applyFormats(QString(), QString(), QString(), false, false);
}

void qPrefLanguage::applyLocaleDefaults(const QLocale &locale)
{
	const QString longDate = date_format_override() && !date_format().isEmpty() ? date_format() : defaultDateFormat(locale);
	const QString shortDate = date_format_override() && !date_format_short().isEmpty() ? date_format_short() : defaultShortDateFormat(locale);
	const QString time = time_format_override() && !time_format().isEmpty() ? time_format() : defaultTimeFormat(locale);
	storeFormats(longDate, shortDate, time, date_format_override(), time_format_override());
}

void qPrefLanguage::set_date_format(const QString &value)
{
	if (value == date_format())
		return;
	prefs.date_format = value.toStdString();
	disk_date_format(true);
	emit instance()->date_formatChanged(value);
	emit instance()->dateTimeFormatsChanged();
}

void qPrefLanguage::set_date_format_override(bool value)
{
	if (value == date_format_override())
		return;
	prefs.date_format_override = value;
	disk_date_format_override(true);
	emit instance()->date_format_overrideChanged(value);
	emit instance()->dateTimeFormatsChanged();
}

void qPrefLanguage::set_date_format_short(const QString &value)
{
	if (value == date_format_short())
		return;
	prefs.date_format_short = value.toStdString();
	disk_date_format_short(true);
	emit instance()->date_format_shortChanged(value);
	emit instance()->dateTimeFormatsChanged();
}

void qPrefLanguage::set_time_format(const QString &value)
{
	if (value == time_format())
		return;
	prefs.time_format = value.toStdString();
	disk_time_format(true);
	emit instance()->time_formatChanged(value);
	emit instance()->dateTimeFormatsChanged();
}

void qPrefLanguage::set_time_format_override(bool value)
{
	if (value == time_format_override())
		return;
	prefs.time_format_override = value;
	disk_time_format_override(true);
	emit instance()->time_format_overrideChanged(value);
	emit instance()->dateTimeFormatsChanged();
}

void qPrefLanguage::set_lang_locale(const QString &value)
{
	if (value == lang_locale())
		return;
	prefs.locale.lang_locale = value.toStdString();
	disk_lang_locale(true);
	emit instance()->lang_localeChanged(value);
	emit instance()->dateTimeFormatsChanged();
}

void qPrefLanguage::set_use_system_language(bool value)
{
	if (value == use_system_language())
		return;
	prefs.locale.use_system_language = value;
	disk_use_system_language(true);
	emit instance()->use_system_languageChanged(value);
	emit instance()->dateTimeFormatsChanged();
}
