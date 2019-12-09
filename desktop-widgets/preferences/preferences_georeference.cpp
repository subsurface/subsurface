// SPDX-License-Identifier: GPL-2.0
#include "preferences_georeference.h"
#include "ui_preferences_georeference.h"
#include "core/qthelper.h"
#include "core/settings/qPrefGeocoding.h"
#include "qt-models/divelocationmodel.h"

#include <ctime>

PreferencesGeoreference::PreferencesGeoreference() : AbstractPreferencesWidget(tr("Georeference"), QIcon(":geotag-icon"), 8)
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
	ui->first_item->setCurrentIndex(prefs.geocoding.category[0]);
	ui->second_item->setCurrentIndex(prefs.geocoding.category[1]);
	ui->third_item->setCurrentIndex(prefs.geocoding.category[2]);
}

void PreferencesGeoreference::syncSettings()
{
	qPrefGeocoding::set_first_taxonomy_category((taxonomy_category) ui->first_item->currentIndex());
	qPrefGeocoding::set_second_taxonomy_category((taxonomy_category) ui->second_item->currentIndex());
	qPrefGeocoding::set_third_taxonomy_category((taxonomy_category) ui->third_item->currentIndex());
}
