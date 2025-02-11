// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFTECHNICALDETAILS_H
#define QPREFTECHNICALDETAILS_H
#include "core/pref.h"

#include <QObject>

class qPrefTechnicalDetails : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool allowOcGasAsDiluent READ allowOcGasAsDiluent WRITE set_allowOcGasAsDiluent NOTIFY allowOcGasAsDiluentChanged)
	Q_PROPERTY(bool calcalltissues READ calcalltissues WRITE set_calcalltissues NOTIFY calcalltissuesChanged)
	Q_PROPERTY(bool calcceiling READ calcceiling WRITE set_calcceiling NOTIFY calcceilingChanged)
	Q_PROPERTY(bool calcceiling3m READ calcceiling3m WRITE set_calcceiling3m NOTIFY calcceiling3mChanged)
	Q_PROPERTY(bool calcndltts READ calcndltts WRITE set_calcndltts NOTIFY calcndlttsChanged)
	Q_PROPERTY(bool decoinfo READ decoinfo WRITE set_decoinfo NOTIFY decoinfoChanged)
	Q_PROPERTY(bool dcceiling READ dcceiling WRITE set_dcceiling NOTIFY dcceilingChanged)
	Q_PROPERTY(deco_mode display_deco_mode READ display_deco_mode WRITE set_display_deco_mode NOTIFY display_deco_modeChanged)
	Q_PROPERTY(bool ead READ ead WRITE set_ead NOTIFY eadChanged)
	Q_PROPERTY(double gasplot_frac READ gasplot_frac WRITE set_gasplot_frac NOTIFY gasplot_fracChanged)
	Q_PROPERTY(int gfhigh READ gfhigh WRITE set_gfhigh NOTIFY gfhighChanged)
	Q_PROPERTY(int gflow READ gflow WRITE set_gflow NOTIFY gflowChanged)
	Q_PROPERTY(bool gf_low_at_maxdepth READ gf_low_at_maxdepth WRITE set_gf_low_at_maxdepth NOTIFY gf_low_at_maxdepthChanged)
	Q_PROPERTY(bool hrgraph READ hrgraph WRITE set_hrgraph NOTIFY hrgraphChanged)
	Q_PROPERTY(bool mod READ mod WRITE set_mod NOTIFY modChanged)
	Q_PROPERTY(double modpO2 READ modpO2 WRITE set_modpO2 NOTIFY modpO2Changed)
	Q_PROPERTY(bool percentagegraph READ percentagegraph WRITE set_percentagegraph NOTIFY percentagegraphChanged)
	Q_PROPERTY(bool redceiling READ redceiling WRITE set_redceiling NOTIFY redceilingChanged)
	Q_PROPERTY(bool rulergraph READ rulergraph WRITE set_rulergraph NOTIFY rulergraphChanged)
	Q_PROPERTY(bool show_ccr_sensors READ show_ccr_sensors WRITE set_show_ccr_sensors NOTIFY show_ccr_sensorsChanged)
	Q_PROPERTY(bool show_ccr_setpoint READ show_ccr_setpoint WRITE set_show_ccr_setpoint NOTIFY show_ccr_setpointChanged)
	Q_PROPERTY(bool show_icd READ show_icd WRITE set_show_icd NOTIFY show_icdChanged)
	Q_PROPERTY(bool show_pictures_in_profile READ show_pictures_in_profile WRITE set_show_pictures_in_profile NOTIFY show_pictures_in_profileChanged)
	Q_PROPERTY(bool show_sac READ show_sac WRITE set_show_sac NOTIFY show_sacChanged)
	Q_PROPERTY(bool show_scr_ocpo2 READ show_scr_ocpo2 WRITE set_show_scr_ocpo2 NOTIFY show_scr_ocpo2Changed)
	Q_PROPERTY(bool tankbar READ tankbar WRITE set_tankbar NOTIFY tankbarChanged)
	Q_PROPERTY(int vpmb_conservatism READ vpmb_conservatism WRITE set_vpmb_conservatism NOTIFY vpmb_conservatismChanged)
	Q_PROPERTY(bool zoomed_plot READ zoomed_plot WRITE set_zoomed_plot NOTIFY zoomed_plotChanged)
	Q_PROPERTY(bool infobox READ infobox WRITE set_infobox NOTIFY infoboxChanged)

public:
	static qPrefTechnicalDetails *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static bool allowOcGasAsDiluent() { return prefs.allowOcGasAsDiluent; }
	static bool calcalltissues() { return prefs.calcalltissues; }
	static bool calcceiling() { return prefs.calcceiling; }
	static bool calcceiling3m() { return prefs.calcceiling3m; }
	static bool calcndltts() { return prefs.calcndltts; }
	static bool decoinfo() { return prefs.decoinfo; }
	static bool dcceiling() { return prefs.dcceiling; }
	static deco_mode display_deco_mode() { return prefs.display_deco_mode; }
	static double gasplot_frac() { return prefs.gasplot_frac; }
	static bool ead() { return prefs.ead; }
	static int gfhigh() { return prefs.gfhigh; }
	static int gflow() { return prefs.gflow; }
	static bool gf_low_at_maxdepth() { return prefs.gf_low_at_maxdepth; }
	static bool hrgraph() { return prefs.hrgraph; }
	static bool mod() { return prefs.mod; }
	static double modpO2() { return prefs.modpO2; }
	static bool percentagegraph() { return prefs.percentagegraph; }
	static bool redceiling() { return prefs.redceiling; }
	static bool rulergraph() { return prefs.rulergraph; }
	static bool show_ccr_sensors() { return prefs.show_ccr_sensors; }
	static bool show_ccr_setpoint() { return prefs.show_ccr_setpoint; }
	static bool show_icd() { return prefs.show_icd; }
	static bool show_pictures_in_profile() { return prefs.show_pictures_in_profile; }
	static bool show_sac() { return prefs.show_sac; }
	static bool show_scr_ocpo2() { return prefs.show_scr_ocpo2; }
	static bool tankbar() { return prefs.tankbar; }
	static int  vpmb_conservatism() { return prefs.vpmb_conservatism; }
	static bool zoomed_plot() { return prefs.zoomed_plot; }
	static bool infobox() { return prefs.infobox; }

public slots:
	static void set_allowOcGasAsDiluent(bool value);
	static void set_calcalltissues(bool value);
	static void set_calcceiling(bool value);
	static void set_calcceiling3m(bool value);
	static void set_calcndltts(bool value);
	static void set_decoinfo(bool value);
	static void set_dcceiling(bool value);
	static void set_display_deco_mode(deco_mode value);
	static void set_ead(bool value);
	static void set_gasplot_frac(double value);
	static void set_gfhigh(int value);
	static void set_gflow(int value);
	static void set_gf_low_at_maxdepth(bool value);
	static void set_hrgraph(bool value);
	static void set_mod(bool value);
	static void set_modpO2(double value);
	static void set_percentagegraph(bool value);
	static void set_redceiling(bool value);
	static void set_rulergraph(bool value);
	static void set_show_ccr_sensors(bool value);
	static void set_show_ccr_setpoint(bool value);
	static void set_show_icd(bool value);
	static void set_show_pictures_in_profile(bool value);
	static void set_show_sac(bool value);
	static void set_show_scr_ocpo2(bool value);
	static void set_tankbar(bool value);
	static void set_vpmb_conservatism(int value);
	static void set_zoomed_plot(bool value);
	static void set_infobox(bool value);

signals:
	void allowOcGasAsDiluentChanged(bool value);
	void calcalltissuesChanged(bool value);
	void calcceilingChanged(bool value);
	void calcceiling3mChanged(bool value);
	void calcndlttsChanged(bool value);
	void decoinfoChanged(bool value);
	void dcceilingChanged(bool value);
	void display_deco_modeChanged(deco_mode value);
	void eadChanged(bool value);
	void gasplot_fracChanged(double value);
	void gfhighChanged(int value);
	void gflowChanged(int value);
	void gf_low_at_maxdepthChanged(bool value);
	void hrgraphChanged(bool value);
	void modChanged(bool value);
	void modpO2Changed(double value);
	void percentagegraphChanged(bool value);
	void redceilingChanged(bool value);
	void rulergraphChanged(bool value);
	void show_ccr_sensorsChanged(bool value);
	void show_ccr_setpointChanged(bool value);
	void show_icdChanged(bool value);
	void show_pictures_in_profileChanged(bool value);
	void show_scr_ocpo2Changed(bool value);
	void show_sacChanged(bool value);
	void tankbarChanged(bool value);
	void vpmb_conservatismChanged(int value);
	void zoomed_plotChanged(bool value);
	void infoboxChanged(bool value);

private:
	qPrefTechnicalDetails() {}

	static void disk_allowOcGasAsDiluent(bool doSync);
	static void disk_calcalltissues(bool doSync);
	static void disk_calcceiling(bool doSync);
	static void disk_calcceiling3m(bool doSync);
	static void disk_calcndltts(bool doSync);
	static void disk_decoinfo(bool doSync);
	static void disk_dcceiling(bool doSync);
	static void disk_display_deco_mode(bool doSync);
	static void disk_ead(bool doSync);
	static void disk_gasplot_frac(bool doSync);
	static void disk_gfhigh(bool doSync);
	static void disk_gflow(bool doSync);
	static void disk_gf_low_at_maxdepth(bool doSync);
	static void disk_hrgraph(bool doSync);
	static void disk_mod(bool doSync);
	static void disk_modpO2(bool doSync);
	static void disk_percentagegraph(bool doSync);
	static void disk_redceiling(bool doSync);
	static void disk_rulergraph(bool doSync);
	static void disk_show_ccr_sensors(bool doSync);
	static void disk_show_ccr_setpoint(bool doSync);
	static void disk_show_icd(bool doSync);
	static void disk_show_pictures_in_profile(bool doSync);
	static void disk_show_sac(bool doSync);
	static void disk_show_scr_ocpo2(bool doSync);
	static void disk_tankbar(bool doSync);
	static void disk_vpmb_conservatism(bool doSync);
	static void disk_zoomed_plot(bool doSync);
	static void disk_infobox(bool doSync);
};

#endif
