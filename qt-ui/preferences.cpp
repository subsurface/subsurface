#include "preferences.h"
#include "ui_preferences.h"
#include <QSettings>

PreferencesDialog* PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog();
	return dialog;
}

#define B(V, P) s.value(#V, default_prefs.P).toBool()
#define D(V, P) s.value(#V, default_prefs.P).toDouble()
#define I(V, P) s.value(#V, default_prefs.P).toInt()

PreferencesDialog::PreferencesDialog(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f)
, ui(new Ui::PreferencesDialog())
{
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(syncSettings()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(resetSettings()));

	oldPrefs = prefs;

	QSettings s;

	// Graph
	s.beginGroup("TecDetails");
	ui->phe->setChecked(B(phegraph, pp_graphs.phe));
	ui->pheThreshold->setEnabled(ui->phe->isChecked());
	ui->po2->setChecked(B(po2graph, pp_graphs.po2));
	ui->po2Threshold->setEnabled(ui->po2->isChecked());
	ui->pn2->setChecked(B(pn2graph, pp_graphs.pn2));
	ui->pn2Threshold->setEnabled(ui->pn2->isChecked());
	ui->pheThreshold->setValue(D(phethreshold, pp_graphs.phe_threshold));
	ui->po2Threshold->setValue(D(po2threshold, pp_graphs.po2_threshold));
	ui->pn2Threshold->setValue(D(pn2threshold, pp_graphs.pn2_threshold));
	ui->ead_end_eadd->setChecked(B(ead, ead));
	ui->dc_reported_ceiling->setChecked(B(dcceiling, profile_dc_ceiling));
	ui->red_ceiling->setEnabled(ui->dc_reported_ceiling->isChecked());
	ui->red_ceiling->setChecked(B(redceiling, profile_red_ceiling));
	ui->calculated_ceiling->setChecked(B(calcceiling, profile_calc_ceiling));
	ui->increment_3m->setEnabled(ui->calculated_ceiling->isChecked());
	ui->increment_3m->setChecked(B(calcceiling3m, calc_ceiling_3m_incr));

	ui->gflow->setValue((int)(I(gflow, gflow)));
	ui->gfhigh->setValue((int)(I(gfhigh, gfhigh)));
	s.endGroup();

	// Units
	s.beginGroup("Units");
	bool value = s.value("units_metric").toBool();
	ui->metric->setChecked(value);
	ui->imperial->setChecked(!value);

	int unit = s.value("temperature").toInt();
	ui->celsius->setChecked(unit == units::CELSIUS);
	ui->fahrenheit->setChecked(unit == units::FAHRENHEIT);

	unit = s.value("length").toInt();
	ui->meter->setChecked(unit == units::METERS);
	ui->feet->setChecked(unit == units::FEET);

	unit = s.value("pressure").toInt();
	ui->bar->setChecked(unit == units::BAR);
	ui->psi->setChecked(unit == units::PSI);

	unit = s.value("volume").toInt();
	ui->liter->setChecked(unit == units::LITER);
	ui->cuft->setChecked(unit == units::CUFT);

	unit = s.value("weight").toInt();
	ui->kgs->setChecked(unit == units::KG);
	ui->lbs->setChecked(unit == units::LBS);

	s.endGroup();

	// Defaults
	s.beginGroup("GeneralSettings");
	ui->font->setFont( QFont(s.value("table_fonts").toString()));
	ui->fontsize->setValue(D(font_size, font_size));

	ui->defaultfilename->setText(s.value("default_filename").toString());
	ui->displayinvalid->setChecked(B(show_invalid, show_invalid));
	s.endGroup();
}

#undef B
#undef D

void PreferencesDialog::resetSettings()
{
	prefs = oldPrefs;
}

#define SB(V, B) s.setValue(V, (int)(B->isChecked() ? 1 : 0))

void PreferencesDialog::syncSettings()
{
	QSettings s;

	// Graph
	s.beginGroup("TecDetails");

	SB("phegraph", ui->phe);
	SB("po2graph", ui->po2);
	SB("pn2graph", ui->pn2);
	s.setValue("phethreshold", ui->pheThreshold->value());
	s.setValue("po2threshold", ui->po2Threshold->value());
	s.setValue("pn2threshold", ui->pn2Threshold->value());
	SB("ead", ui->ead_end_eadd);
	SB("dcceiling", ui->dc_reported_ceiling);
	SB("redceiling", ui->red_ceiling);
	SB("calcceiling", ui->calculated_ceiling);
	SB("calcceiling3m", ui->increment_3m);
	s.setValue("gflow", ui->gflow->value());
	s.setValue("gfhigh", ui->gfhigh->value());
	s.endGroup();

	// Units
	s.beginGroup("Units");
	s.setValue("units_metric", ui->metric->isChecked());
	s.setValue("temperature", ui->fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS);
	s.setValue("length", ui->feet->isChecked() ? units::FEET : units::METERS);
	s.setValue("pressure", ui->psi->isChecked() ? units::PSI : units::BAR);
	s.setValue("volume", ui->cuft->isChecked() ? units::CUFT : units::LITER);
	s.setValue("weight", ui->lbs->isChecked() ? units::LBS : units::KG);
	s.endGroup();
	// Defaults
	s.beginGroup("GeneralSettings");
	s.value("table_fonts", ui->font->font().family());
	s.value("font_size", ui->fontsize->value());
	s.value("default_filename", ui->defaultfilename->text());
	s.value("displayinvalid", ui->displayinvalid->isChecked());
	s.endGroup();
	s.sync();

	oldPrefs = prefs;
	emit settingsChanged();
}

#undef SB
