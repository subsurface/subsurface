// SPDX-License-Identifier: GPL-2.0
#include "preferences_graph.h"
#include "ui_preferences_graph.h"
#include "core/prefs-macros.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include <QMessageBox>

#include "qt-models/models.h"

PreferencesGraph::PreferencesGraph() : AbstractPreferencesWidget(tr("Profile"), QIcon(":graph"), 5)
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
	ui->defaultSetpoint->setValue((double)prefs.defaultsetpoint / 1000.0);
	ui->psro2rate->setValue(prefs.o2consumption / 1000.0);
	ui->pscrfactor->setValue(lrint(1000.0 / prefs.pscr_ratio));

	ui->display_unused_tanks->setChecked(prefs.display_unused_tanks);
	ui->show_average_depth->setChecked(prefs.show_average_depth);
}

void PreferencesGraph::syncSettings()
{
	auto general = SettingsObjectWrapper::instance()->general_settings;
	general->setDefaultSetPoint(lrint(ui->defaultSetpoint->value() * 1000.0));
	general->setO2Consumption(lrint(ui->psro2rate->value() *1000.0));
	general->setPscrRatio(lrint(1000.0 / ui->pscrfactor->value()));

	auto pp_gas = SettingsObjectWrapper::instance()->pp_gas;
	pp_gas->setPheThreshold(ui->pheThreshold->value());
	pp_gas->setPo2ThresholdMax(ui->po2ThresholdMax->value());
	pp_gas->setPo2ThresholdMin(ui->po2ThresholdMin->value());
	pp_gas->setPn2Threshold(ui->pn2Threshold->value());

	auto tech = SettingsObjectWrapper::instance()->techDetails;
	tech->setModp02(ui->maxpo2->value());
	tech->setRedceiling(ui->red_ceiling->isChecked());
	tech->setBuehlmann(ui->buehlmann->isChecked());
	tech->setGflow(ui->gflow->value());
	tech->setGfhigh(ui->gfhigh->value());
	tech->setVpmbConservatism(ui->vpmb_conservatism->value());
	tech->setShowCCRSetpoint(ui->show_ccr_setpoint->isChecked());
	tech->setShowCCRSensors(ui->show_ccr_sensors->isChecked());
	tech->setDisplayUnusedTanks(ui->display_unused_tanks->isChecked());
	tech->setShowAverageDepth(ui->show_average_depth->isChecked());
	tech->setDecoMode(ui->vpmb->isChecked() ? VPMB : BUEHLMANN);
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
