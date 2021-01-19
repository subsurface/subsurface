// SPDX-License-Identifier: GPL-2.0
#include "qPrefTechnicalDetails.h"
#include "qPrefPrivate.h"
#include "core/deco.h"

static const QString group = QStringLiteral("TecDetails");

qPrefTechnicalDetails *qPrefTechnicalDetails::instance()
{
	static qPrefTechnicalDetails *self = new qPrefTechnicalDetails;
	return self;
}


void qPrefTechnicalDetails::loadSync(bool doSync)
{
	disk_calcalltissues(doSync);
	disk_calcceiling(doSync);
	disk_calcceiling3m(doSync);
	disk_calcndltts(doSync);
	disk_dcceiling(doSync);
	disk_display_deco_mode(doSync);
	disk_ead(doSync);
	disk_gfhigh(doSync);
	disk_gflow(doSync);
	disk_gf_low_at_maxdepth(doSync);
	disk_hrgraph(doSync);
	disk_mod(doSync);
	disk_modpO2(doSync);
	disk_percentagegraph(doSync);
	disk_redceiling(doSync);
	disk_rulergraph(doSync);
	disk_show_ccr_sensors(doSync);
	disk_show_ccr_setpoint(doSync);
	disk_show_icd(doSync);
	disk_show_pictures_in_profile(doSync);
	disk_show_sac(doSync);
	disk_show_scr_ocpo2(doSync);
	disk_tankbar(doSync);
	disk_vpmb_conservatism(doSync);
	disk_zoomed_plot(doSync);
}

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "calcalltissues", calcalltissues);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "calcceiling", calcceiling);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "calcceiling3m", calcceiling3m);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "calcndltts", calcndltts);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "decoinfo", decoinfo);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "dcceiling", dcceiling);

HANDLE_PREFERENCE_ENUM(TechnicalDetails, deco_mode, "display_deco_mode", display_deco_mode);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "ead", ead);

void qPrefTechnicalDetails::set_gfhigh(int value)
{
	if (value != prefs.gfhigh) {
		prefs.gfhigh = value;
		disk_gfhigh(true);
		set_gf(-1, prefs.gfhigh);
		emit instance()->gfhighChanged(value);
	}
}

void qPrefTechnicalDetails::disk_gfhigh(bool doSync)
{
	if (doSync) {
		if (prefs.gfhigh)
			qPrefPrivate::propSetValue(keyFromGroupAndName(group, "gfhigh"), prefs.gfhigh, default_prefs.gfhigh);
	} else {
		prefs.gfhigh = qPrefPrivate::propValue(keyFromGroupAndName(group, "gfhigh"), default_prefs.gfhigh).toInt();
		set_gf(-1, prefs.gfhigh);
	}
}

void qPrefTechnicalDetails::set_gflow(int value)
{
	if (value != prefs.gflow) {
		prefs.gflow = value;
		disk_gflow(true);
		set_gf(prefs.gflow, -1);
		emit instance()->gflowChanged(value);
	}
}

void qPrefTechnicalDetails::disk_gflow(bool doSync)
{
	if (doSync) {
		if (prefs.gflow)
			qPrefPrivate::propSetValue(keyFromGroupAndName(group, "gflow"), prefs.gflow, default_prefs.gflow);
	} else {
		prefs.gflow = qPrefPrivate::propValue(keyFromGroupAndName(group, "gflow"), default_prefs.gflow).toInt();
		set_gf(prefs.gflow, -1);
	}
}

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "gf_low_at_maxdepth", gf_low_at_maxdepth);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "hrgraph", hrgraph);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "mod", mod);

HANDLE_PREFERENCE_DOUBLE(TechnicalDetails, "modpO2", modpO2);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "percentagegraph", percentagegraph);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "redceiling", redceiling);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "RulerBar", rulergraph);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "show_ccr_sensors", show_ccr_sensors);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "show_ccr_setpoint", show_ccr_setpoint);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "show_icd", show_icd);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "show_pictures_in_profile", show_pictures_in_profile);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "show_sac", show_sac);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "show_scr_ocpo2", show_scr_ocpo2);

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "tankbar", tankbar);

void qPrefTechnicalDetails::set_vpmb_conservatism(int value)
{
	if (value != prefs.vpmb_conservatism) {
		prefs.vpmb_conservatism = value;
		disk_vpmb_conservatism(true);
		emit instance()->vpmb_conservatismChanged(value);
	}
}

void qPrefTechnicalDetails::disk_vpmb_conservatism(bool doSync)
{
	if (doSync) {
		if (prefs.vpmb_conservatism)
			qPrefPrivate::propSetValue(keyFromGroupAndName(group, "vpmb_conservatism"), prefs.vpmb_conservatism, default_prefs.vpmb_conservatism);
	} else {
		prefs.vpmb_conservatism = qPrefPrivate::propValue(keyFromGroupAndName(group, "vpmb_conservatism"), default_prefs.vpmb_conservatism).toInt();
		set_vpmb_conservatism(prefs.vpmb_conservatism);
	}
}

HANDLE_PREFERENCE_BOOL(TechnicalDetails, "zoomed_plot", zoomed_plot);
