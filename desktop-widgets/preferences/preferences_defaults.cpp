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
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	// Item data is set here because uic does not support user data for combo box items
	ui->colorScheme->addItem(tr("Use system"), QStringLiteral("system"));
	ui->colorScheme->addItem(tr("Light"), QStringLiteral("light"));
	ui->colorScheme->addItem(tr("Dark"), QStringLiteral("dark"));
#else
	ui->colorSchemeGroup->hide();
#endif
}

PreferencesDefaults::~PreferencesDefaults()
{
	delete ui;
}

void PreferencesDefaults::refreshSettings()
{
	ui->font->setCurrentFont(qPrefDisplay::divelist_font());
	ui->fontsize->setValue(qPrefDisplay::font_size());
	ui->velocitySlider->setValue(qPrefDisplay::animation_speed());

	if (qPrefDisplay::three_m_based_grid())
		ui->grid3MBased->setChecked(true);
	else
		ui->gridGeneric->setChecked(true);

	ui->checkBox_map_short_names->setChecked(qPrefDisplay::map_short_names());
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	ui->colorScheme->setCurrentIndex(ui->colorScheme->findData(qPrefDisplay::color_scheme()));
#endif
}

void PreferencesDefaults::syncSettings()
{
	qPrefDisplay::set_divelist_font(ui->font->currentFont().toString());
	qPrefDisplay::set_font_size(ui->fontsize->value());
	qPrefDisplay::set_animation_speed(ui->velocitySlider->value());
	qPrefDisplay::set_three_m_based_grid(ui->grid3MBased->isChecked());
	qPrefDisplay::set_map_short_names(ui->checkBox_map_short_names->isChecked());
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	qPrefDisplay::set_color_scheme(ui->colorScheme->currentData().toString());
#endif
}
