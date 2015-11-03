#include "preferences_defaults.h"
#include "ui_preferences_defaults.h"
#include "dive.h"
#include "subsurface-core/prefs-macros.h"

#include <QSettings>
#include <QFileDialog>

PreferencesDefaults::PreferencesDefaults(): AbstractPreferencesWidget(tr("Defaults"), QIcon(":defaults"), 0 ), ui(new Ui::PreferencesDefaults())
{
	ui->setupUi(this);
}

PreferencesDefaults::~PreferencesDefaults()
{
	delete ui;
}

void PreferencesDefaults::on_chooseFile_clicked()
{
	QFileInfo fi(system_default_filename());
	QString choosenFileName = QFileDialog::getOpenFileName(this, tr("Open default log file"), fi.absolutePath(), tr("Subsurface XML files (*.ssrf *.xml *.XML)"));

	if (!choosenFileName.isEmpty())
		ui->defaultfilename->setText(choosenFileName);
}

void PreferencesDefaults::on_btnUseDefaultFile_toggled(bool toggle)
{
	if (toggle) {
		ui->defaultfilename->setText(system_default_filename());
		ui->defaultfilename->setEnabled(false);
	} else {
		ui->defaultfilename->setEnabled(true);
	}
}

void PreferencesDefaults::on_localDefaultFile_toggled(bool toggle)
{
	ui->defaultfilename->setEnabled(toggle);
	ui->btnUseDefaultFile->setEnabled(toggle);
	ui->chooseFile->setEnabled(toggle);
}

void PreferencesDefaults::refreshSettings()
{
	ui->font->setCurrentFont(QString(prefs.divelist_font));
	ui->fontsize->setValue(prefs.font_size);
	ui->defaultfilename->setText(prefs.default_filename);
	ui->noDefaultFile->setChecked(prefs.default_file_behavior == NO_DEFAULT_FILE);
	ui->cloudDefaultFile->setChecked(prefs.default_file_behavior == CLOUD_DEFAULT_FILE);
	ui->localDefaultFile->setChecked(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);

	ui->default_cylinder->clear();
	for (int i = 0; tank_info[i].name != NULL; i++) {
		ui->default_cylinder->addItem(tank_info[i].name);
		if (prefs.default_cylinder && strcmp(tank_info[i].name, prefs.default_cylinder) == 0)
			ui->default_cylinder->setCurrentIndex(i);
	}
	ui->displayinvalid->setChecked(prefs.display_invalid_dives);
	ui->velocitySlider->setValue(prefs.animation_speed);
	ui->btnUseDefaultFile->setChecked(prefs.use_default_file);

	if (prefs.cloud_verification_status == CS_VERIFIED) {
		ui->cloudDefaultFile->setEnabled(true);
	} else {
		if (ui->cloudDefaultFile->isChecked())
			ui->noDefaultFile->setChecked(true);
		ui->cloudDefaultFile->setEnabled(false);
	}

	ui->defaultfilename->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	ui->btnUseDefaultFile->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
	ui->chooseFile->setEnabled(prefs.default_file_behavior == LOCAL_DEFAULT_FILE);
}

void PreferencesDefaults::syncSettings()
{
	QSettings s;
	s.beginGroup("GeneralSettings");
	s.setValue("default_filename", ui->defaultfilename->text());
	s.setValue("default_cylinder", ui->default_cylinder->currentText());
	s.setValue("use_default_file", ui->btnUseDefaultFile->isChecked());
	if (ui->noDefaultFile->isChecked())
		s.setValue("default_file_behavior", NO_DEFAULT_FILE);
	else if (ui->localDefaultFile->isChecked())
		s.setValue("default_file_behavior", LOCAL_DEFAULT_FILE);
	else if (ui->cloudDefaultFile->isChecked())
		s.setValue("default_file_behavior", CLOUD_DEFAULT_FILE);
	s.endGroup();

	s.beginGroup("Display");
	SAVE_OR_REMOVE_SPECIAL("divelist_font", system_divelist_default_font, ui->font->currentFont().toString(), ui->font->currentFont());
	SAVE_OR_REMOVE("font_size", system_divelist_default_font_size, ui->fontsize->value());
	s.setValue("displayinvalid", ui->displayinvalid->isChecked());
	s.endGroup();
	s.sync();

		// Animation
	s.beginGroup("Animations");
	s.setValue("animation_speed", ui->velocitySlider->value());
	s.endGroup();
}
