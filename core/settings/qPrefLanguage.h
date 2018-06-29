// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLANGUAGE_H
#define QPREFLANGUAGE_H

#include <QObject>


class qPrefLanguage : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString date_format READ dateFormat WRITE setDateFormat NOTIFY dateFormatChanged);
	Q_PROPERTY(bool date_format_override READ dateFormatOverride WRITE setDateFormatOverride NOTIFY dateFormatOverrideChanged);
	Q_PROPERTY(QString date_format_short READ dateFormatShort WRITE setDateFormatShort NOTIFY dateFormatShortChanged);
	Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged);
	Q_PROPERTY(QString lang_locale READ langLocale WRITE setLangLocale NOTIFY langLocaleChanged);
	Q_PROPERTY(QString time_format READ timeFormat WRITE setTimeFormat NOTIFY timeFormatChanged);
	Q_PROPERTY(bool time_format_override READ timeFormatOverride WRITE setTimeFormatOverride NOTIFY timeFormatOverrideChanged);
	Q_PROPERTY(bool use_system_language READ useSystemLanguage WRITE setUseSystemLanguage NOTIFY useSystemLanguageChanged);

public:
	qPrefLanguage(QObject *parent = NULL) : QObject(parent) {};
	~qPrefLanguage() {};
	static qPrefLanguage *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	const QString dateFormat() const;
	bool dateFormatOverride() const;
	const QString dateFormatShort() const;
	const QString language() const;
	const QString langLocale() const;
	const QString timeFormat() const;
	bool timeFormatOverride() const;
	bool useSystemLanguage() const;

public slots:
	void setDateFormat(const QString& value);
	void setDateFormatOverride(bool value);
	void setDateFormatShort(const QString& value);
	void setLanguage(const QString& value);
	void setLangLocale(const QString& value);
	void setTimeFormat(const QString& value);
	void setTimeFormatOverride(bool value);
	void setUseSystemLanguage(bool value);

signals:
	void dateFormatChanged(const QString& value);
	void dateFormatOverrideChanged(bool value);
	void dateFormatShortChanged(const QString& value);
	void languageChanged(const QString& value);
	void langLocaleChanged(const QString& value);
	void timeFormatChanged(const QString& value);
	void timeFormatOverrideChanged(bool value);
	void useSystemLanguageChanged(bool value);


private:
	const QString group = QStringLiteral("Language");
	static qPrefLanguage *m_instance;

	void diskDateFormat(bool doSync);
	void diskDateFormatOverride(bool doSync);
	void diskDateFormatShort(bool doSync);
	void diskLanguage(bool doSync);
	void diskLangLocale(bool doSync);
	void diskTimeFormat(bool doSync);
	void diskTimeFormatOverride(bool doSync);
	void diskUseSystemLanguage(bool doSync);
};

#endif
