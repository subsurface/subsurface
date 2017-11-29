// SPDX-License-Identifier: GPL-2.0
#include "preferences_units.h"
#include "ui_preferences_units.h"
#include "core/prefs-macros.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"

PreferencesUnits::PreferencesUnits(): AbstractPreferencesWidget(tr("Units"),QIcon(":units-icon"),1), ui(new Ui::PreferencesUnits())
{
	ui->setupUi(this);
}

PreferencesUnits::~PreferencesUnits()
{

}

void PreferencesUnits::refreshSettings()
{
	switch(prefs.unit_system) {
		case METRIC: ui->metric->setChecked(true); break;
		case IMPERIAL: ui->imperial->setChecked(true); break;
		default: ui->personalize->setChecked(true); break;
	}

	ui->gpsTraditional->setChecked(prefs.coordinates_traditional);
	ui->gpsDecimal->setChecked(!prefs.coordinates_traditional);

	ui->celsius->setChecked(prefs.units.temperature == units::CELSIUS);
	ui->fahrenheit->setChecked(prefs.units.temperature == units::FAHRENHEIT);
	ui->meter->setChecked(prefs.units.length == units::METERS);
	ui->feet->setChecked(prefs.units.length == units::FEET);
	ui->bar->setChecked(prefs.units.pressure == units::BAR);
	ui->psi->setChecked(prefs.units.pressure == units::PSI);
	ui->liter->setChecked(prefs.units.volume == units::LITER);
	ui->cuft->setChecked(prefs.units.volume == units::CUFT);
	ui->kg->setChecked(prefs.units.weight == units::KG);
	ui->lbs->setChecked(prefs.units.weight == units::LBS);
	ui->units_group->setEnabled(ui->personalize->isChecked());

	ui->vertical_speed_minutes->setChecked(prefs.units.vertical_speed_time == units::MINUTES);
	ui->vertical_speed_seconds->setChecked(prefs.units.vertical_speed_time == units::SECONDS);
	ui->duration_mixed->setChecked(prefs.units.duration_units == units::MIXED);
	ui->duration_no_hours->setChecked(prefs.units.duration_units == units::MINUTES_ONLY);
	ui->duration_show_hours->setChecked(prefs.units.duration_units == units::ALWAYS_HOURS);
	ui->show_units_table->setChecked(prefs.units.show_units_table);
}

void PreferencesUnits::syncSettings()
{
	auto units = SettingsObjectWrapper::instance()->unit_settings;
	QString unitSystem[] = {"metric", "imperial", "personal"};
	short unitValue = ui->metric->isChecked() ? METRIC : (ui->imperial->isChecked() ? IMPERIAL : PERSONALIZE);

	units->setUnitSystem(unitSystem[unitValue]);
	units->setTemperature(ui->fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS);
	units->setLength(ui->feet->isChecked() ? units::FEET : units::METERS);
	units->setPressure(ui->psi->isChecked() ? units::PSI : units::BAR);
	units->setVolume(ui->cuft->isChecked() ? units::CUFT : units::LITER);
	units->setWeight(ui->lbs->isChecked() ? units::LBS : units::KG);
	units->setVerticalSpeedTime(ui->vertical_speed_minutes->isChecked() ? units::MINUTES : units::SECONDS);
	units->setCoordinatesTraditional(ui->gpsTraditional->isChecked());
	units->setDurationUnits(ui->duration_mixed->isChecked() ? units::MIXED : (ui->duration_no_hours->isChecked() ? units::MINUTES_ONLY : units::ALWAYS_HOURS));
	units->setShowUnitsTable(ui->show_units_table->isChecked());
}
