// SPDX-License-Identifier: GPL-2.0
#include "preferences_graph.h"
#include "ui_preferences_graph.h"
#include "core/settings/qPrefGeneral.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefPartialPressureGas.h"
#include <QMessageBox>

#include "qt-models/models.h"
#include "core/deco.h"

PreferencesGraph::PreferencesGraph() : AbstractPreferencesWidget(tr("Tech setup"), QIcon(":graph-icon"), 7)
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
		on_buehlmann_toggled(true);
	} else {
		ui->buehlmann->setChecked(false);
		ui->vpmb->setChecked(true);
		on_buehlmann_toggled(false);
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

	ui->show_icd->setChecked(prefs.show_icd);
}

void PreferencesGraph::syncSettings()
{
	qPrefGeneral::set_defaultsetpoint(lrint(ui->defaultSetpoint->value() * 1000.0));
	qPrefGeneral::set_o2consumption(lrint(ui->psro2rate->value() *1000.0));
	qPrefGeneral::set_pscr_ratio(lrint(1000.0 / ui->pscrfactor->value()));

	qPrefPartialPressureGas::set_phe_threshold(ui->pheThreshold->value());
	qPrefPartialPressureGas::set_po2_threshold_max(ui->po2ThresholdMax->value());
	qPrefPartialPressureGas::set_po2_threshold_min(ui->po2ThresholdMin->value());
	qPrefPartialPressureGas::set_pn2_threshold(ui->pn2Threshold->value());

	qPrefTechnicalDetails::set_modpO2(ui->maxpo2->value());
	qPrefTechnicalDetails::set_redceiling(ui->red_ceiling->isChecked());
	prefs.planner_deco_mode = ui->buehlmann->isChecked() ? BUEHLMANN : VPMB;
	qPrefTechnicalDetails::set_gflow(ui->gflow->value());
	qPrefTechnicalDetails::set_gfhigh(ui->gfhigh->value());
	qPrefTechnicalDetails::set_vpmb_conservatism(ui->vpmb_conservatism->value());
	set_vpmb_conservatism(ui->vpmb_conservatism->value());
	qPrefTechnicalDetails::set_show_ccr_setpoint(ui->show_ccr_setpoint->isChecked());
	qPrefTechnicalDetails::set_show_ccr_sensors(ui->show_ccr_sensors->isChecked());
	qPrefTechnicalDetails::set_show_scr_ocpo2(ui->show_scr_ocpo2->isChecked());
	qPrefTechnicalDetails::set_show_icd(ui->show_icd->isChecked());
	qPrefTechnicalDetails::set_display_deco_mode(ui->vpmb->isChecked() ? VPMB : BUEHLMANN);
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
