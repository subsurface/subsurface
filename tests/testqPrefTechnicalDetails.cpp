// SPDX-License-Identifier: GPL-2.0
#include "testqPrefTechnicalDetails.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"

#include <QTest>

void TestQPrefTechnicalDetails::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefTechnicalDetails");
}

void TestQPrefTechnicalDetails::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefTechnicalDetails::instance();

	prefs.calcalltissues = true;
	prefs.calcceiling = true;
	prefs.calcceiling3m = true;
	prefs.calcndltts = true;
	prefs.dcceiling = true;
	prefs.display_deco_mode = BUEHLMANN;
	prefs.display_unused_tanks = true;
	prefs.ead = true;
	prefs.gfhigh = 27;
	prefs.gflow = 25;
	prefs.gf_low_at_maxdepth = true;
	prefs.hrgraph = true;
	prefs.mod = true;
	prefs.modpO2 = 1.02;
	prefs.percentagegraph = true;
	prefs.redceiling = true;
	prefs.rulergraph = true;
	prefs.show_average_depth = true;
	prefs.show_ccr_sensors = true;
	prefs.show_ccr_setpoint = true;
	prefs.show_icd = true;
	prefs.show_pictures_in_profile = true;
	prefs.show_sac = true;
	prefs.show_scr_ocpo2 = true;
	prefs.tankbar = true;
	prefs.vpmb_conservatism = 123;
	prefs.zoomed_plot = true;

	QCOMPARE(tst->calcceiling(), prefs.calcceiling);
	QCOMPARE(tst->calcceiling3m(), prefs.calcceiling3m);
	QCOMPARE(tst->calcndltts(), prefs.calcndltts);
	QCOMPARE(tst->dcceiling(), prefs.dcceiling);
	QCOMPARE(tst->display_deco_mode(), prefs.display_deco_mode);
	QCOMPARE(tst->display_unused_tanks(), prefs.display_unused_tanks);
	QCOMPARE(tst->ead(), prefs.ead);
	QCOMPARE(tst->gfhigh(), (int)prefs.gfhigh);
	QCOMPARE(tst->gflow(), (int)prefs.gflow);
	QCOMPARE(tst->gf_low_at_maxdepth(), prefs.gf_low_at_maxdepth);
	QCOMPARE(tst->hrgraph(), prefs.hrgraph);
	QCOMPARE(tst->mod(), prefs.mod);
	QCOMPARE(tst->modpO2(), prefs.modpO2);
	QCOMPARE(tst->percentagegraph(), prefs.percentagegraph);
	QCOMPARE(tst->redceiling(), prefs.redceiling);
	QCOMPARE(tst->rulergraph(), prefs.rulergraph);
	QCOMPARE(tst->show_average_depth(), prefs.show_average_depth);
	QCOMPARE(tst->show_ccr_sensors(), prefs.show_ccr_sensors);
	QCOMPARE(tst->show_ccr_setpoint(), prefs.show_ccr_setpoint);
	QCOMPARE(tst->show_icd(), prefs.show_icd);
	QCOMPARE(tst->show_pictures_in_profile(), prefs.show_pictures_in_profile);
	QCOMPARE(tst->show_sac(), prefs.show_sac);
	QCOMPARE(tst->show_scr_ocpo2(), prefs.show_scr_ocpo2);
	QCOMPARE(tst->tankbar(), prefs.tankbar);
	QCOMPARE(tst->vpmb_conservatism(), (int)prefs.vpmb_conservatism);
	QCOMPARE(tst->zoomed_plot(), prefs.zoomed_plot);
}

void TestQPrefTechnicalDetails::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefTechnicalDetails::instance();

	tst->set_calcalltissues(false);
	tst->set_calcceiling(false);
	tst->set_calcceiling3m(false);
	tst->set_calcndltts(false);
	tst->set_dcceiling(false);
	tst->set_display_deco_mode(RECREATIONAL);
	tst->set_display_unused_tanks(false);
	tst->set_ead(false);
	tst->set_gfhigh(29);
	tst->set_gflow(24);
	tst->set_gf_low_at_maxdepth(false);
	tst->set_hrgraph(false);
	tst->set_mod(false);
	tst->set_modpO2(1.12);
	tst->set_percentagegraph(false);
	tst->set_redceiling(false);
	tst->set_rulergraph(false);
	tst->set_show_average_depth(false);
	tst->set_show_ccr_sensors(false);
	tst->set_show_ccr_setpoint(false);
	tst->set_show_icd(false);
	tst->set_show_pictures_in_profile(false);
	tst->set_show_sac(false);
	tst->set_show_scr_ocpo2(false);
	tst->set_tankbar(false);
	tst->set_vpmb_conservatism(64);
	tst->set_zoomed_plot(false);

	QCOMPARE(prefs.calcceiling, false);
	QCOMPARE(prefs.calcceiling3m, false);
	QCOMPARE(prefs.calcndltts, false);
	QCOMPARE(prefs.dcceiling, false);
	QCOMPARE(prefs.display_deco_mode, RECREATIONAL);
	QCOMPARE(prefs.display_unused_tanks, false);
	QCOMPARE(prefs.ead, false);
	QCOMPARE((int)prefs.gfhigh, 29);
	QCOMPARE((int)prefs.gflow, 24);
	QCOMPARE(prefs.gf_low_at_maxdepth, false);
	QCOMPARE(prefs.hrgraph, false);
	QCOMPARE(prefs.mod, false);
	QCOMPARE(prefs.modpO2, 1.12);
	QCOMPARE(prefs.percentagegraph, false);
	QCOMPARE(prefs.redceiling, false);
	QCOMPARE(prefs.rulergraph, false);
	QCOMPARE(prefs.show_average_depth, false);
	QCOMPARE(prefs.show_ccr_sensors, false);
	QCOMPARE(prefs.show_ccr_setpoint, false);
	QCOMPARE(prefs.show_icd, false);
	QCOMPARE(prefs.show_pictures_in_profile, false);
	QCOMPARE(prefs.show_sac, false);
	QCOMPARE(prefs.show_scr_ocpo2, false);
	QCOMPARE(prefs.tankbar, false);
	QCOMPARE((int)prefs.vpmb_conservatism, 64);
	QCOMPARE(prefs.zoomed_plot, false);
}

void TestQPrefTechnicalDetails::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefTechnicalDetails::instance();

	tst->set_calcalltissues(false);
	tst->set_calcceiling(false);
	tst->set_calcceiling3m(false);
	tst->set_calcndltts(false);
	tst->set_dcceiling(false);
	tst->set_display_deco_mode(RECREATIONAL);
	tst->set_display_unused_tanks(false);
	tst->set_ead(false);
	tst->set_gfhigh(29);
	tst->set_gflow(24);
	tst->set_gf_low_at_maxdepth(false);
	tst->set_hrgraph(false);
	tst->set_mod(false);
	tst->set_modpO2(1.12);
	tst->set_percentagegraph(false);
	tst->set_redceiling(false);
	tst->set_rulergraph(false);
	tst->set_show_average_depth(false);
	tst->set_show_ccr_sensors(false);
	tst->set_show_ccr_setpoint(false);
	tst->set_show_icd(false);
	tst->set_show_pictures_in_profile(false);
	tst->set_show_sac(false);
	tst->set_show_scr_ocpo2(false);
	tst->set_tankbar(false);
	tst->set_vpmb_conservatism(64);
	tst->set_zoomed_plot(false);

	prefs.calcalltissues = true;
	prefs.calcceiling = true;
	prefs.calcceiling3m = true;
	prefs.calcndltts = true;
	prefs.dcceiling = true;
	prefs.display_deco_mode = BUEHLMANN;
	prefs.display_unused_tanks = true;
	prefs.ead = true;
	prefs.gfhigh = 27;
	prefs.gflow = 25;
	prefs.gf_low_at_maxdepth = true;
	prefs.hrgraph = true;
	prefs.mod = true;
	prefs.modpO2 = 1.02;
	prefs.percentagegraph = true;
	prefs.redceiling = true;
	prefs.rulergraph = true;
	prefs.show_average_depth = true;
	prefs.show_ccr_sensors = true;
	prefs.show_ccr_setpoint = true;
	prefs.show_icd = true;
	prefs.show_pictures_in_profile = true;
	prefs.show_sac = true;
	prefs.show_scr_ocpo2 = true;
	prefs.tankbar = true;
	prefs.vpmb_conservatism = 123;
	prefs.zoomed_plot = true;

	tst->load();
	QCOMPARE(prefs.calcceiling, false);
	QCOMPARE(prefs.calcceiling3m, false);
	QCOMPARE(prefs.calcndltts, false);
	QCOMPARE(prefs.dcceiling, false);
	QCOMPARE(prefs.display_deco_mode, RECREATIONAL);
	QCOMPARE(prefs.display_unused_tanks, false);
	QCOMPARE(prefs.ead, false);
	QCOMPARE((int)prefs.gfhigh, 29);
	QCOMPARE((int)prefs.gflow, 24);
	QCOMPARE(prefs.gf_low_at_maxdepth, false);
	QCOMPARE(prefs.hrgraph, false);
	QCOMPARE(prefs.mod, false);
	QCOMPARE(prefs.modpO2, 1.12);
	QCOMPARE(prefs.percentagegraph, false);
	QCOMPARE(prefs.redceiling, false);
	QCOMPARE(prefs.rulergraph, false);
	QCOMPARE(prefs.show_average_depth, false);
	QCOMPARE(prefs.show_ccr_sensors, false);
	QCOMPARE(prefs.show_ccr_setpoint, false);
	QCOMPARE(prefs.show_icd, false);
	QCOMPARE(prefs.show_pictures_in_profile, false);
	QCOMPARE(prefs.show_sac, false);
	QCOMPARE(prefs.show_scr_ocpo2, false);
	QCOMPARE(prefs.tankbar, false);
	QCOMPARE((int)prefs.vpmb_conservatism, 64);
	QCOMPARE(prefs.zoomed_plot, false);
}

void TestQPrefTechnicalDetails::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefTechnicalDetails::instance();

	prefs.calcalltissues = true;
	prefs.calcceiling = true;
	prefs.calcceiling3m = true;
	prefs.calcndltts = true;
	prefs.dcceiling = true;
	prefs.display_deco_mode = BUEHLMANN;
	prefs.display_unused_tanks = true;
	prefs.ead = true;
	prefs.gfhigh = 11;
	prefs.gflow = 12;
	prefs.gf_low_at_maxdepth = true;
	prefs.hrgraph = true;
	prefs.mod = true;
	prefs.modpO2 = 1.14;
	prefs.percentagegraph = true;
	prefs.redceiling = true;
	prefs.rulergraph = true;
	prefs.show_average_depth = true;
	prefs.show_ccr_sensors = true;
	prefs.show_ccr_setpoint = true;
	prefs.show_icd = true;
	prefs.show_pictures_in_profile = true;
	prefs.show_sac = true;
	prefs.show_scr_ocpo2 = true;
	prefs.tankbar = true;
	prefs.vpmb_conservatism = 15;
	prefs.zoomed_plot = true;

	tst->sync();
	prefs.calcalltissues = false;
	prefs.calcceiling = false;
	prefs.calcceiling3m = false;
	prefs.calcndltts = false;
	prefs.dcceiling = false;
	prefs.display_deco_mode = RECREATIONAL;
	prefs.display_unused_tanks = false;
	prefs.ead = false;
	prefs.gfhigh = 27;
	prefs.gflow = 25;
	prefs.gf_low_at_maxdepth = false;
	prefs.hrgraph = false;
	prefs.mod = false;
	prefs.modpO2 = 1.02;
	prefs.percentagegraph = false;
	prefs.redceiling = false;
	prefs.rulergraph = false;
	prefs.show_average_depth = false;
	prefs.show_ccr_sensors = false;
	prefs.show_ccr_setpoint = false;
	prefs.show_icd = false;
	prefs.show_pictures_in_profile = false;
	prefs.show_sac = false;
	prefs.show_scr_ocpo2 = false;
	prefs.tankbar = false;
	prefs.vpmb_conservatism = 123;
	prefs.zoomed_plot = false;

	tst->load();
	QCOMPARE(prefs.calcceiling, true);
	QCOMPARE(prefs.calcceiling3m, true);
	QCOMPARE(prefs.calcndltts, true);
	QCOMPARE(prefs.dcceiling, true);
	QCOMPARE(prefs.display_deco_mode, BUEHLMANN);
	QCOMPARE(prefs.display_unused_tanks, true);
	QCOMPARE(prefs.ead, true);
	QCOMPARE((int)prefs.gfhigh, 11);
	QCOMPARE((int)prefs.gflow, 12);
	QCOMPARE(prefs.gf_low_at_maxdepth, true);
	QCOMPARE(prefs.hrgraph, true);
	QCOMPARE(prefs.mod, true);
	QCOMPARE(prefs.modpO2, 1.14);
	QCOMPARE(prefs.percentagegraph, true);
	QCOMPARE(prefs.redceiling, true);
	QCOMPARE(prefs.rulergraph, true);
	QCOMPARE(prefs.show_average_depth, true);
	QCOMPARE(prefs.show_ccr_sensors, true);
	QCOMPARE(prefs.show_ccr_setpoint, true);
	QCOMPARE(prefs.show_icd, true);
	QCOMPARE(prefs.show_pictures_in_profile, true);
	QCOMPARE(prefs.show_sac, true);
	QCOMPARE(prefs.show_scr_ocpo2, true);
	QCOMPARE(prefs.tankbar, true);
	QCOMPARE((int)prefs.vpmb_conservatism, 15);
	QCOMPARE(prefs.zoomed_plot, true);
}

void TestQPrefTechnicalDetails::test_multiple()
{
	// test multiple instances have the same information

	prefs.gfhigh = 27;
	auto tst_direct = new qPrefTechnicalDetails;

	prefs.gflow = 25;
	auto tst = qPrefTechnicalDetails::instance();

	QCOMPARE(tst->gfhigh(), tst_direct->gfhigh());
	QCOMPARE(tst->gflow(), tst_direct->gflow());
	QCOMPARE(tst_direct->gfhigh(), 27);
	QCOMPARE(tst_direct->gflow(), 25);
}

QTEST_MAIN(TestQPrefTechnicalDetails)
