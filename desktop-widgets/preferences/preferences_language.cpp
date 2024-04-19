// SPDX-License-Identifier: GPL-2.0
#include "preferences_language.h"
#include "ui_preferences_language.h"
#include "core/qthelper.h"
#include "core/settings/qPrefLanguage.h"

#include <QDir>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QCompleter>

#include "qt-models/models.h"

PreferencesLanguage::PreferencesLanguage() : AbstractPreferencesWidget(tr("Language"), QIcon(":preferences-desktop-locale-icon"), 1)
{
	ui = new Ui::PreferencesLanguage();
	ui->setupUi(this);

	QSortFilterProxyModel *filterModel = new QSortFilterProxyModel();
	filterModel->setSourceModel(LanguageModel::instance());
	filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	ui->languageDropdown->setModel(filterModel);
	filterModel->sort(0);
	connect(ui->languageFilter, &QLineEdit::textChanged,
			filterModel, &QSortFilterProxyModel::setFilterFixedString);

	dateFormatShortMap.insert("MM/dd/yyyy", "M/d/yy");
	dateFormatShortMap.insert("dd.MM.yyyy", "d.M.yy");
	dateFormatShortMap.insert("yyyy-MM-dd", "yy-M-d");
	for (const QString &format: dateFormatShortMap.keys())
		ui->dateFormatEntry->addItem(format);
	ui->dateFormatEntry->completer()->setCaseSensitivity(Qt::CaseSensitive);
	connect(ui->dateFormatEntry, &QComboBox::currentTextChanged,
		this, &PreferencesLanguage::dateFormatChanged);

	ui->timeFormatEntry->addItem("hh:mm");
	ui->timeFormatEntry->addItem("h:mm AP");
	ui->timeFormatEntry->addItem("hh:mm AP");
	ui->timeFormatEntry->completer()->setCaseSensitivity(Qt::CaseSensitive);
}

PreferencesLanguage::~PreferencesLanguage()
{
	delete ui;
}

void PreferencesLanguage::dateFormatChanged(const QString &text)
{
	ui->shortDateFormatEntry->setText(dateFormatShortMap.value(text));
}

void PreferencesLanguage::refreshSettings()
{
	ui->languageSystemDefault->setChecked(prefs.locale.use_system_language);
	ui->timeFormatSystemDefault->setChecked(!prefs.time_format_override);
	ui->dateFormatSystemDefault->setChecked(!prefs.date_format_override);
	ui->timeFormatEntry->setCurrentText(prefs.time_format);
	ui->dateFormatEntry->setCurrentText(prefs.date_format);
	ui->shortDateFormatEntry->setText(prefs.date_format_short);
	QAbstractItemModel *m = ui->languageDropdown->model();
	QModelIndexList languages = m->match(m->index(0, 0), Qt::UserRole, QString(prefs.locale.lang_locale).replace("-", "_"));
	if (languages.count())
		ui->languageDropdown->setCurrentIndex(languages.first().row());
}

void PreferencesLanguage::syncSettings()
{
	bool useSystemLang = prefs.locale.use_system_language;
	QString currentText = ui->languageDropdown->currentText();

	if (useSystemLang != ui->languageSystemDefault->isChecked() ||
		(!useSystemLang && currentText != prefs.locale.language)) {
		// remove the googlemaps cache folder on language change
		QDir googlecachedir(QString(system_default_directory()).append("/googlemaps"));
		googlecachedir.removeRecursively();

		QMessageBox::warning(this, tr("Restart required"),
			tr("To correctly load a new language you must restart Subsurface."));
	}
	QAbstractItemModel *m = ui->languageDropdown->model();
	QModelIndexList languages = m->match(m->index(0, 0), Qt::DisplayRole, currentText);
	QString currentLocale;
	if (languages.count())
		currentLocale = m->data(languages.first(),Qt::UserRole).toString();

	qPrefLanguage::set_language(currentText);
	qPrefLanguage::set_lang_locale(currentLocale);
	qPrefLanguage::set_use_system_language(ui->languageSystemDefault->isChecked());
	qPrefLanguage::set_time_format_override(!ui->timeFormatSystemDefault->isChecked());
	qPrefLanguage::set_date_format_override(!ui->dateFormatSystemDefault->isChecked());
	qPrefLanguage::set_time_format(ui->timeFormatEntry->currentText());
	qPrefLanguage::set_date_format(ui->dateFormatEntry->currentText());
	qPrefLanguage::set_date_format_short(ui->shortDateFormatEntry->text());
	initUiLanguage();

	// When switching to system defaults, initUiLanguage() will reset the date and time formats.
	// Therefore, refresh the UI fields to give the user a visual feedback of the new formats.
	refreshSettings();

	QString qDateTimeWeb = tr("These will be used as is. This might not be what you intended. To avoid this warning wrap the literal parts in quotes (').\nSee https://doc.qt.io/archives/qt-4.8/qdatetime.html#fromString");
	QRegularExpression timeStandardFormat("^([hHmszaApP\\s:\\.]|'[^']*')*$");
	if (!timeStandardFormat.match(ui->timeFormatEntry->currentText()).hasMatch())
		QMessageBox::warning(this, tr("Literal characters"),
			tr("Non-standard character(s) in time format.\n") + qDateTimeWeb);

	QRegularExpression dateStandardFormat("^([dMy\\s/\\.,\\-]|'[^']*')*$");
	if (!dateStandardFormat.match(ui->dateFormatEntry->currentText()).hasMatch() ||
	    !dateStandardFormat.match(ui->shortDateFormatEntry->text()).hasMatch())
		QMessageBox::warning(this, tr("Literal characters"),
			tr("Non-standard character(s) in date format.\n") + qDateTimeWeb);
}
