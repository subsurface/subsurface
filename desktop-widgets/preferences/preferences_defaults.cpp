// SPDX-License-Identifier: GPL-2.0
#include "preferences_defaults.h"
#include "ui_preferences_defaults.h"
#include "core/dive.h"
#include "preferencesdialog.h"
#include "core/settings/qPrefGeneral.h"
#include "core/settings/qPrefDisplay.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/settings/qPrefDiveComputer.h"

#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>

PreferencesDefaults::PreferencesDefaults(): AbstractPreferencesWidget(tr("Display"), QIcon(":preferences-display-icon"), 0 ), ui(new Ui::PreferencesDefaults())
{
	ui->setupUi(this);
}

PreferencesDefaults::~PreferencesDefaults()
{
	delete ui;
}

void PreferencesDefaults::refreshSettings()
{
	prefs.darkmode_colour == BLACK ? ui->black_text->setChecked(true) : (prefs.darkmode_colour == LIGHTBLUE ? ui->lightblue_text->setChecked(true) : ui->mediumblue_text->setChecked(true));
	ui->font->setCurrentFont(qPrefDisplay::divelist_font());
	ui->fontsize->setValue(qPrefDisplay::font_size());
	ui->velocitySlider->setValue(qPrefDisplay::animation_speed());

}

void PreferencesDefaults::syncSettings()
{
	qPrefDisplay::set_divelist_font(ui->font->currentFont().toString());
	qPrefDisplay::set_font_size(ui->fontsize->value());
	qPrefDisplay::set_animation_speed(ui->velocitySlider->value());
	qPrefDisplay::set_darkmode_colour(ui->black_text->isChecked() ? BLACK : (ui->lightblue_text->isChecked() ? LIGHTBLUE : MEDIUMBLUE));

}
