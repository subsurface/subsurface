// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFTECHNICALDETAILS_H
#define QPREFTECHNICALDETAILS_H

#include <QObject>
#include "core/pref.h"


class qPrefTechnicalDetails : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool buehlmann READ buehlmann WRITE setBuehlmann NOTIFY buehlmannChanged);
	Q_PROPERTY(bool calcalltissues READ calcalltissues WRITE setCalcalltissues NOTIFY calcalltissuesChanged);
	Q_PROPERTY(bool calcceiling READ calcceiling WRITE setCalcceiling NOTIFY calcceilingChanged);
	Q_PROPERTY(bool calcceiling3m READ calcceiling3m WRITE setCalcceiling3m NOTIFY calcceiling3mChanged);
	Q_PROPERTY(bool calcndltts READ calcndltts WRITE setCalcndltts NOTIFY calcndlttsChanged);
	Q_PROPERTY(bool dcceiling READ dcceiling WRITE setDCceiling NOTIFY dcceilingChanged);
	Q_PROPERTY(deco_mode deco_mode READ decoMode WRITE setDecoMode NOTIFY decoModeChanged);
	Q_PROPERTY(bool display_unused_tanks READ displayUnusedTanks WRITE setDisplayUnusedTanks NOTIFY displayUnusedTanksChanged);
	Q_PROPERTY(bool ead READ ead WRITE setEad NOTIFY eadChanged);
	Q_PROPERTY(int gfhigh READ gfhigh WRITE setGfhigh NOTIFY gfhighChanged);
	Q_PROPERTY(int gflow READ gflow WRITE setGflow NOTIFY gflowChanged);
	Q_PROPERTY(int gf_low_at_maxdepth READ gflowAtMaxDepth WRITE setGflowAtMaxDepth NOTIFY gflowAtMaxDepthChanged);
	Q_PROPERTY(bool hrgraph READ hrgraph WRITE setHRgraph NOTIFY hrgraphChanged);
	Q_PROPERTY(bool mod READ mod WRITE setMod NOTIFY modChanged);
	Q_PROPERTY(double modpO2 READ modpO2 WRITE setModpO2 NOTIFY modpO2Changed);
	Q_PROPERTY(bool percentagegraph READ percentageGraph WRITE setPercentageGraph NOTIFY percentageGraphChanged);
	Q_PROPERTY(bool redceiling READ redceiling WRITE setRedceiling NOTIFY redceilingChanged);
	Q_PROPERTY(bool rulergraph READ rulerGraph WRITE setRulerGraph NOTIFY rulerGraphChanged);
	Q_PROPERTY(bool show_average_depth READ showAverageDepth WRITE setShowAverageDepth NOTIFY showAverageDepthChanged);
	Q_PROPERTY(bool show_ccr_sensors READ showCCRSensors WRITE setShowCCRSensors NOTIFY showCCRSensorsChanged);
	Q_PROPERTY(bool show_ccr_setpoint READ showCCRSetpoint WRITE setShowCCRSetpoint NOTIFY showCCRSetpointChanged);
	Q_PROPERTY(bool show_icd READ showIcd WRITE setShowIcd NOTIFY showIcdChanged);
	Q_PROPERTY(bool show_pictures_in_profile READ showPicturesInProfile WRITE setShowPicturesInProfile NOTIFY showPicturesInProfileChanged);
	Q_PROPERTY(bool show_sac READ showSac WRITE setShowSac NOTIFY showSacChanged);
	Q_PROPERTY(bool show_scr_ocpo2 READ showSCROCpO2 WRITE setShowSCROCpO2 NOTIFY showSCROCpO2Changed);
	Q_PROPERTY(bool tankbar READ tankBar WRITE setTankBar NOTIFY tankBarChanged);
	Q_PROPERTY(short vpmb_conservatism READ vpmbConservatism WRITE setVpmbConservatism NOTIFY vpmbConservatismChanged);
	Q_PROPERTY(bool zoomed_plot READ zoomedPlot WRITE setZoomedPlot NOTIFY zoomedPlotChanged);

public:
	qPrefTechnicalDetails(QObject *parent = NULL) : QObject(parent) {};
	~qPrefTechnicalDetails() {};
	static qPrefTechnicalDetails *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

	bool buehlmann() const;
	bool calcalltissues() const;
	bool calcceiling() const;
	bool calcceiling3m() const;
	bool calcndltts() const;
	bool dcceiling() const;
	deco_mode decoMode() const;
	bool displayUnusedTanks() const;
	bool ead() const;
	int gfhigh() const;
	int gflow() const;
	int gflowAtMaxDepth() const;
	bool hrgraph() const;
	bool mod() const;
	double modpO2() const;
	bool percentageGraph() const;
	bool redceiling() const;
	bool rulerGraph() const;
	bool showAverageDepth() const;
	bool showCCRSensors() const;
	bool showCCRSetpoint() const;
	bool showIcd() const;
	bool showPicturesInProfile() const;
	bool showSac() const;
	bool showSCROCpO2() const;
	bool tankBar() const;
	short vpmbConservatism() const;
	bool zoomedPlot() const;

public slots:
	void setBuehlmann(bool value);
	void setCalcalltissues(bool value);
	void setCalcceiling(bool value);
	void setCalcceiling3m(bool value);
	void setCalcndltts(bool value);
	void setDCceiling(bool value);
	void setDecoMode(deco_mode value);
	void setDisplayUnusedTanks(bool value);
	void setEad(bool value);
	void setGfhigh(int value);
	void setGflow(int value);
	void setGflowAtMaxDepth(int value);
	void setHRgraph(bool value);
	void setMod(bool value);
	void setModpO2(double value);
	void setPercentageGraph(bool value);
	void setRedceiling(bool value);
	void setRulerGraph(bool value);
	void setShowAverageDepth(bool value);
	void setShowCCRSensors(bool value);
	void setShowCCRSetpoint(bool value);
	void setShowIcd(bool value);
	void setShowPicturesInProfile(bool value);
	void setShowSac(bool value);
	void setShowSCROCpO2(bool value);
	void setTankBar(bool value);
	void setVpmbConservatism(short value);
	void setZoomedPlot(bool value);

signals:
	void buehlmannChanged(bool value);
	void calcalltissuesChanged(bool value);
	void calcceilingChanged(bool value);
	void calcceiling3mChanged(bool value);
	void calcndlttsChanged(bool value);
	void dcceilingChanged(bool value);
	void decoModeChanged(deco_mode value);
	void displayUnusedTanksChanged(bool value);
	void eadChanged(bool value);
	void gfhighChanged(int value);
	void gflowChanged(int value);
	void gflowAtMaxDepthChanged(int value);
	void hrgraphChanged(bool value);
	void modChanged(bool value);
	void modpO2Changed(double value);
	void percentageGraphChanged(bool value);
	void redceilingChanged(bool value);
	void rulerGraphChanged(bool value);
	void showAverageDepthChanged(bool value);
	void showCCRSensorsChanged(bool value);
	void showCCRSetpointChanged(bool value);
	void showIcdChanged(bool value);
	void showPicturesInProfileChanged(bool value);
	void showSCROCpO2Changed(bool value);
	void showSacChanged(bool value);
	void tankBarChanged(bool value);
	void vpmbConservatismChanged(short value);
	void zoomedPlotChanged(bool value);

private:
	const QString group = QStringLiteral("TecDetails");
	static qPrefTechnicalDetails *m_instance;

	void diskCalcalltissues(bool doSync);
	void diskCalcceiling(bool doSync);
	void diskCalcceiling3m(bool doSync);
	void diskCalcndltts(bool doSync);
	void diskDCceiling(bool doSync);
	void diskDecoMode(bool doSync);
	void diskDisplayUnusedTanks(bool doSync);
	void diskEad(bool doSync);
	void diskGfhigh(bool doSync);
	void diskGflow(bool doSync);
	void diskGflowAtMaxDepth(bool doSync);
	void diskHRgraph(bool doSync);
	void diskMod(bool doSync);
	void diskModpO2(bool doSync);
	void diskPercentageGraph(bool doSync);
	void diskRedceiling(bool doSync);
	void diskRulerGraph(bool doSync);
	void diskShowAverageDepth(bool doSync);
	void diskShowCCRSensors(bool doSync);
	void diskShowCCRSetpoint(bool doSync);
	void diskShowIcd(bool doSync);
	void diskShowPicturesInProfile(bool doSync);
	void diskShowSac(bool doSync);
	void diskShowSCROCpO2(bool doSync);
	void diskTankBar(bool doSync);
	void diskVpmbConservatism(bool doSync);
	void diskZoomedPlot(bool doSync);
};

#endif
