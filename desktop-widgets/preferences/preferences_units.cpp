// SPDX-License-Identifier: GPL-2.0
#include "preferences_units.h"
#include "ui_preferences_units.h"
#include "core/qthelper.h"
#include "core/settings/qPrefUnit.h"

PreferencesUnits::PreferencesUnits(): AbstractPreferencesWidget(tr("Units"), QIcon(":units-icon"), 2), ui(new Ui::PreferencesUnits())
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
	ui->show_mdecimal->setChecked(prefs.units.show_mdecimal);
}

void PreferencesUnits::syncSettings()
{
	QString unitSystem[] = {"metric", "imperial", "personal"};
	short unitValue = ui->metric->isChecked() ? METRIC : (ui->imperial->isChecked() ? IMPERIAL : PERSONALIZE);

	qPrefUnits::set_unit_system((unit_system_values)unitValue);
	qPrefUnits::set_temperature(ui->fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS);
	qPrefUnits::set_length(ui->feet->isChecked() ? units::FEET : units::METERS);
	qPrefUnits::set_pressure(ui->psi->isChecked() ? units::PSI : units::BAR);
	qPrefUnits::set_volume(ui->cuft->isChecked() ? units::CUFT : units::LITER);
	qPrefUnits::set_weight(ui->lbs->isChecked() ? units::LBS : units::KG);
	qPrefUnits::set_vertical_speed_time(ui->vertical_speed_minutes->isChecked() ? units::MINUTES : units::SECONDS);
	qPrefUnits::set_coordinates_traditional(ui->gpsTraditional->isChecked());
	qPrefUnits::set_duration_units(ui->duration_mixed->isChecked() ? units::MIXED : (ui->duration_no_hours->isChecked() ? units::MINUTES_ONLY : units::ALWAYS_HOURS));
	qPrefUnits::set_show_units_table(ui->show_units_table->isChecked());
	qPrefUnits::set_show_mdecimal(ui->show_mdecimal->isChecked());
}
