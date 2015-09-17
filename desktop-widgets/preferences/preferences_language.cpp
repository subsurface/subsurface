#include "preferences_language.h"
#include "ui_prefs_language.h"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>


PreferencesLanguage::PreferencesLanguage() : AbstractPreferencesWidget(tr("Language"), QIcon(":/language"), 4)
{
	ui = new Ui::PreferencesLanguage();
	ui->setupUi(this);
}

PreferencesLanguage::~PreferencesLanguage()
{
	delete ui;
}

void PreferencesLanguage::refreshSettings()
{
	QSettings s;
	s.beginGroup("Language");
	ui->languageSystemDefault->setChecked(s.value("UseSystemLanguage", true).toBool());
	QAbstractItemModel *m = ui->languageView->model();
	QModelIndexList languages = m->match(m->index(0, 0), Qt::UserRole, s.value("UiLanguage").toString());
	if (languages.count())
		ui->languageView->setCurrentIndex(languages.first());
	s.endGroup();
}

void PreferencesLanguage::syncSettings()
{
	// Locale
	QLocale loc;
	QSettings s;
	s.beginGroup("Language");
	bool useSystemLang = s.value("UseSystemLanguage", true).toBool();
	if (useSystemLang != ui->languageSystemDefault->isChecked() ||
	    (!useSystemLang && s.value("UiLanguage").toString() != ui->languageView->currentIndex().data(Qt::UserRole))) {
		QMessageBox::warning(this, tr("Restart required"),
			tr("To correctly load a new language you must restart Subsurface."));
	}
	s.setValue("UseSystemLanguage", ui->languageSystemDefault->isChecked());
	s.setValue("UiLanguage", ui->languageView->currentIndex().data(Qt::UserRole));
	s.endGroup();
}