#include "preferences_graph.h"
#include "ui_preferences_graph.h"
#include "subsurface-core/prefs-macros.h"

#include <QSettings>
#include <QMessageBox>

#include "qt-models/models.h"

PreferencesGraph::PreferencesGraph() : AbstractPreferencesWidget(tr("Graph"), QIcon(":graph"), 5)
{
	ui = new Ui::PreferencesGraph();
	ui->setupUi(this);
}

PreferencesGraph::~PreferencesGraph()
{
	delete ui;
}

void PreferencesGraph::refreshSettings()
{
	ui->pheThreshold->setValue(prefs.pp_graphs.phe_threshold);
	ui->po2Threshold->setValue(prefs.pp_graphs.po2_threshold);
	ui->pn2Threshold->setValue(prefs.pp_graphs.pn2_threshold);
	ui->maxpo2->setValue(prefs.modpO2);
	ui->red_ceiling->setChecked(prefs.redceiling);

	ui->gflow->setValue(prefs.gflow);
	ui->gfhigh->setValue(prefs.gfhigh);
	ui->gf_low_at_maxdepth->setChecked(prefs.gf_low_at_maxdepth);
	ui->show_ccr_setpoint->setChecked(prefs.show_ccr_setpoint);
	ui->show_ccr_sensors->setChecked(prefs.show_ccr_sensors);
	ui->defaultSetpoint->setValue((double)prefs.defaultsetpoint / 1000.0);
	ui->psro2rate->setValue(prefs.o2consumption / 1000.0);
	ui->pscrfactor->setValue(rint(1000.0 / prefs.pscr_ratio));

	ui->display_unused_tanks->setChecked(prefs.display_unused_tanks);
	ui->show_average_depth->setChecked(prefs.show_average_depth);
}

void PreferencesGraph::syncSettings()
{
	QSettings s;

	s.beginGroup("GeneralSettings");
	s.setValue("defaultsetpoint", rint(ui->defaultSetpoint->value() * 1000.0));
	s.setValue("o2consumption", rint(ui->psro2rate->value() *1000.0));
	s.setValue("pscr_ratio", rint(1000.0 / ui->pscrfactor->value()));
	s.endGroup();

	s.beginGroup("TecDetails");
	SAVE_OR_REMOVE("phethreshold", default_prefs.pp_graphs.phe_threshold, ui->pheThreshold->value());
	SAVE_OR_REMOVE("po2threshold", default_prefs.pp_graphs.po2_threshold, ui->po2Threshold->value());
	SAVE_OR_REMOVE("pn2threshold", default_prefs.pp_graphs.pn2_threshold, ui->pn2Threshold->value());
	SAVE_OR_REMOVE("modpO2", default_prefs.modpO2, ui->maxpo2->value());
	SAVE_OR_REMOVE("redceiling", default_prefs.redceiling, ui->red_ceiling->isChecked());
	SAVE_OR_REMOVE("gflow", default_prefs.gflow, ui->gflow->value());
	SAVE_OR_REMOVE("gfhigh", default_prefs.gfhigh, ui->gfhigh->value());
	SAVE_OR_REMOVE("gf_low_at_maxdepth", default_prefs.gf_low_at_maxdepth, ui->gf_low_at_maxdepth->isChecked());
	SAVE_OR_REMOVE("show_ccr_setpoint", default_prefs.show_ccr_setpoint, ui->show_ccr_setpoint->isChecked());
	SAVE_OR_REMOVE("show_ccr_sensors", default_prefs.show_ccr_sensors, ui->show_ccr_sensors->isChecked());
	SAVE_OR_REMOVE("display_unused_tanks", default_prefs.display_unused_tanks, ui->display_unused_tanks->isChecked());
	SAVE_OR_REMOVE("show_average_depth", default_prefs.show_average_depth, ui->show_average_depth->isChecked());
	s.endGroup();
}

#define DANGER_GF (gf > 100) ? "* { color: red; }" : ""
void PreferencesGraph::on_gflow_valueChanged(int gf)
{
	ui->gflow->setStyleSheet(DANGER_GF);
}
void PreferencesGraph::on_gfhigh_valueChanged(int gf)
{
	ui->gfhigh->setStyleSheet(DANGER_GF);
}
#undef DANGER_GF