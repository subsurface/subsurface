// SPDX-License-Identifier: GPL-2.0
#include "testqPrefTechnicalDetails.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPref.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefTechnicalDetails::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefTechnicalDetails");
	qPref::registerQML(NULL);
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
	prefs.show_ccr_sensors = true;
	prefs.show_ccr_setpoint = true;
	prefs.show_icd = true;
	prefs.show_pictures_in_profile = true;
	prefs.show_sac = true;
	prefs.show_scr_ocpo2 = true;
	prefs.tankbar = true;
	prefs.vpmb_conservatism = 123;
	prefs.zoomed_plot = true;
	prefs.infobox = true;

	QCOMPARE(tst->calcceiling(), prefs.calcceiling);
	QCOMPARE(tst->calcceiling3m(), prefs.calcceiling3m);
	QCOMPARE(tst->calcndltts(), prefs.calcndltts);
	QCOMPARE(tst->dcceiling(), prefs.dcceiling);
	QCOMPARE(tst->display_deco_mode(), prefs.display_deco_mode);
	QCOMPARE(tst->ead(), prefs.ead);
	QCOMPARE(tst->gfhigh(), prefs.gfhigh);
	QCOMPARE(tst->gflow(), prefs.gflow);
	QCOMPARE(tst->gf_low_at_maxdepth(), prefs.gf_low_at_maxdepth);
	QCOMPARE(tst->hrgraph(), prefs.hrgraph);
	QCOMPARE(tst->mod(), prefs.mod);
	QCOMPARE(tst->modpO2(), prefs.modpO2);
	QCOMPARE(tst->percentagegraph(), prefs.percentagegraph);
	QCOMPARE(tst->redceiling(), prefs.redceiling);
	QCOMPARE(tst->rulergraph(), prefs.rulergraph);
	QCOMPARE(tst->show_ccr_sensors(), prefs.show_ccr_sensors);
	QCOMPARE(tst->show_ccr_setpoint(), prefs.show_ccr_setpoint);
	QCOMPARE(tst->show_icd(), prefs.show_icd);
	QCOMPARE(tst->show_pictures_in_profile(), prefs.show_pictures_in_profile);
	QCOMPARE(tst->show_sac(), prefs.show_sac);
	QCOMPARE(tst->show_scr_ocpo2(), prefs.show_scr_ocpo2);
	QCOMPARE(tst->tankbar(), prefs.tankbar);
	QCOMPARE(tst->vpmb_conservatism(), prefs.vpmb_conservatism);
	QCOMPARE(tst->zoomed_plot(), prefs.zoomed_plot);
	QCOMPARE(tst->infobox(), prefs.infobox);
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
	tst->set_show_ccr_sensors(false);
	tst->set_show_ccr_setpoint(false);
	tst->set_show_icd(false);
	tst->set_show_pictures_in_profile(false);
	tst->set_show_sac(false);
	tst->set_show_scr_ocpo2(false);
	tst->set_tankbar(false);
	tst->set_vpmb_conservatism(64);
	tst->set_zoomed_plot(false);
	tst->set_infobox(false);

	QCOMPARE(prefs.calcceiling, false);
	QCOMPARE(prefs.calcceiling3m, false);
	QCOMPARE(prefs.calcndltts, false);
	QCOMPARE(prefs.dcceiling, false);
	QCOMPARE(prefs.display_deco_mode, RECREATIONAL);
	QCOMPARE(prefs.ead, false);
	QCOMPARE(prefs.gfhigh, 29);
	QCOMPARE(prefs.gflow, 24);
	QCOMPARE(prefs.gf_low_at_maxdepth, false);
	QCOMPARE(prefs.hrgraph, false);
	QCOMPARE(prefs.mod, false);
	QCOMPARE(prefs.modpO2, 1.12);
	QCOMPARE(prefs.percentagegraph, false);
	QCOMPARE(prefs.redceiling, false);
	QCOMPARE(prefs.rulergraph, false);
	QCOMPARE(prefs.show_ccr_sensors, false);
	QCOMPARE(prefs.show_ccr_setpoint, false);
	QCOMPARE(prefs.show_icd, false);
	QCOMPARE(prefs.show_pictures_in_profile, false);
	QCOMPARE(prefs.show_sac, false);
	QCOMPARE(prefs.show_scr_ocpo2, false);
	QCOMPARE(prefs.tankbar, false);
	QCOMPARE(prefs.vpmb_conservatism, 64);
	QCOMPARE(prefs.zoomed_plot, false);
	QCOMPARE(prefs.infobox, false);
}

void TestQPrefTechnicalDetails::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefTechnicalDetails::instance();

	tst->set_calcalltissues(false);
	tst->set_calcceiling(false);
	tst->set_calcceiling3m(false);
	tst->set_calcndltts(false);
	tst->set_dcceiling(true);
	tst->set_display_deco_mode(RECREATIONAL);
	tst->set_ead(false);
	tst->set_gfhigh(29);
	tst->set_gflow(24);
	tst->set_gf_low_at_maxdepth(true);
	tst->set_hrgraph(false);
	tst->set_mod(false);
	tst->set_modpO2(1.12);
	tst->set_percentagegraph(false);
	tst->set_redceiling(false);
	tst->set_rulergraph(false);
	tst->set_show_ccr_sensors(true);
	tst->set_show_ccr_setpoint(true);
	tst->set_show_icd(true);
	tst->set_show_pictures_in_profile(true);
	tst->set_show_sac(true);
	tst->set_show_scr_ocpo2(true);
	tst->set_tankbar(true);
	tst->set_vpmb_conservatism(64);
	tst->set_zoomed_plot(true);
	tst->set_infobox(true);

	prefs.calcalltissues = true;
	prefs.calcceiling = true;
	prefs.calcceiling3m = true;
	prefs.calcndltts = true;
	prefs.dcceiling = false;
	prefs.display_deco_mode = BUEHLMANN;
	prefs.ead = true;
	prefs.gfhigh = 27;
	prefs.gflow = 25;
	prefs.gf_low_at_maxdepth = false;
	prefs.hrgraph = true;
	prefs.mod = true;
	prefs.modpO2 = 1.02;
	prefs.percentagegraph = true;
	prefs.redceiling = true;
	prefs.rulergraph = true;
	prefs.show_ccr_sensors = false;
	prefs.show_ccr_setpoint = false;
	prefs.show_icd = false;
	prefs.show_pictures_in_profile = false;
	prefs.show_sac = false;
	prefs.show_scr_ocpo2 = false;
	prefs.tankbar = false;
	prefs.vpmb_conservatism = 123;
	prefs.zoomed_plot = false;
	prefs.infobox = false;

	tst->load();
	QCOMPARE(prefs.calcceiling, false);
	QCOMPARE(prefs.calcceiling3m, false);
	QCOMPARE(prefs.calcndltts, false);
	QCOMPARE(prefs.dcceiling, true);
	QCOMPARE(prefs.display_deco_mode, RECREATIONAL);
	QCOMPARE(prefs.ead, false);
	QCOMPARE((int)prefs.gfhigh, 29);
	QCOMPARE((int)prefs.gflow, 24);
	QCOMPARE(prefs.gf_low_at_maxdepth, true);
	QCOMPARE(prefs.hrgraph, false);
	QCOMPARE(prefs.mod, false);
	QCOMPARE(prefs.modpO2, 1.12);
	QCOMPARE(prefs.percentagegraph, false);
	QCOMPARE(prefs.redceiling, false);
	QCOMPARE(prefs.rulergraph, false);
	QCOMPARE(prefs.show_ccr_sensors, true);
	QCOMPARE(prefs.show_ccr_setpoint, true);
	QCOMPARE(prefs.show_icd, true);
	QCOMPARE(prefs.show_pictures_in_profile, true);
	QCOMPARE(prefs.show_sac, true);
	QCOMPARE(prefs.show_scr_ocpo2, true);
	QCOMPARE(prefs.tankbar, true);
	QCOMPARE(prefs.vpmb_conservatism, 64);
	QCOMPARE(prefs.zoomed_plot, true);
	QCOMPARE(prefs.infobox, true);
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
	prefs.show_ccr_sensors = true;
	prefs.show_ccr_setpoint = true;
	prefs.show_icd = true;
	prefs.show_pictures_in_profile = true;
	prefs.show_sac = true;
	prefs.show_scr_ocpo2 = true;
	prefs.tankbar = true;
	prefs.vpmb_conservatism = 15;
	prefs.zoomed_plot = true;
	prefs.infobox = true;

	tst->sync();
	prefs.calcalltissues = false;
	prefs.calcceiling = false;
	prefs.calcceiling3m = false;
	prefs.calcndltts = false;
	prefs.dcceiling = false;
	prefs.display_deco_mode = RECREATIONAL;
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
	QCOMPARE(prefs.ead, true);
	QCOMPARE(prefs.gfhigh, 11);
	QCOMPARE(prefs.gflow, 12);
	QCOMPARE(prefs.gf_low_at_maxdepth, true);
	QCOMPARE(prefs.hrgraph, true);
	QCOMPARE(prefs.mod, true);
	QCOMPARE(prefs.modpO2, 1.14);
	QCOMPARE(prefs.percentagegraph, true);
	QCOMPARE(prefs.redceiling, true);
	QCOMPARE(prefs.rulergraph, true);
	QCOMPARE(prefs.show_ccr_sensors, true);
	QCOMPARE(prefs.show_ccr_setpoint, true);
	QCOMPARE(prefs.show_icd, true);
	QCOMPARE(prefs.show_pictures_in_profile, true);
	QCOMPARE(prefs.show_sac, true);
	QCOMPARE(prefs.show_scr_ocpo2, true);
	QCOMPARE(prefs.tankbar, true);
	QCOMPARE(prefs.vpmb_conservatism, 15);
	QCOMPARE(prefs.zoomed_plot, true);
	QCOMPARE(prefs.infobox, true);
}

void TestQPrefTechnicalDetails::test_multiple()
{
	// test multiple instances have the same information

	prefs.gfhigh = 27;
	prefs.gflow = 25;
	auto tst = qPrefTechnicalDetails::instance();

	QCOMPARE(tst->gfhigh(), qPrefTechnicalDetails::gfhigh());
	QCOMPARE(tst->gflow(), qPrefTechnicalDetails::gflow());
	QCOMPARE(qPrefTechnicalDetails::gfhigh(), 27);
	QCOMPARE(qPrefTechnicalDetails::gflow(), 25);
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	tecDetails->sync();           \
	tecDetails->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefTechnicalDetails::test_oldPreferences()
{
	auto tecDetails = qPrefTechnicalDetails::instance();
	tecDetails->set_modpO2(0.2);
	TEST(tecDetails->modpO2(), 0.2);
	tecDetails->set_modpO2(1.0);
	TEST(tecDetails->modpO2(), 1.0);

	tecDetails->set_gflow(2);
	TEST(tecDetails->gflow(), 2);
	tecDetails->set_gflow(3);
	TEST(tecDetails->gflow(), 3);

	tecDetails->set_gfhigh(4);
	TEST(tecDetails->gfhigh(), 4);
	tecDetails->set_gfhigh(5);
	TEST(tecDetails->gfhigh(), 5);

	tecDetails->set_vpmb_conservatism(5);
	TEST(tecDetails->vpmb_conservatism(), 5);
	tecDetails->set_vpmb_conservatism(6);
	TEST(tecDetails->vpmb_conservatism(), 6);

	tecDetails->set_ead(true);
	TEST(tecDetails->ead(), true);
	tecDetails->set_mod(true);
	TEST(tecDetails->mod(), true);
	tecDetails->set_dcceiling(true);
	TEST(tecDetails->dcceiling(), true);
	tecDetails->set_redceiling(true);
	TEST(tecDetails->redceiling(), true);
	tecDetails->set_calcceiling(true);
	TEST(tecDetails->calcceiling(), true);
	tecDetails->set_calcceiling3m(true);
	TEST(tecDetails->calcceiling3m(), true);
	tecDetails->set_calcalltissues(true);
	TEST(tecDetails->calcalltissues(), true);
	tecDetails->set_calcndltts(true);
	TEST(tecDetails->calcndltts(), true);
	tecDetails->set_hrgraph(true);
	TEST(tecDetails->hrgraph(), true);
	tecDetails->set_tankbar(true);
	TEST(tecDetails->tankbar(), true);
	tecDetails->set_percentagegraph(true);
	TEST(tecDetails->percentagegraph(), true);
	tecDetails->set_rulergraph(true);
	TEST(tecDetails->rulergraph(), true);
	tecDetails->set_show_ccr_setpoint(true);
	TEST(tecDetails->show_ccr_setpoint(), true);
	tecDetails->set_show_ccr_sensors(true);
	TEST(tecDetails->show_ccr_sensors(), true);
	tecDetails->set_zoomed_plot(true);
	TEST(tecDetails->zoomed_plot(), true);
	tecDetails->set_show_sac(true);
	TEST(tecDetails->show_sac(), true);
	tecDetails->set_show_pictures_in_profile(true);
	TEST(tecDetails->show_pictures_in_profile(), true);
	tecDetails->set_infobox(true);
	TEST(tecDetails->infobox(), true);

	tecDetails->set_ead(false);
	TEST(tecDetails->ead(), false);
	tecDetails->set_mod(false);
	TEST(tecDetails->mod(), false);
	tecDetails->set_dcceiling(false);
	TEST(tecDetails->dcceiling(), false);
	tecDetails->set_redceiling(false);
	TEST(tecDetails->redceiling(), false);
	tecDetails->set_calcceiling(false);
	TEST(tecDetails->calcceiling(), false);
	tecDetails->set_calcceiling3m(false);
	TEST(tecDetails->calcceiling3m(), false);
	tecDetails->set_calcalltissues(false);
	TEST(tecDetails->calcalltissues(), false);
	tecDetails->set_calcndltts(false);
	TEST(tecDetails->calcndltts(), false);
	tecDetails->set_hrgraph(false);
	TEST(tecDetails->hrgraph(), false);
	tecDetails->set_tankbar(false);
	TEST(tecDetails->tankbar(), false);
	tecDetails->set_percentagegraph(false);
	TEST(tecDetails->percentagegraph(), false);
	tecDetails->set_rulergraph(false);
	TEST(tecDetails->rulergraph(), false);
	tecDetails->set_show_ccr_setpoint(false);
	TEST(tecDetails->show_ccr_setpoint(), false);
	tecDetails->set_show_ccr_sensors(false);
	TEST(tecDetails->show_ccr_sensors(), false);
	tecDetails->set_zoomed_plot(false);
	TEST(tecDetails->zoomed_plot(), false);
	tecDetails->set_show_sac(false);
	TEST(tecDetails->show_sac(), false);
	tecDetails->set_show_pictures_in_profile(false);
	TEST(tecDetails->show_pictures_in_profile(), false);
	tecDetails->set_infobox(false);
	TEST(tecDetails->infobox(), false);
}

void TestQPrefTechnicalDetails::test_signals()
{
	QSignalSpy spy1(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::calcalltissuesChanged);
	QSignalSpy spy2(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::calcceilingChanged);
	QSignalSpy spy3(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::calcceiling3mChanged);
	QSignalSpy spy4(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::calcndlttsChanged);
	QSignalSpy spy5(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::dcceilingChanged);
	QSignalSpy spy6(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::display_deco_modeChanged);
	QSignalSpy spy8(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::eadChanged);
	QSignalSpy spy9(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::gfhighChanged);
	QSignalSpy spy10(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::gflowChanged);
	QSignalSpy spy11(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::gf_low_at_maxdepthChanged);
	QSignalSpy spy12(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::hrgraphChanged);
	QSignalSpy spy13(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::modChanged);
	QSignalSpy spy14(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::modpO2Changed);
	QSignalSpy spy15(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::percentagegraphChanged);
	QSignalSpy spy16(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::redceilingChanged);
	QSignalSpy spy17(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::rulergraphChanged);
	QSignalSpy spy19(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::show_ccr_sensorsChanged);
	QSignalSpy spy20(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::show_ccr_setpointChanged);
	QSignalSpy spy21(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::show_icdChanged);
	QSignalSpy spy22(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::show_pictures_in_profileChanged);
	QSignalSpy spy23(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::show_sacChanged);
	QSignalSpy spy24(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::show_scr_ocpo2Changed);
	QSignalSpy spy25(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::tankbarChanged);
	QSignalSpy spy26(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::vpmb_conservatismChanged);
	QSignalSpy spy27(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::zoomed_plotChanged);
	QSignalSpy spy28(qPrefTechnicalDetails::instance(), &qPrefTechnicalDetails::infoboxChanged);

	prefs.calcalltissues = true;
	qPrefTechnicalDetails::set_calcalltissues(false);
	prefs.calcceiling = true;
	qPrefTechnicalDetails::set_calcceiling(false);
	prefs.calcceiling3m = true;
	qPrefTechnicalDetails::set_calcceiling3m(false);
	prefs.calcndltts = true;
	qPrefTechnicalDetails::set_calcndltts(false);
	prefs.dcceiling = true;
	qPrefTechnicalDetails::set_dcceiling(false);
	qPrefTechnicalDetails::set_display_deco_mode(VPMB);
	prefs.ead = true;
	qPrefTechnicalDetails::set_ead(false);
	qPrefTechnicalDetails::set_gfhigh(-29);
	qPrefTechnicalDetails::set_gflow(-24);
	prefs.gf_low_at_maxdepth = true;
	qPrefTechnicalDetails::set_gf_low_at_maxdepth(false);
	prefs.hrgraph = true;
	qPrefTechnicalDetails::set_hrgraph(false);
	prefs.mod = true;
	qPrefTechnicalDetails::set_mod(false);
	qPrefTechnicalDetails::set_modpO2(-1.12);
	prefs.percentagegraph = true;
	qPrefTechnicalDetails::set_percentagegraph(false);
	prefs.redceiling = true;
	qPrefTechnicalDetails::set_redceiling(false);
	prefs.rulergraph = true;
	qPrefTechnicalDetails::set_rulergraph(false);
	prefs.show_ccr_sensors = true;
	qPrefTechnicalDetails::set_show_ccr_sensors(false);
	prefs.show_ccr_setpoint = true;
	qPrefTechnicalDetails::set_show_ccr_setpoint(false);
	prefs.show_icd = true;
	qPrefTechnicalDetails::set_show_icd(false);
	prefs.show_pictures_in_profile = true;
	qPrefTechnicalDetails::set_show_pictures_in_profile(false);
	prefs.show_sac = true;
	qPrefTechnicalDetails::set_show_sac(false);
	prefs.show_scr_ocpo2 = true;
	qPrefTechnicalDetails::set_show_scr_ocpo2(false);
	prefs.tankbar = true;
	qPrefTechnicalDetails::set_tankbar(false);
	qPrefTechnicalDetails::set_vpmb_conservatism(-64);
	prefs.zoomed_plot = true;
	qPrefTechnicalDetails::set_zoomed_plot(false);
	prefs.infobox = true;
	qPrefTechnicalDetails::set_infobox(false);

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);
	QCOMPARE(spy6.count(), 1);
	QCOMPARE(spy8.count(), 1);
	QCOMPARE(spy9.count(), 1);
	QCOMPARE(spy10.count(), 1);
	QCOMPARE(spy11.count(), 1);
	QCOMPARE(spy12.count(), 1);
	QCOMPARE(spy13.count(), 1);
	QCOMPARE(spy14.count(), 1);
	QCOMPARE(spy15.count(), 1);
	QCOMPARE(spy16.count(), 1);
	QCOMPARE(spy17.count(), 1);
	QCOMPARE(spy19.count(), 1);
	QCOMPARE(spy20.count(), 1);
	QCOMPARE(spy21.count(), 1);
	QCOMPARE(spy22.count(), 1);
	QCOMPARE(spy23.count(), 1);
	QCOMPARE(spy24.count(), 1);
	QCOMPARE(spy25.count(), 1);
	QCOMPARE(spy26.count(), 1);
	QCOMPARE(spy27.count(), 1);
	QCOMPARE(spy28.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toBool() == false);
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
	QVERIFY(spy3.takeFirst().at(0).toBool() == false);
	QVERIFY(spy4.takeFirst().at(0).toBool() == false);
	QVERIFY(spy5.takeFirst().at(0).toBool() == false);
	QVERIFY(spy6.takeFirst().at(0).toInt() == VPMB);
	QVERIFY(spy8.takeFirst().at(0).toBool() == false);
	QVERIFY(spy9.takeFirst().at(0).toInt() == -29);
	QVERIFY(spy10.takeFirst().at(0).toInt() == -24);
	QVERIFY(spy11.takeFirst().at(0).toBool() == false);
	QVERIFY(spy12.takeFirst().at(0).toBool() == false);
	QVERIFY(spy13.takeFirst().at(0).toBool() == false);
	QVERIFY(spy14.takeFirst().at(0).toDouble() == -1.12);
	QVERIFY(spy15.takeFirst().at(0).toBool() == false);
	QVERIFY(spy16.takeFirst().at(0).toBool() == false);
	QVERIFY(spy17.takeFirst().at(0).toBool() == false);
	QVERIFY(spy19.takeFirst().at(0).toBool() == false);
	QVERIFY(spy20.takeFirst().at(0).toBool() == false);
	QVERIFY(spy21.takeFirst().at(0).toBool() == false);
	QVERIFY(spy22.takeFirst().at(0).toBool() == false);
	QVERIFY(spy23.takeFirst().at(0).toBool() == false);
	QVERIFY(spy24.takeFirst().at(0).toBool() == false);
	QVERIFY(spy25.takeFirst().at(0).toBool() == false);
	QVERIFY(spy26.takeFirst().at(0).toInt() == -64);
	QVERIFY(spy27.takeFirst().at(0).toBool() == false);
	QVERIFY(spy28.takeFirst().at(0).toBool() == false);
}


QTEST_MAIN(TestQPrefTechnicalDetails)
