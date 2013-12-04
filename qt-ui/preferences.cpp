#include "preferences.h"
#include "mainwindow.h"
#include <QSettings>
#include <QDebug>
#include <QFileDialog>

PreferencesDialog* PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog(mainWindow());
	dialog->setAttribute(Qt::WA_QuitOnClose, false);
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	connect(ui.gflow, SIGNAL(valueChanged(int)), this, SLOT(gflowChanged(int)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), this, SLOT(gfhighChanged(int)));
	setUiFromPrefs();
	rememberPrefs();
}

void PreferencesDialog::gflowChanged(int gf)
{
	if (gf > 100)
		ui.gflow->setStyleSheet("* { color: red; }");
	else
		ui.gflow->setStyleSheet("");
}

void PreferencesDialog::gfhighChanged(int gf)
{
	if (gf > 100)
		ui.gfhigh->setStyleSheet("* { color: red; }");
	else
		ui.gfhigh->setStyleSheet("");
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
	ui.phe->setChecked(prefs.pp_graphs.phe);
	ui.pheThreshold->setEnabled(ui.phe->isChecked());
	ui.po2->setChecked(prefs.pp_graphs.po2);
	ui.po2Threshold->setEnabled(ui.po2->isChecked());
	ui.pn2->setChecked(prefs.pp_graphs.pn2);
	ui.pn2Threshold->setEnabled(ui.pn2->isChecked());
	ui.pheThreshold->setValue(prefs.pp_graphs.phe_threshold);
	ui.po2Threshold->setValue(prefs.pp_graphs.po2_threshold);
	ui.pn2Threshold->setValue(prefs.pp_graphs.pn2_threshold);
	ui.ead_end_eadd->setChecked(prefs.ead);
	ui.mod->setChecked(prefs.mod);
	ui.maxppo2->setEnabled(ui.mod->isChecked());
	ui.maxppo2->setValue(prefs.mod_ppO2);
	ui.dc_reported_ceiling->setChecked(prefs.profile_dc_ceiling);
	ui.red_ceiling->setEnabled(ui.dc_reported_ceiling->isChecked());
	ui.red_ceiling->setChecked(prefs.profile_red_ceiling);
	ui.calculated_ceiling->setChecked(prefs.profile_calc_ceiling);
	ui.increment_3m->setEnabled(ui.calculated_ceiling->isChecked());
	ui.increment_3m->setChecked(prefs.calc_ceiling_3m_incr);
	ui.all_tissues->setEnabled(ui.calculated_ceiling->isChecked());
	ui.all_tissues->setChecked(prefs.calc_all_tissues);
	ui.calc_ndl_tts->setEnabled(ui.calculated_ceiling->isChecked());
	ui.calc_ndl_tts->setChecked(prefs.calc_ndl_tts);
	ui.groupBox->setEnabled(ui.personalize->isChecked());

	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);
	ui.gf_low_at_maxdepth->setChecked(prefs.gf_low_at_maxdepth);

	// units
	if (prefs.unit_system == METRIC)
		ui.metric->setChecked(true);
	else if (prefs.unit_system == IMPERIAL)
		ui.imperial->setChecked(true);
	else
		ui.personalize->setChecked(true);

	ui.celsius->setChecked(prefs.units.temperature == units::CELSIUS);
	ui.fahrenheit->setChecked(prefs.units.temperature == units::FAHRENHEIT);
	ui.meter->setChecked(prefs.units.length == units::METERS);
	ui.feet->setChecked(prefs.units.length == units::FEET);
	ui.bar->setChecked(prefs.units.pressure == units::BAR);
	ui.psi->setChecked(prefs.units.pressure == units::PSI);
	ui.liter->setChecked(prefs.units.volume == units::LITER);
	ui.cuft->setChecked(prefs.units.volume == units::CUFT);
	ui.kgs->setChecked(prefs.units.weight == units::KG);
	ui.lbs->setChecked(prefs.units.weight == units::LBS);
	ui.font->setFont(QString(prefs.divelist_font));
	ui.fontsize->setValue(prefs.font_size);
	ui.defaultfilename->setText(prefs.default_filename);
	ui.default_cylinder->clear();
	for(int i=0; tank_info[i].name != NULL; i++) {
		ui.default_cylinder->addItem(tank_info[i].name);
		if (prefs.default_cylinder && strcmp(tank_info[i].name, prefs.default_cylinder) == 0)
			ui.default_cylinder->setCurrentIndex(i);
	}
	ui.displayinvalid->setChecked(prefs.display_invalid_dives);
	ui.show_sac->setChecked(prefs.show_sac);
	ui.vertical_speed_minutes->setChecked(prefs.units.vertical_speed_time == units::MINUTES);
	ui.vertical_speed_seconds->setChecked(prefs.units.vertical_speed_time == units::SECONDS);
}

void PreferencesDialog::restorePrefs()
{
	prefs = oldPrefs;
	setUiFromPrefs();
}

void PreferencesDialog::rememberPrefs()
{
	oldPrefs = prefs;
}

#define SB(V, B) s.setValue(V, (int)(B->isChecked() ? 1 : 0))

void PreferencesDialog::syncSettings()
{
	QSettings s;

	// Graph
	s.beginGroup("TecDetails");

	SB("phegraph", ui.phe);
	SB("po2graph", ui.po2);
	SB("pn2graph", ui.pn2);
	s.setValue("phethreshold", ui.pheThreshold->value());
	s.setValue("po2threshold", ui.po2Threshold->value());
	s.setValue("pn2threshold", ui.pn2Threshold->value());
	SB("ead", ui.ead_end_eadd);
	SB("mod", ui.mod);
	s.setValue("modppO2", ui.maxppo2->value());
	SB("dcceiling", ui.dc_reported_ceiling);
	SB("redceiling", ui.red_ceiling);
	SB("calcceiling", ui.calculated_ceiling);
	SB("calcceiling3m", ui.increment_3m);
	SB("calcndltts", ui.calc_ndl_tts);
	SB("calcalltissues", ui.all_tissues);
	s.setValue("gflow", ui.gflow->value());
	s.setValue("gfhigh", ui.gfhigh->value());
	SB("gf_low_at_maxdepth", ui.gf_low_at_maxdepth);
	SB("show_sac", ui.show_sac);
	s.endGroup();

	// Units
	s.beginGroup("Units");
	QString unitSystem = ui.metric->isChecked() ? "metric" : (ui.imperial->isChecked() ? "imperial" : "personal");
	s.setValue("unit_system", unitSystem);
	s.setValue("temperature", ui.fahrenheit->isChecked() ? units::FAHRENHEIT : units::CELSIUS);
	s.setValue("length", ui.feet->isChecked() ? units::FEET : units::METERS);
	s.setValue("pressure", ui.psi->isChecked() ? units::PSI : units::BAR);
	s.setValue("volume", ui.cuft->isChecked() ? units::CUFT : units::LITER);
	s.setValue("weight", ui.lbs->isChecked() ? units::LBS : units::KG);
	s.setValue("vertical_speed_time", ui.vertical_speed_minutes->isChecked() ? units::MINUTES : units::SECONDS);
	s.endGroup();
	// Defaults
	s.beginGroup("GeneralSettings");
	s.setValue("default_filename", ui.defaultfilename->text());
	s.setValue("default_cylinder", ui.default_cylinder->currentText());
	s.endGroup();

	s.beginGroup("Display");
	s.value("divelist_font", ui.font->font().family());
	s.value("font_size", ui.fontsize->value());
	s.value("displayinvalid", ui.displayinvalid->isChecked());
	s.endGroup();
	s.sync();

	emit settingsChanged();
}

void PreferencesDialog::buttonClicked(QAbstractButton* button)
{
	switch(ui.buttonBox->standardButton(button)){
	case QDialogButtonBox::Discard:
		restorePrefs();
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

void PreferencesDialog::on_chooseFile_clicked()
{
	QFileInfo fi(system_default_filename());
	ui.defaultfilename->setText(QFileDialog::getOpenFileName(this, tr("Open Default Log File"), fi.absolutePath(), tr("Subsurface XML files (*.ssrf *.xml *.XML)")));
}
