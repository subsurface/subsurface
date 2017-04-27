// SPDX-License-Identifier: GPL-2.0
#include "preferences_georeference.h"
#include "ui_prefs_georeference.h"
#include "core/prefs-macros.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include "qt-models/divelocationmodel.h"

#include <ctime>

PreferencesGeoreference::PreferencesGeoreference() : AbstractPreferencesWidget(tr("Georeference"), QIcon(":/georeference"), 9)
{
	ui = new Ui::PreferencesGeoreference();
	ui->setupUi(this);
	ui->first_item->setModel(GeoReferencingOptionsModel::instance());
	ui->second_item->setModel(GeoReferencingOptionsModel::instance());
	ui->third_item->setModel(GeoReferencingOptionsModel::instance());
}

PreferencesGeoreference::~PreferencesGeoreference()
{
	delete ui;
}

void PreferencesGeoreference::refreshSettings()
{
	ui->enable_geocoding->setChecked(prefs.geocoding.enable_geocoding);
	ui->parse_without_gps->setChecked(prefs.geocoding.parse_dive_without_gps);
	ui->tag_existing_dives->setChecked(prefs.geocoding.tag_existing_dives);
	ui->first_item->setCurrentIndex(prefs.geocoding.category[0]);
	ui->second_item->setCurrentIndex(prefs.geocoding.category[1]);
	ui->third_item->setCurrentIndex(prefs.geocoding.category[2]);
}

void PreferencesGeoreference::syncSettings()
{
	auto geocoding = SettingsObjectWrapper::instance()->geocoding;
	geocoding->setEnableGeocoding(ui->enable_geocoding->isChecked());
	geocoding->setParseDiveWithoutGps(ui->parse_without_gps->isChecked());
	geocoding->setTagExistingDives(ui->tag_existing_dives->isChecked());
	geocoding->setFirstTaxonomyCategory((taxonomy_category) ui->first_item->currentIndex());
	geocoding->setSecondTaxonomyCategory((taxonomy_category) ui->second_item->currentIndex());
	geocoding->setThirdTaxonomyCategory((taxonomy_category) ui->third_item->currentIndex());
}
