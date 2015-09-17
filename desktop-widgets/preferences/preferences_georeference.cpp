#include "preferences_georeference.h"
#include "ui_prefs_georeference.h"
#include "prefs-macros.h"
#include "qthelper.h"
#include "qt-models/divelocationmodel.h"

#include <ctime>
#include <QSettings>

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
	QSettings s;
	s.beginGroup("geocoding");
	s.setValue("enable_geocoding", ui->enable_geocoding->isChecked());
	s.setValue("parse_dives_without_gps", ui->parse_without_gps->isChecked());
	s.setValue("tag_existing_dives", ui->tag_existing_dives->isChecked());
	s.setValue("cat0", ui->first_item->currentIndex());
	s.setValue("cat1", ui->second_item->currentIndex());
	s.setValue("cat2", ui->third_item->currentIndex());
	s.endGroup();
}