#include "preferences.h"
#include "ui_preferences.h"
#include <QSettings>

PreferencesDialog* PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog();
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f)
, ui(new Ui::PreferencesDialog())
{
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(syncSettings()));

	#define B(X) s.value(#X, false).toBool()
	#define D(X) s.value(#X, 0.0).toDouble()

	QSettings s;

	// Graph
	ui->calculated_ceiling->setChecked(B(show_calculated_ceiling));
	ui->phe->setChecked(B(show_phe));
	ui->po2->setChecked(B(show_po2));
	ui->pn2->setChecked(B(show_pn2));
	ui->pheThreshould->setValue(D(phe_threshould));
	ui->po2Threashould->setValue(D(po2_threshould));
	ui->pn2Threshould->setValue(D(pn2_threshould));
	ui->ead_end_eadd->setChecked(B(show_ead_end_eadd));
	ui->dc_reported_ceiling->setChecked(B(show_dc_reported_ceiling));
	ui->calculated_ceiling->setChecked(B(show_calculated_ceiling));
	ui->increment_3m->setChecked(B(show_3m_increments));
	ui->gflow->setValue(D(gflow));
	ui->gfhigh->setValue(D(gfhigh));

	// Units
	bool value = s.value("units_metric").toBool();
	ui->metric->setChecked(value);
	ui->imperial->setChecked(!value);

	value = s.value("units_celcius").toBool();
	ui->celsius->setChecked( value);
	ui->fahrenheit->setChecked( !value);

	value = s.value("units_meters").toBool();
	ui->meter->setChecked(value);
	ui->feet->setChecked(!value);

	value = s.value("units_bar").toBool();
	ui->bar->setChecked(value);
	ui->psi->setChecked(!value);

	value = s.value("units_liter").toBool();
	ui->liter->setChecked(value);
	ui->cuft->setChecked(!value);

	value = s.value("units_kgs").toBool();
	ui->kgs->setChecked(value);
	ui->lbs->setChecked(!value);

	// Defaults
	ui->font->setFont( QFont(s.value("table_fonts").toString()));
	ui->fontsize->setValue(D(font_size));

	ui->defaultfilename->setText(s.value("default_file").toString());
	ui->displayinvalid->setChecked(B(show_invalid));

#undef B
#undef D
}

void PreferencesDialog::syncSettings()
{
	QSettings s;

	// Graph
	s.setValue("show_calculated_ceiling", ui->calculated_ceiling->isChecked());
	s.setValue("show_phe", ui->phe->isChecked());
	s.setValue("show_po2", ui->po2->isChecked());
	s.setValue("show_pn2", ui->pn2->isChecked());
	s.setValue("phe_threshould", ui->pheThreshould->value());
	s.setValue("po2_threshould", ui->po2Threashould->value());
	s.setValue("pn2_threshould", ui->pn2Threshould->value());
	s.setValue("show_ead_end_eadd", ui->ead_end_eadd->isChecked());
	s.setValue("show_dc_reported_ceiling", ui->dc_reported_ceiling->isChecked());
	s.setValue("show_calculated_ceiling", ui->calculated_ceiling->isChecked());

	s.setValue("show_3m_increments", ui->increment_3m->isChecked());
	s.setValue("gflow", ui->gflow->value());
	s.setValue("gfhigh", ui->gfhigh->value());

	// Units
	s.setValue("units_metric", ui->metric->isChecked());
	s.setValue("units_celcius", ui->celsius->isChecked());
	s.setValue("units_meter", ui->meter->isChecked());
	s.setValue("units_bar", ui->bar->isChecked());
	s.setValue("units_liter", ui->liter->isChecked());
	s.setValue("units_kgs", ui->liter->isChecked());

	// Defaults
	s.value("table_fonts", ui->font->font().family());
	s.value("font_size", ui->fontsize->value());
	s.value("default_file", ui->defaultfilename->text());
	s.value("displayinvalid", ui->displayinvalid->isChecked());
	s.sync();

	emit settingsChanged();
}
