#include "preferences.h"
#include "ui_preferences.h"
#include <QSettings>
#include <QDebug>

PreferencesDialog* PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog();
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f),
	ui(new Ui::PreferencesDialog())
{
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	setUiFromPrefs();
	rememberPrefs();
}

void PreferencesDialog::showEvent(QShowEvent *event)
{
	setUiFromPrefs();
	rememberPrefs();
	QDialog::showEvent(event);
}

void PreferencesDialog::setUiFromPrefs()
{
	// graphs
	ui->phe->setChecked(prefs.pp_graphs.phe);
	ui->pheThreshold->setEnabled(ui->phe->isChecked());
	ui->po2->setChecked(prefs.pp_graphs.po2);
	ui->po2Threshold->setEnabled(ui->po2->isChecked());
	ui->pn2->setChecked(prefs.pp_graphs.pn2);
	ui->pn2Threshold->setEnabled(ui->pn2->isChecked());
	ui->pheThreshold->setValue(prefs.pp_graphs.phe_threshold);
	ui->po2Threshold->setValue(prefs.pp_graphs.po2_threshold);
	ui->pn2Threshold->setValue(prefs.pp_graphs.pn2_threshold);
	ui->ead_end_eadd->setChecked(prefs.ead);
	ui->dc_reported_ceiling->setChecked(prefs.profile_dc_ceiling);
	ui->red_ceiling->setEnabled(ui->dc_reported_ceiling->isChecked());
	ui->red_ceiling->setChecked(prefs.profile_red_ceiling);
	ui->calculated_ceiling->setChecked(prefs.profile_calc_ceiling);
	ui->increment_3m->setEnabled(ui->calculated_ceiling->isChecked());
	ui->increment_3m->setChecked(prefs.calc_ceiling_3m_incr);
	ui->all_tissues->setEnabled(ui->calculated_ceiling->isChecked());
	ui->all_tissues->setChecked(prefs.calc_all_tissues);
	ui->groupBox->setEnabled(ui->personalize->isChecked());

	ui->gflow->setValue(prefs.gflow);
	ui->gfhigh->setValue(prefs.gfhigh);

	// units
	if (prefs.unit_system == METRIC)
		ui->metric->setChecked(true);
	else if (prefs.unit_system == IMPERIAL)
		ui->imperial->setChecked(true);
	else
		ui->personalize->setChecked(true);

	ui->celsius->setChecked(prefs.units.temperature == units::CELSIUS);
	ui->fahrenheit->setChecked(prefs.units.temperature == units::FAHRENHEIT);
	ui->meter->setChecked(prefs.units.length == units::METERS);
	ui->feet->setChecked(prefs.units.length == units::FEET);
	ui->bar->setChecked(prefs.units.pressure == units::BAR);
	ui->psi->setChecked(prefs.units.pressure == units::PSI);
	ui->liter->setChecked(prefs.units.volume == units::LITER);
	ui->cuft->setChecked(prefs.units.volume == units::CUFT);
	ui->kgs->setChecked(prefs.units.weight == units::KG);
	ui->lbs->setChecked(prefs.units.weight == units::LBS);
	ui->font->setFont(QString(prefs.divelist_font));
	ui->fontsize->setValue(prefs.font_size);
	ui->defaultfilename->setText(prefs.default_filename);
	ui->displayinvalid->setChecked(prefs.show_invalid);
}

void PreferencesDialog::restorePrefs()
{
	prefs = oldPrefs;
}

void PreferencesDialog::rememberPrefs()
{
	oldPrefs = prefs;
}

#define SP(V, B) prefs.V = (int)(B->isChecked() ? 1 : 0)

void PreferencesDialog::setPrefsFromUi()
{
	SP(pp_graphs.phe, ui->phe);
	SP(pp_graphs.po2, ui->po2);
	SP(pp_graphs.pn2, ui->pn2);
	prefs.pp_graphs.phe_threshold = ui->pheThreshold->value();
	prefs.pp_graphs.po2_threshold = ui->po2Threshold->value();
	prefs.pp_graphs.pn2_threshold = ui->pn2Threshold->value();
	SP(ead, ui->ead_end_eadd);
	SP(profile_dc_ceiling, ui->dc_reported_ceiling);
	SP(profile_red_ceiling, ui->red_ceiling);
	SP(profile_calc_ceiling, ui->calculated_ceiling);
	SP(calc_ceiling_3m_incr, ui->increment_3m);
	SP(calc_all_tissues, ui->all_tissues);
	prefs.gflow = ui->gflow->value();
	prefs.gfhigh = ui->gfhigh->value();
	prefs.unit_system = ui->metric->isChecked() ? METRIC : (ui->imperial->isChecked() ? IMPERIAL : PERSONALIZE);
	prefs.units.temperature = ui->fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS;
	prefs.units.length = ui->feet->isChecked() ? units::FEET : units::METERS;
	prefs.units.pressure = ui->psi->isChecked() ? units::PSI : units::BAR;
	prefs.units.volume = ui->cuft->isChecked() ? units::CUFT : units::LITER;
	prefs.units.weight = ui->lbs->isChecked() ? units::LBS : units::KG;
	prefs.divelist_font = strdup(ui->font->font().family().toUtf8().data());
	prefs.font_size = ui->fontsize->value();
	prefs.default_filename = strdup(ui->defaultfilename->text().toUtf8().data());
	prefs.display_invalid_dives = ui->displayinvalid->isChecked();
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
	SB("calcalltissues", ui->all_tissues);
	s.setValue("gflow", ui->gflow->value());
	s.setValue("gfhigh", ui->gfhigh->value());
	s.endGroup();

	// Units
	s.beginGroup("Units");
	QString unitSystem = ui->metric->isChecked() ? "metric" : (ui->imperial->isChecked() ? "imperial" : "personal");
	s.setValue("unit_system", unitSystem);
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

	emit settingsChanged();
}

void PreferencesDialog::buttonClicked(QAbstractButton* button)
{
	switch(ui->buttonBox->standardButton(button)){
	case QDialogButtonBox::Discard:
		restorePrefs();
		setUiFromPrefs();
		syncSettings();
		close();
		break;
	case QDialogButtonBox::Apply:
		syncSettings();
		break;
	case QDialogButtonBox::FirstButton:
		syncSettings();
		close();
		break;
	default:
		break; // ignore warnings.
	}
}


#undef SB
