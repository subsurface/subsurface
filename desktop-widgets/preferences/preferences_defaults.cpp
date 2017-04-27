// SPDX-License-Identifier: GPL-2.0
#include "preferences_defaults.h"
#include "ui_preferences_defaults.h"
#include "core/dive.h"
#include "core/prefs-macros.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"

#include <QFileDialog>

PreferencesDefaults::PreferencesDefaults(): AbstractPreferencesWidget(tr("General"), QIcon(":defaults"), 0 ), ui(new Ui::PreferencesDefaults())
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
	auto general = SettingsObjectWrapper::instance()->general_settings;
	general->setDefaultFilename(ui->defaultfilename->text());
	general->setDefaultCylinder(ui->default_cylinder->currentText());
	general->setUseDefaultFile(ui->btnUseDefaultFile->isChecked());
	if (ui->noDefaultFile->isChecked())
		general->setDefaultFileBehavior(NO_DEFAULT_FILE);
	else if (ui->localDefaultFile->isChecked())
		general->setDefaultFileBehavior(LOCAL_DEFAULT_FILE);
	else if (ui->cloudDefaultFile->isChecked())
		general->setDefaultFileBehavior(CLOUD_DEFAULT_FILE);

	auto display =  SettingsObjectWrapper::instance()->display_settings;
	display->setDivelistFont(ui->font->currentFont().toString());
	display->setFontSize(ui->fontsize->value());
	display->setDisplayInvalidDives(ui->displayinvalid->isChecked());

	auto animation = SettingsObjectWrapper::instance()->animation_settings;
	animation->setAnimationSpeed(ui->velocitySlider->value());
}
