// SPDX-License-Identifier: GPL-2.0
#include "preferences_graph.h"
#include "ui_preferences_graph.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include <QMessageBox>

#include "qt-models/models.h"

PreferencesGraph::PreferencesGraph() : AbstractPreferencesWidget(tr("Profile"), QIcon(":graph-icon"), 5)
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
	ui->po2ThresholdMax->setValue(prefs.pp_graphs.po2_threshold_max);
	ui->po2ThresholdMin->setValue(prefs.pp_graphs.po2_threshold_min);
	ui->pn2Threshold->setValue(prefs.pp_graphs.pn2_threshold);
	ui->maxpo2->setValue(prefs.modpO2);
	ui->red_ceiling->setChecked(prefs.redceiling);

	if (prefs.display_deco_mode == BUEHLMANN) {
		ui->buehlmann->setChecked(true);
		ui->vpmb->setChecked(false);
	} else {
		ui->buehlmann->setChecked(false);
		ui->vpmb->setChecked(true);
	}

	ui->gflow->setValue(prefs.gflow);
	ui->gfhigh->setValue(prefs.gfhigh);
	ui->vpmb_conservatism->setValue(prefs.vpmb_conservatism);
	ui->show_ccr_setpoint->setChecked(prefs.show_ccr_setpoint);
	ui->show_ccr_sensors->setChecked(prefs.show_ccr_sensors);
	ui->show_scr_ocpo2->setChecked(prefs.show_scr_ocpo2);
	ui->defaultSetpoint->setValue((double)prefs.defaultsetpoint / 1000.0);
	ui->psro2rate->setValue(prefs.o2consumption / 1000.0);
	ui->pscrfactor->setValue(lrint(1000.0 / prefs.pscr_ratio));

	ui->display_unused_tanks->setChecked(prefs.display_unused_tanks);
	ui->show_average_depth->setChecked(prefs.show_average_depth);
	ui->auto_recalculate_thumbnails->setChecked(prefs.auto_recalculate_thumbnails);
	ui->show_icd->setChecked(prefs.show_icd);
}

void PreferencesGraph::syncSettings()
{
	auto general = qPrefGeneral::instance();
	general->set_defaultsetpoint(lrint(ui->defaultSetpoint->value() * 1000.0));
	general->set_o2consumption(lrint(ui->psro2rate->value() *1000.0));
	general->set_pscr_ratio(lrint(1000.0 / ui->pscrfactor->value()));
	general->set_auto_recalculate_thumbnails(ui->auto_recalculate_thumbnails->isChecked());

	auto pp_gas = qPrefPartialPressureGas::instance();
	pp_gas->set_phe_threshold(ui->pheThreshold->value());
	pp_gas->set_po2_threshold_max(ui->po2ThresholdMax->value());
	pp_gas->set_po2_threshold_min(ui->po2ThresholdMin->value());
	pp_gas->set_pn2_threshold(ui->pn2Threshold->value());

	auto tech = qPrefTechnicalDetails::instance();
	tech->set_modpO2(ui->maxpo2->value());
	tech->set_redceiling(ui->red_ceiling->isChecked());
	prefs.planner_deco_mode = ui->buehlmann->isChecked() ? BUEHLMANN : VPMB;
	tech->set_gflow(ui->gflow->value());
	tech->set_gfhigh(ui->gfhigh->value());
	tech->set_vpmb_conservatism(ui->vpmb_conservatism->value());
	tech->set_show_ccr_setpoint(ui->show_ccr_setpoint->isChecked());
	tech->set_show_ccr_sensors(ui->show_ccr_sensors->isChecked());
	tech->set_show_scr_ocpo2(ui->show_scr_ocpo2->isChecked());
	tech->set_display_unused_tanks(ui->display_unused_tanks->isChecked());
	tech->set_show_average_depth(ui->show_average_depth->isChecked());
	tech->set_show_icd(ui->show_icd->isChecked());
	tech->set_display_deco_mode(ui->vpmb->isChecked() ? VPMB : BUEHLMANN);
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

void PreferencesGraph::on_buehlmann_toggled(bool buehlmann)
{
	ui->gfhigh->setEnabled(buehlmann);
	ui->gflow->setEnabled(buehlmann);
	ui->label_GFhigh->setEnabled(buehlmann);
	ui->label_GFlow->setEnabled(buehlmann);
	ui->vpmb_conservatism->setEnabled(!buehlmann);
	ui->label_VPMB->setEnabled(!buehlmann);
}

#undef DANGER_GF
