#include "preferences_units.h"
#include "ui_preferences_units.h"
#include "prefs-macros.h"
#include "qthelper.h"

#include <QSettings>

PreferencesUnits::PreferencesUnits(): AbstractPreferencesWidget(tr("Units"),QIcon(":units"),1), ui(new Ui::PreferencesUnits())
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
}

void PreferencesUnits::syncSettings()
{
	QSettings s;
	s.beginGroup("Units");
	QString unitSystem[] = {"metric", "imperial", "personal"};
	short unitValue = ui->metric->isChecked() ? METRIC : (ui->imperial->isChecked() ? IMPERIAL : PERSONALIZE);
	SAVE_OR_REMOVE_SPECIAL("unit_system", default_prefs.unit_system, unitValue, unitSystem[unitValue]);
	s.setValue("temperature", ui->fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS);
	s.setValue("length", ui->feet->isChecked() ? units::FEET : units::METERS);
	s.setValue("pressure", ui->psi->isChecked() ? units::PSI : units::BAR);
	s.setValue("volume", ui->cuft->isChecked() ? units::CUFT : units::LITER);
	s.setValue("weight", ui->lbs->isChecked() ? units::LBS : units::KG);
	s.setValue("vertical_speed_time", ui->vertical_speed_minutes->isChecked() ? units::MINUTES : units::SECONDS);
	s.setValue("coordinates", ui->gpsTraditional->isChecked());
	s.endGroup();
}
