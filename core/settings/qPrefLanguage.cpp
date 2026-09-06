// SPDX-License-Identifier: GPL-2.0
#include "qPrefLanguage.h"
#include "qPrefPrivate.h"

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <QTime>
#include <QDebug>
#include <QVariantMap>

static const QString group = QStringLiteral("Language");

static const QDate previewDate(2000, 12, 31);
static const QTime previewTime(13, 45);

// AI-generated (Claude): Replace display punctuation with separators available
// on Android's specialised date and time keyboards, without reordering fields.
static QString keypadDateFormat(const QString &format)
{
	QRegularExpression component(QStringLiteral("d+|M+|y+"));
	QRegularExpressionMatchIterator matches = component.globalMatch(format);
	QStringList result;
	while (matches.hasNext()) {
		QString value = matches.next().captured();
		if (value.startsWith(QLatin1Char('d')) && value.size() > 2)
			value = QStringLiteral("d");
		else if (value.startsWith(QLatin1Char('M')) && value.size() > 2)
			value = QStringLiteral("M");
		result.append(value);
	}
	return result.join(QLatin1Char('/'));
}

static QString keypadTimeFormat(const QString &format)
{
	QRegularExpression component(QStringLiteral("AP|ap|h+|H+|m+|s+"));
	QRegularExpressionMatchIterator matches = component.globalMatch(format);
	QStringList timeComponents;
	QString designator;
	bool designatorFirst = false;
	while (matches.hasNext()) {
		const QString value = matches.next().captured();
		if (value == QLatin1String("AP") || value == QLatin1String("ap")) {
			designator = value;
			designatorFirst = timeComponents.isEmpty();
		} else {
			timeComponents.append(value);
		}
	}
	QString result = timeComponents.join(QLatin1Char(':'));
	if (!designator.isEmpty())
		result = designatorFirst ? designator + QLatin1Char(' ') + result : result + QLatin1Char(' ') + designator;
	return result;
}

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
	// AI-generated (Claude): On iOS the 12/24-hour preference is a system
	// setting independent of the UI language. QLocale() (no arguments) reads
	// it correctly; a locale constructed from a language tag (e.g. "en") uses
	// Qt's built-in default for that language and may disagree with the iOS
	// toggle. Use the system locale for the time format pattern.
	//
	// Qt on iOS returns "h:mm Ap" — mixed-case AM/PM token. Qt's C++ date/time
	// formatting only recognises "AP" (uppercase) or "ap" (lowercase); "Ap" is
	// treated as literals, causing toString() to produce 24-hour output and
	// toDateTime() to fail to parse. Normalise any "Ap" or "aP" to "AP".
	Q_UNUSED(locale)
	QString format = QLocale().timeFormat();
	format.replace("(t)", "").replace(" t", "").replace("t", "").replace("hh", "h").replace("HH", "H").replace("'kl'.", "");
	format.replace(".ss", "").replace(":ss", "").replace("ss", "");
	// Normalise mixed-case AM/PM token to uppercase "AP"
	format.replace("Ap", "AP").replace("aP", "AP");
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
	const QString result = time_format_override() && !time_format().isEmpty() ? time_format() : defaultTimeFormat(preferenceLocale());
	qDebug() << "effectiveTimeFormat: override=" << time_format_override()
	         << "stored=" << time_format()
	         << "QLocale().timeFormat()=" << QLocale().timeFormat()
	         << "result=" << result;
	return result;
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
	// AI-generated (Claude): Store the resolved format only when an override is
	// active; store empty string otherwise. This ensures that when "system default"
	// is selected, prefs.time_format / date_format_short remain empty, and
	// effectiveTimeFormat() / effectiveDateFormatShort() fall back to the system
	// locale at call time — correctly picking up the iOS 12/24-hour device toggle
	// rather than a format string baked in at settings-apply time.
	const QString storedLongDate = overrideDate && !longDateFormat.isEmpty() ? longDateFormat : QString();
	const QString storedShortDate = overrideDate && !shortDateFormat.isEmpty() ? shortDateFormat : QString();
	const QString storedTime = overrideTime && !timeFormat.isEmpty() ? timeFormat : QString();
	storeFormats(storedLongDate, storedShortDate, storedTime, overrideDate, overrideTime);
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

QString qPrefLanguage::timeEditText(const QString &displayText) const
{
	const QLocale locale = preferenceLocale();
	const QTime time = locale.toTime(displayText, effectiveTimeFormat());
	return time.isValid() ? locale.toString(time, keypadTimeFormat(effectiveTimeFormat())) : QString();
}

QString qPrefLanguage::timeDisplayText(const QString &editText) const
{
	const QLocale locale = preferenceLocale();
	const QTime time = locale.toTime(editText, keypadTimeFormat(effectiveTimeFormat()));
	return time.isValid() ? locale.toString(time, effectiveTimeFormat()) : QString();
}

QString qPrefLanguage::dateTimeEditText(const QString &displayText) const
{
	const QLocale locale = preferenceLocale();
	const QString timeFormat = effectiveTimeFormat();
	const QString displayFormat = effectiveDateFormatShort() + QLatin1Char(' ') + timeFormat;
	const QDateTime dateTime = locale.toDateTime(displayText, displayFormat);
	const QString editFormat = keypadDateFormat(effectiveDateFormatShort()) + QLatin1Char(' ') + keypadTimeFormat(timeFormat);
	qDebug() << "dateTimeEditText: input=" << displayText
	         << "displayFormat=" << displayFormat
	         << "parsed=" << dateTime
	         << "editFormat=" << editFormat
	         << "result=" << (dateTime.isValid() ? locale.toString(dateTime, editFormat) : QString("PARSE FAILED"));
	return dateTime.isValid() ? locale.toString(dateTime, editFormat) : QString();
}

QString qPrefLanguage::dateTimeDisplayText(const QString &editText) const
{
	const QLocale locale = preferenceLocale();
	const QString editFormat = keypadDateFormat(effectiveDateFormatShort()) + QLatin1Char(' ') + keypadTimeFormat(effectiveTimeFormat());
	const QDateTime dateTime = locale.toDateTime(editText, editFormat);
	const QString displayFormat = effectiveDateFormatShort() + QLatin1Char(' ') + effectiveTimeFormat();
	return dateTime.isValid() ? locale.toString(dateTime, displayFormat) : QString();
}

QString qPrefLanguage::toggleMeridiem(const QString &editText, bool dateTime) const
{
	const QLocale locale = preferenceLocale();
	const QString timeFormat = keypadTimeFormat(effectiveTimeFormat());
	if (!timeFormat.contains(QLatin1String("AP")) && !timeFormat.contains(QLatin1String("ap")))
		return editText;
	if (dateTime) {
		const QString format = keypadDateFormat(effectiveDateFormatShort()) + QLatin1Char(' ') + timeFormat;
		QDateTime value = locale.toDateTime(editText, format);
		if (!value.isValid())
			return editText;
		value.setTime(value.time().addSecs(12 * 60 * 60));
		return locale.toString(value, format);
	}
	QTime value = locale.toTime(editText, timeFormat);
	return value.isValid() ? locale.toString(value.addSecs(12 * 60 * 60), timeFormat) : editText;
}

QString qPrefLanguage::preferenceLocaleName() const
{
	return preferenceLocale().name();
}

void qPrefLanguage::applyLocaleDefaults(const QLocale &locale)
{
	// AI-generated (Claude): When no override is active, store empty string so
	// the effective* helpers resolve dynamically from the system locale at call
	// time, rather than baking in a format string that may not reflect the iOS
	// 12/24-hour device toggle.
	const QString longDate = date_format_override() && !date_format().isEmpty() ? date_format() : QString();
	const QString shortDate = date_format_override() && !date_format_short().isEmpty() ? date_format_short() : QString();
	const QString time = time_format_override() && !time_format().isEmpty() ? time_format() : QString();
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
