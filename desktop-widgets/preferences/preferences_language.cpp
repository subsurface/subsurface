#include "preferences_language.h"
#include "ui_prefs_language.h"
#include "core/helpers.h"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include "qt-models/models.h"

PreferencesLanguage::PreferencesLanguage() : AbstractPreferencesWidget(tr("Language"), QIcon(":/language"), 4)
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
}

PreferencesLanguage::~PreferencesLanguage()
{
	delete ui;
}

void PreferencesLanguage::refreshSettings()
{
	ui->languageSystemDefault->setChecked(prefs.locale.use_system_language);
	ui->timeFormatSystemDefault->setChecked(!prefs.time_format_override);
	ui->dateFormatSystemDefault->setChecked(!prefs.date_format_override);
	ui->timeFormatEntry->setText(prefs.time_format);
	ui->dateFormatEntry->setText(prefs.date_format);
	ui->shortDateFormatEntry->setText(prefs.date_format_short);
	QAbstractItemModel *m = ui->languageDropdown->model();
	QModelIndexList languages = m->match(m->index(0, 0), Qt::UserRole, prefs.locale.language);
	if (languages.count())
		ui->languageDropdown->setCurrentIndex(languages.first().row());
}

void PreferencesLanguage::syncSettings()
{
	QSettings s;
	s.beginGroup("Language");
	bool useSystemLang = s.value("UseSystemLanguage", true).toBool();
	QAbstractItemModel *m = ui->languageDropdown->model();
	QString currentText = m->data(m->index(ui->languageDropdown->currentIndex(),0), Qt::UserRole).toString();
	if (useSystemLang != ui->languageSystemDefault->isChecked() ||
	    (!useSystemLang && s.value("UiLanguage").toString() != currentText)) {
		QMessageBox::warning(this, tr("Restart required"),
			tr("To correctly load a new language you must restart Subsurface."));
	}
	s.setValue("UiLanguage", currentText);
	s.setValue("UseSystemLanguage", ui->languageSystemDefault->isChecked());
	s.setValue("time_format_override", !ui->timeFormatSystemDefault->isChecked());
	s.setValue("date_format_override", !ui->dateFormatSystemDefault->isChecked());
	if (!ui->timeFormatSystemDefault->isChecked())
		s.setValue("time_format", ui->timeFormatEntry->text());
	if (!ui->dateFormatSystemDefault->isChecked()) {
		s.setValue("date_format", ui->dateFormatEntry->text());
		s.setValue("date_format_short", ui->shortDateFormatEntry->text());
	}
	s.endGroup();
	uiLanguage(NULL);
}
