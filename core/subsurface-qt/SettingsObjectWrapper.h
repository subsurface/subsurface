// SPDX-License-Identifier: GPL-2.0
#ifndef SETTINGSOBJECTWRAPPER_H
#define SETTINGSOBJECTWRAPPER_H

#include <QObject>
#include <QDate>

#include "../pref.h"
#include "../prefs-macros.h"

/* Wrapper class for the Settings. This will allow
 * seamlessy integration of the settings with the QML
 * and QWidget frontends. This class will be huge, since
 * I need tons of properties, one for each option. */

class DiveComputerSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString vendor READ dc_vendor WRITE setVendor NOTIFY vendorChanged)
	Q_PROPERTY(QString product READ dc_product WRITE setProduct NOTIFY productChanged)
	Q_PROPERTY(QString device READ dc_device WRITE setDevice NOTIFY deviceChanged)
	Q_PROPERTY(QString device_name READ dc_device_name WRITE setDeviceName NOTIFY deviceNameChanged)
	Q_PROPERTY(int download_mode READ downloadMode WRITE setDownloadMode NOTIFY downloadModeChanged)
public:
	DiveComputerSettings(QObject *parent);
	QString dc_vendor() const;
	QString dc_product() const;
	QString dc_device() const;
	QString dc_device_name() const;
	int downloadMode() const;

public slots:
	void setVendor(const QString& vendor);
	void setProduct(const QString& product);
	void setDevice(const QString& device);
	void setDeviceName(const QString& device_name);
	void setDownloadMode(int mode);

signals:
	void vendorChanged(const QString& vendor);
	void productChanged(const QString& product);
	void deviceChanged(const QString& device);
	void deviceNameChanged(const QString& device_name);
	void downloadModeChanged(int mode);
private:
	const QString group = QStringLiteral("DiveComputer");

};

class UpdateManagerSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool dont_check_for_updates READ dontCheckForUpdates WRITE setDontCheckForUpdates NOTIFY dontCheckForUpdatesChanged)
	Q_PROPERTY(QString last_version_used READ lastVersionUsed WRITE setLastVersionUsed NOTIFY lastVersionUsedChanged)
	Q_PROPERTY(QDate next_check READ nextCheck WRITE nextCheckChanged)
public:
	UpdateManagerSettings(QObject *parent);
	bool dontCheckForUpdates() const;
	bool dontCheckExists() const;
	QString lastVersionUsed() const;
	QDate nextCheck() const;

public slots:
	void setDontCheckForUpdates(bool value);
	void setLastVersionUsed(const QString& value);
	void setNextCheck(const QDate& date);

signals:
	void dontCheckForUpdatesChanged(bool value);
	void lastVersionUsedChanged(const QString& value);
	void nextCheckChanged(const QDate& date);
private:
	const QString group = QStringLiteral("UpdateManager");
};

/* Control the state of the Partial Pressure Graphs preferences */
class PartialPressureGasSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(short show_po2           READ showPo2         WRITE setShowPo2         NOTIFY showPo2Changed)
	Q_PROPERTY(short show_pn2           READ showPn2         WRITE setShowPn2         NOTIFY showPn2Changed)
	Q_PROPERTY(short show_phe	    READ showPhe         WRITE setShowPhe         NOTIFY showPheChanged)
	Q_PROPERTY(double po2_threshold_min READ po2ThresholdMin WRITE setPo2ThresholdMin NOTIFY po2ThresholdMinChanged)
	Q_PROPERTY(double po2_threshold_max READ po2ThresholdMax WRITE setPo2ThresholdMax NOTIFY po2ThresholdMaxChanged)
	Q_PROPERTY(double pn2_threshold     READ pn2Threshold    WRITE setPn2Threshold    NOTIFY pn2ThresholdChanged)
	Q_PROPERTY(double phe_threshold     READ pheThreshold    WRITE setPheThreshold    NOTIFY pheThresholdChanged)

public:
	PartialPressureGasSettings(QObject *parent);
	short showPo2() const;
	short showPn2() const;
	short showPhe() const;
	double po2ThresholdMin() const;
	double po2ThresholdMax() const;
	double pn2Threshold() const;
	double pheThreshold() const;

public slots:
	void setShowPo2(short value);
	void setShowPn2(short value);
	void setShowPhe(short value);
	void setPo2ThresholdMin(double value);
	void setPo2ThresholdMax(double value);
	void setPn2Threshold(double value);
	void setPheThreshold(double value);

signals:
	void showPo2Changed(short value);
	void showPn2Changed(short value);
	void showPheChanged(short value);
	void po2ThresholdMaxChanged(double value);
	void po2ThresholdMinChanged(double value);
	void pn2ThresholdChanged(double value);
	void pheThresholdChanged(double value);

private:
	const QString group = QStringLiteral("TecDetails");
};

class TechnicalDetailsSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(double modpO2         READ modpO2          WRITE setModpO2          NOTIFY modpO2Changed)
	Q_PROPERTY(bool ead              READ ead             WRITE setEad             NOTIFY eadChanged)
	Q_PROPERTY(bool mod              READ mod             WRITE setMod             NOTIFY modChanged)
	Q_PROPERTY(bool dcceiling        READ dcceiling       WRITE setDCceiling       NOTIFY dcceilingChanged)
	Q_PROPERTY(bool redceiling       READ redceiling      WRITE setRedceiling      NOTIFY redceilingChanged)
	Q_PROPERTY(bool calcceiling      READ calcceiling     WRITE setCalcceiling     NOTIFY calcceilingChanged)
	Q_PROPERTY(bool calcceiling3m    READ calcceiling3m   WRITE setCalcceiling3m   NOTIFY calcceiling3mChanged)
	Q_PROPERTY(bool calcalltissues   READ calcalltissues  WRITE setCalcalltissues  NOTIFY calcalltissuesChanged)
	Q_PROPERTY(bool calcndltts       READ calcndltts      WRITE setCalcndltts      NOTIFY calcndlttsChanged)
	Q_PROPERTY(bool buehlmann        READ buehlmann       WRITE setBuehlmann       NOTIFY buehlmannChanged)
	Q_PROPERTY(int gflow            READ gflow           WRITE setGflow           NOTIFY gflowChanged)
	Q_PROPERTY(int gfhigh           READ gfhigh          WRITE setGfhigh          NOTIFY gfhighChanged)
	Q_PROPERTY(short vpmb_conservatism READ vpmbConservatism WRITE setVpmbConservatism NOTIFY vpmbConservatismChanged)
	Q_PROPERTY(bool hrgraph          READ hrgraph         WRITE setHRgraph         NOTIFY hrgraphChanged)
	Q_PROPERTY(bool tankbar          READ tankBar         WRITE setTankBar         NOTIFY tankBarChanged)
	Q_PROPERTY(bool percentagegraph  READ percentageGraph WRITE setPercentageGraph NOTIFY percentageGraphChanged)
	Q_PROPERTY(bool rulergraph       READ rulerGraph      WRITE setRulerGraph      NOTIFY rulerGraphChanged)
	Q_PROPERTY(bool show_ccr_setpoint READ showCCRSetpoint WRITE setShowCCRSetpoint NOTIFY showCCRSetpointChanged)
	Q_PROPERTY(bool show_ccr_sensors  READ showCCRSensors  WRITE setShowCCRSensors  NOTIFY showCCRSensorsChanged)
	Q_PROPERTY(bool zoomed_plot      READ zoomedPlot      WRITE setZoomedPlot      NOTIFY zoomedPlotChanged)
	Q_PROPERTY(bool show_sac             READ showSac            WRITE setShowSac            NOTIFY showSacChanged)
	Q_PROPERTY(bool display_unused_tanks READ displayUnusedTanks WRITE setDisplayUnusedTanks NOTIFY displayUnusedTanksChanged)
	Q_PROPERTY(bool show_average_depth   READ showAverageDepth   WRITE setShowAverageDepth   NOTIFY showAverageDepthChanged)
	Q_PROPERTY(bool show_pictures_in_profile READ showPicturesInProfile WRITE setShowPicturesInProfile NOTIFY showPicturesInProfileChanged)
	Q_PROPERTY(deco_mode deco READ deco WRITE setDecoMode NOTIFY decoModeChanged)

public:
	TechnicalDetailsSettings(QObject *parent);

	double modpO2() const;
	bool ead() const;
	bool mod() const;
	bool dcceiling() const;
	bool redceiling() const;
	bool calcceiling() const;
	bool calcceiling3m() const;
	bool calcalltissues() const;
	bool calcndltts() const;
	bool buehlmann() const;
	int gflow() const;
	int gfhigh() const;
	short vpmbConservatism() const;
	bool hrgraph() const;
	bool tankBar() const;
	bool percentageGraph() const;
	bool rulerGraph() const;
	bool showCCRSetpoint() const;
	bool showCCRSensors() const;
	bool zoomedPlot() const;
	bool showSac() const;
	bool displayUnusedTanks() const;
	bool showAverageDepth() const;
	bool showPicturesInProfile() const;
	deco_mode deco() const;

public slots:
	void setMod(bool value);
	void setModpO2(double value);
	void setEad(bool value);
	void setDCceiling(bool value);
	void setRedceiling(bool value);
	void setCalcceiling(bool value);
	void setCalcceiling3m(bool value);
	void setCalcalltissues(bool value);
	void setCalcndltts(bool value);
	void setBuehlmann(bool value);
	void setGflow(int value);
	void setGfhigh(int value);
	void setVpmbConservatism(short);
	void setHRgraph(bool value);
	void setTankBar(bool value);
	void setPercentageGraph(bool value);
	void setRulerGraph(bool value);
	void setShowCCRSetpoint(bool value);
	void setShowCCRSensors(bool value);
	void setZoomedPlot(bool value);
	void setShowSac(bool value);
	void setDisplayUnusedTanks(bool value);
	void setShowAverageDepth(bool value);
	void setShowPicturesInProfile(bool value);
	void setDecoMode(deco_mode d);

signals:
	void modpO2Changed(double value);
	void eadChanged(bool value);
	void modChanged(bool value);
	void dcceilingChanged(bool value);
	void redceilingChanged(bool value);
	void calcceilingChanged(bool value);
	void calcceiling3mChanged(bool value);
	void calcalltissuesChanged(bool value);
	void calcndlttsChanged(bool value);
	void buehlmannChanged(bool value);
	void gflowChanged(int value);
	void gfhighChanged(int value);
	void vpmbConservatismChanged(short value);
	void hrgraphChanged(bool value);
	void tankBarChanged(bool value);
	void percentageGraphChanged(bool value);
	void rulerGraphChanged(bool value);
	void showCCRSetpointChanged(bool value);
	void showCCRSensorsChanged(bool value);
	void zoomedPlotChanged(bool value);
	void showSacChanged(bool value);
	void gfLowAtMaxDepthChanged(bool value);
	void displayUnusedTanksChanged(bool value);
	void showAverageDepthChanged(bool value);
	void showPicturesInProfileChanged(bool value);
	void decoModeChanged(deco_mode m);

private:
	const QString group = QStringLiteral("TecDetails");
};

/* Control the state of the Facebook preferences */
class FacebookSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString  accessToken READ accessToken WRITE setAccessToken NOTIFY accessTokenChanged)
	Q_PROPERTY(QString  userId      READ userId      WRITE setUserId      NOTIFY userIdChanged)
	Q_PROPERTY(QString  albumId     READ albumId     WRITE setAlbumId     NOTIFY albumIdChanged)

public:
	FacebookSettings(QObject *parent);
	QString accessToken() const;
	QString userId() const;
	QString albumId() const;

public slots:
	void setAccessToken (const QString& value);
	void setUserId(const QString& value);
	void setAlbumId(const QString& value);

signals:
	void accessTokenChanged(const QString& value);
	void userIdChanged(const QString& value);
	void albumIdChanged(const QString& value);
private:
	QString group;
	QString subgroup;
};

/* Control the state of the Geocoding preferences */
class GeocodingPreferences : public QObject {
	Q_OBJECT
	Q_PROPERTY(taxonomy_category first_category     READ firstTaxonomyCategory  WRITE setFirstTaxonomyCategory  NOTIFY firstTaxonomyCategoryChanged)
	Q_PROPERTY(taxonomy_category second_category    READ secondTaxonomyCategory WRITE setSecondTaxonomyCategory NOTIFY secondTaxonomyCategoryChanged)
	Q_PROPERTY(taxonomy_category third_category     READ thirdTaxonomyCategory  WRITE setThirdTaxonomyCategory  NOTIFY thirdTaxonomyCategoryChanged)
public:
	GeocodingPreferences(QObject *parent);
	taxonomy_category firstTaxonomyCategory() const;
	taxonomy_category secondTaxonomyCategory() const;
	taxonomy_category thirdTaxonomyCategory() const;

public slots:
	void setFirstTaxonomyCategory(taxonomy_category value);
	void setSecondTaxonomyCategory(taxonomy_category value);
	void setThirdTaxonomyCategory(taxonomy_category value);

signals:
	void firstTaxonomyCategoryChanged(taxonomy_category value);
	void secondTaxonomyCategoryChanged(taxonomy_category value);
	void thirdTaxonomyCategoryChanged(taxonomy_category value);
private:
	const QString group = QStringLiteral("geocoding");
};

class ProxySettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(int type     READ type WRITE setType NOTIFY typeChanged)
	Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
	Q_PROPERTY(int port     READ port WRITE setPort NOTIFY portChanged)
	Q_PROPERTY(bool  auth READ auth WRITE setAuth NOTIFY authChanged)
	Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)
	Q_PROPERTY(QString pass READ pass WRITE setPass NOTIFY passChanged)

public:
	ProxySettings(QObject *parent);
	int type() const;
	QString host() const;
	int port() const;
	bool auth() const;
	QString user() const;
	QString pass() const;

public slots:
	void setType(int value);
	void setHost(const QString& value);
	void setPort(int value);
	void setAuth(bool value);
	void setUser(const QString& value);
	void setPass(const QString& value);

signals:
	void typeChanged(int value);
	void hostChanged(const QString& value);
	void portChanged(int value);
	void authChanged(bool value);
	void userChanged(const QString& value);
	void passChanged(const QString& value);
private:
	const QString group = QStringLiteral("Network");
};

class CloudStorageSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString password          READ password           WRITE setPassword           NOTIFY passwordChanged)
	Q_PROPERTY(QString newpassword       READ newPassword        WRITE setNewPassword        NOTIFY newPasswordChanged)
	Q_PROPERTY(QString email             READ email              WRITE setEmail              NOTIFY emailChanged)
	Q_PROPERTY(QString email_encoded     READ emailEncoded       WRITE setEmailEncoded       NOTIFY emailEncodedChanged)
	Q_PROPERTY(QString userid            READ userId             WRITE setUserId             NOTIFY userIdChanged)
	Q_PROPERTY(QString base_url          READ baseUrl            WRITE setBaseUrl            NOTIFY baseUrlChanged)
	Q_PROPERTY(QString git_url           READ gitUrl             WRITE setGitUrl             NOTIFY gitUrlChanged)
	Q_PROPERTY(short save_userid_local  READ saveUserIdLocal WRITE setSaveUserIdLocal NOTIFY saveUserIdLocalChanged)
	Q_PROPERTY(bool git_local_only       READ gitLocalOnly       WRITE setGitLocalOnly       NOTIFY gitLocalOnlyChanged)
	Q_PROPERTY(bool save_password_local  READ savePasswordLocal  WRITE setSavePasswordLocal  NOTIFY savePasswordLocalChanged)
	Q_PROPERTY(short verification_status READ verificationStatus WRITE setVerificationStatus NOTIFY verificationStatusChanged)
	Q_PROPERTY(bool background_sync      READ backgroundSync     WRITE setBackgroundSync     NOTIFY backgroundSyncChanged)
public:
	CloudStorageSettings(QObject *parent);
	QString password() const;
	QString newPassword() const;
	QString email() const;
	QString emailEncoded() const;
	QString userId() const;
	QString baseUrl() const;
	QString gitUrl() const;
	bool savePasswordLocal() const;
	short verificationStatus() const;
	bool backgroundSync() const;
	bool gitLocalOnly() const;
	short saveUserIdLocal() const;

public slots:
	void setPassword(const QString& value);
	void setNewPassword(const QString& value);
	void setEmail(const QString& value);
	void setEmailEncoded(const QString& value);
	void setUserId(const QString& value);
	void setBaseUrl(const QString& value);
	void setGitUrl(const QString& value);
	void setSavePasswordLocal(bool value);
	void setVerificationStatus(short value);
	void setBackgroundSync(bool value);
	void setGitLocalOnly(bool value);
	void setSaveUserIdLocal(short value);

signals:
	void passwordChanged(const QString& value);
	void newPasswordChanged(const QString& value);
	void emailChanged(const QString& value);
	void emailEncodedChanged(const QString& value);
	void userIdChanged(const QString& value);
	void baseUrlChanged(const QString& value);
	void gitUrlChanged(const QString& value);
	void savePasswordLocalChanged(bool value);
	void verificationStatusChanged(short value);
	void backgroundSyncChanged(bool value);
	void gitLocalOnlyChanged(bool value);
	void saveUserIdLocalChanged(short value);

private:
	const QString group = QStringLiteral("CloudStorage");
};

class DivePlannerSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool last_stop           READ lastStop             WRITE setLastStop             NOTIFY lastStopChanged)
	Q_PROPERTY(bool verbatim_plan       READ verbatimPlan         WRITE setVerbatimPlan         NOTIFY verbatimPlanChanged)
	Q_PROPERTY(bool display_runtime     READ displayRuntime       WRITE setDisplayRuntime       NOTIFY displayRuntimeChanged)
	Q_PROPERTY(bool display_duration    READ displayDuration      WRITE setDisplayDuration      NOTIFY displayDurationChanged)
	Q_PROPERTY(bool display_transitions READ displayTransitions   WRITE setDisplayTransitions   NOTIFY displayTransitionsChanged)
	Q_PROPERTY(bool display_variations  READ displayVariations    WRITE setDisplayVariations    NOTIFY displayVariationsChanged)
	Q_PROPERTY(bool doo2breaks          READ doo2breaks           WRITE setDoo2breaks           NOTIFY doo2breaksChanged)
	Q_PROPERTY(bool drop_stone_mode     READ dropStoneMode        WRITE setDropStoneMode        NOTIFY dropStoneModeChanged)
	Q_PROPERTY(bool safetystop          READ safetyStop           WRITE setSafetyStop           NOTIFY safetyStopChanged)
	Q_PROPERTY(bool switch_at_req_stop  READ switchAtRequiredStop WRITE setSwitchAtRequiredStop NOTIFY switchAtRequiredStopChanged)
	Q_PROPERTY(int ascrate75            READ ascrate75            WRITE setAscrate75            NOTIFY ascrate75Changed)
	Q_PROPERTY(int ascrate50            READ ascrate50            WRITE setAscrate50            NOTIFY ascrate50Changed)
	Q_PROPERTY(int ascratestops         READ ascratestops         WRITE setAscratestops         NOTIFY ascratestopsChanged)
	Q_PROPERTY(int ascratelast6m        READ ascratelast6m        WRITE setAscratelast6m        NOTIFY ascratelast6mChanged)
	Q_PROPERTY(int descrate             READ descrate             WRITE setDescrate             NOTIFY descrateChanged)
	Q_PROPERTY(int sacfactor            READ sacfactor            WRITE setSacFactor            NOTIFY sacFactorChanged)
	Q_PROPERTY(int problemsolvingtime   READ problemsolvingtime   WRITE setProblemSolvingTime   NOTIFY problemSolvingTimeChanged)
	Q_PROPERTY(int bottompo2            READ bottompo2            WRITE setBottompo2            NOTIFY bottompo2Changed)
	Q_PROPERTY(int decopo2              READ decopo2              WRITE setDecopo2              NOTIFY decopo2Changed)
	Q_PROPERTY(int bestmixend           READ bestmixend           WRITE setBestmixend           NOTIFY bestmixendChanged)
	Q_PROPERTY(int reserve_gas          READ reserveGas           WRITE setReserveGas           NOTIFY reserveGasChanged)
	Q_PROPERTY(int min_switch_duration  READ minSwitchDuration    WRITE setMinSwitchDuration    NOTIFY minSwitchDurationChanged)
	Q_PROPERTY(int bottomsac            READ bottomSac            WRITE setBottomSac            NOTIFY bottomSacChanged)
	Q_PROPERTY(int decosac              READ decoSac              WRITE setDecoSac              NOTIFY decoSacChanged)
	Q_PROPERTY(deco_mode decoMode       READ decoMode             WRITE setDecoMode             NOTIFY decoModeChanged)

public:
	DivePlannerSettings(QObject *parent = 0);
	bool lastStop() const;
	bool verbatimPlan() const;
	bool displayRuntime() const;
	bool displayDuration() const;
	bool displayTransitions() const;
	bool displayVariations() const;
	bool doo2breaks() const;
	bool dropStoneMode() const;
	bool safetyStop() const;
	bool switchAtRequiredStop() const;
	int ascrate75() const;
	int ascrate50() const;
	int ascratestops() const;
	int ascratelast6m() const;
	int descrate() const;
	int sacfactor() const;
	int problemsolvingtime() const;
	int bottompo2() const;
	int decopo2() const;
	int bestmixend() const;
	int reserveGas() const;
	int minSwitchDuration() const;
	int bottomSac() const;
	int decoSac() const;
	deco_mode decoMode() const;

public slots:
	void setLastStop(bool value);
	void setVerbatimPlan(bool value);
	void setDisplayRuntime(bool value);
	void setDisplayDuration(bool value);
	void setDisplayTransitions(bool value);
	void setDisplayVariations(bool value);
	void setDoo2breaks(bool value);
	void setDropStoneMode(bool value);
	void setSafetyStop(bool value);
	void setSwitchAtRequiredStop(bool value);
	void setAscrate75(int value);
	void setAscrate50(int value);
	void setAscratestops(int value);
	void setAscratelast6m(int value);
	void setDescrate(int value);
	void setSacFactor(int value);
	void setProblemSolvingTime(int value);
	void setBottompo2(int value);
	void setDecopo2(int value);
	void setBestmixend(int value);
	void setReserveGas(int value);
	void setMinSwitchDuration(int value);
	void setBottomSac(int value);
	void setDecoSac(int value);
	void setDecoMode(deco_mode value);

signals:
	void lastStopChanged(bool value);
	void verbatimPlanChanged(bool value);
	void displayRuntimeChanged(bool value);
	void displayDurationChanged(bool value);
	void displayTransitionsChanged(bool value);
	void displayVariationsChanged(bool value);
	void doo2breaksChanged(bool value);
	void dropStoneModeChanged(bool value);
	void safetyStopChanged(bool value);
	void switchAtRequiredStopChanged(bool value);
	void ascrate75Changed(int value);
	void ascrate50Changed(int value);
	void ascratestopsChanged(int value);
	void ascratelast6mChanged(int value);
	void descrateChanged(int value);
	void sacFactorChanged(int value);
	void problemSolvingTimeChanged(int value);
	void bottompo2Changed(int value);
	void decopo2Changed(int value);
	void bestmixendChanged(int value);
	void reserveGasChanged(int value);
	void minSwitchDurationChanged(int value);
	void bottomSacChanged(int value);
	void decoSacChanged(int value);
	void decoModeChanged(deco_mode value);

private:
	const QString group = QStringLiteral("Planner");
};

class UnitsSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(int length           READ length                 WRITE setLength                 NOTIFY lengthChanged)
	Q_PROPERTY(int pressure       READ pressure               WRITE setPressure               NOTIFY pressureChanged)
	Q_PROPERTY(int volume           READ volume                 WRITE setVolume                 NOTIFY volumeChanged)
	Q_PROPERTY(int temperature READ temperature            WRITE setTemperature            NOTIFY temperatureChanged)
	Q_PROPERTY(int weight           READ weight                 WRITE setWeight                 NOTIFY weightChanged)
	Q_PROPERTY(QString unit_system            READ unitSystem             WRITE setUnitSystem             NOTIFY unitSystemChanged)
	Q_PROPERTY(bool coordinates_traditional   READ coordinatesTraditional WRITE setCoordinatesTraditional NOTIFY coordinatesTraditionalChanged)
	Q_PROPERTY(int vertical_speed_time READ verticalSpeedTime    WRITE setVerticalSpeedTime    NOTIFY verticalSpeedTimeChanged)
	Q_PROPERTY(int duration_units          READ durationUnits        WRITE setDurationUnits         NOTIFY durationUnitChanged)
	Q_PROPERTY(int show_units_table          READ showUnitsTable        WRITE setShowUnitsTable         NOTIFY showUnitsTableChanged)

public:
	UnitsSettings(QObject *parent = 0);
	int length() const;
	int pressure() const;
	int volume() const;
	int temperature() const;
	int weight() const;
	int verticalSpeedTime() const;
	int durationUnits() const;
	int showUnitsTable() const;
	QString unitSystem() const;
	bool coordinatesTraditional() const;

public slots:
	void setLength(int value);
	void setPressure(int value);
	void setVolume(int value);
	void setTemperature(int value);
	void setWeight(int value);
	void setVerticalSpeedTime(int value);
	void setDurationUnits(int value);
	void setShowUnitsTable(int value);
	void setUnitSystem(const QString& value);
	void setCoordinatesTraditional(bool value);

signals:
	void lengthChanged(int value);
	void pressureChanged(int value);
	void volumeChanged(int value);
	void temperatureChanged(int value);
	void weightChanged(int value);
	void verticalSpeedTimeChanged(int value);
	void unitSystemChanged(const QString& value);
	void coordinatesTraditionalChanged(bool value);
	void durationUnitChanged(int value);
	void showUnitsTableChanged(int value);
private:
	const QString group = QStringLiteral("Units");
};

class GeneralSettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString default_filename      READ defaultFilename       WRITE setDefaultFilename       NOTIFY defaultFilenameChanged)
	Q_PROPERTY(QString default_cylinder      READ defaultCylinder       WRITE setDefaultCylinder       NOTIFY defaultCylinderChanged)
	Q_PROPERTY(short default_file_behavior   READ defaultFileBehavior   WRITE setDefaultFileBehavior   NOTIFY defaultFileBehaviorChanged)
	Q_PROPERTY(bool use_default_file         READ useDefaultFile        WRITE setUseDefaultFile        NOTIFY useDefaultFileChanged)
	Q_PROPERTY(int defaultsetpoint           READ defaultSetPoint       WRITE setDefaultSetPoint       NOTIFY defaultSetPointChanged)
	Q_PROPERTY(int o2consumption             READ o2Consumption         WRITE setO2Consumption         NOTIFY o2ConsumptionChanged)
	Q_PROPERTY(int pscr_ratio                READ pscrRatio             WRITE setPscrRatio             NOTIFY pscrRatioChanged)

public:
	GeneralSettingsObjectWrapper(QObject *parent);
	QString defaultFilename() const;
	QString defaultCylinder() const;
	short defaultFileBehavior() const;
	bool useDefaultFile() const;
	int defaultSetPoint() const;
	int o2Consumption() const;
	int pscrRatio() const;

public slots:
	void setDefaultFilename       (const QString& value);
	void setDefaultCylinder       (const QString& value);
	void setDefaultFileBehavior   (short value);
	void setUseDefaultFile        (bool value);
	void setDefaultSetPoint       (int value);
	void setO2Consumption         (int value);
	void setPscrRatio             (int value);

signals:
	void defaultFilenameChanged(const QString& value);
	void defaultCylinderChanged(const QString& value);
	void defaultFileBehaviorChanged(short value);
	void useDefaultFileChanged(bool value);
	void defaultSetPointChanged(int value);
	void o2ConsumptionChanged(int value);
	void pscrRatioChanged(int value);
private:
	const QString group = QStringLiteral("GeneralSettings");
};

class DisplaySettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString divelist_font     READ divelistFont       WRITE setDivelistFont       NOTIFY divelistFontChanged)
	Q_PROPERTY(double font_size          READ fontSize           WRITE setFontSize           NOTIFY fontSizeChanged)
	Q_PROPERTY(short display_invalid_dives  READ displayInvalidDives     WRITE setDisplayInvalidDives       NOTIFY displayInvalidDivesChanged)
public:
	DisplaySettingsObjectWrapper(QObject *parent);
	QString divelistFont() const;
	double fontSize() const;
	short displayInvalidDives() const;
public slots:
	void setDivelistFont(const QString& value);
	void setFontSize(double value);
	void setDisplayInvalidDives(short value);
signals:
	void divelistFontChanged(const QString& value);
	void fontSizeChanged(double value);
	void displayInvalidDivesChanged(short value);
private:
	const QString group = QStringLiteral("Display");
};

class LanguageSettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString language          READ language           WRITE setLanguage           NOTIFY languageChanged)
	Q_PROPERTY(QString time_format       READ timeFormat         WRITE setTimeFormat         NOTIFY timeFormatChanged)
	Q_PROPERTY(QString date_format       READ dateFormat         WRITE setDateFormat         NOTIFY dateFormatChanged)
	Q_PROPERTY(QString date_format_short READ dateFormatShort    WRITE setDateFormatShort    NOTIFY dateFormatShortChanged)
	Q_PROPERTY(QString lang_locale       READ langLocale         WRITE setLangLocale         NOTIFY langLocaleChanged)
	Q_PROPERTY(bool time_format_override READ timeFormatOverride WRITE setTimeFormatOverride NOTIFY timeFormatOverrideChanged)
	Q_PROPERTY(bool date_format_override READ dateFormatOverride WRITE setDateFormatOverride NOTIFY dateFormatOverrideChanged)
	Q_PROPERTY(bool use_system_language  READ useSystemLanguage  WRITE setUseSystemLanguage  NOTIFY useSystemLanguageChanged)

public:
	LanguageSettingsObjectWrapper(QObject *parent);
	QString language() const;
	QString langLocale() const;
	QString timeFormat() const;
	QString dateFormat() const;
	QString dateFormatShort() const;
	bool timeFormatOverride() const;
	bool dateFormatOverride() const;
	bool useSystemLanguage() const;

public slots:
	void  setLangLocale         (const QString& value);
	void  setLanguage           (const QString& value);
	void  setTimeFormat         (const QString& value);
	void  setDateFormat         (const QString& value);
	void  setDateFormatShort    (const QString& value);
	void  setTimeFormatOverride (bool value);
	void  setDateFormatOverride (bool value);
	void  setUseSystemLanguage  (bool value);
signals:
	void languageChanged(const QString& value);
	void langLocaleChanged(const QString& value);
	void timeFormatChanged(const QString& value);
	void dateFormatChanged(const QString& value);
	void dateFormatShortChanged(const QString& value);
	void timeFormatOverrideChanged(bool value);
	void dateFormatOverrideChanged(bool value);
	void useSystemLanguageChanged(bool value);

private:
	const QString group = QStringLiteral("Language");
};

class AnimationsSettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(int animation_speed       READ animationSpeed     WRITE setAnimationSpeed       NOTIFY animationSpeedChanged)
public:
	AnimationsSettingsObjectWrapper(QObject *parent);
	int animationSpeed() const;

public slots:
	void setAnimationSpeed(int value);

signals:
	void animationSpeedChanged(int value);

private:
	const QString group = QStringLiteral("Animations");
};

class LocationServiceSettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(int time_threshold            READ timeThreshold         WRITE setTimeThreshold         NOTIFY timeThresholdChanged)
	Q_PROPERTY(int distance_threshold        READ distanceThreshold     WRITE setDistanceThreshold     NOTIFY distanceThresholdChanged)
public:
	LocationServiceSettingsObjectWrapper(QObject *parent);
	int timeThreshold() const;
	int distanceThreshold() const;
public slots:
	void setTimeThreshold(int value);
	void setDistanceThreshold(int value);
signals:
	void timeThresholdChanged(int value);
	void distanceThresholdChanged(int value);
private:
	const QString group = QStringLiteral("LocationService");
};

class SettingsObjectWrapper : public QObject {
	Q_OBJECT

	Q_PROPERTY(TechnicalDetailsSettings*   techical_details MEMBER techDetails CONSTANT)
	Q_PROPERTY(PartialPressureGasSettings* pp_gas           MEMBER pp_gas CONSTANT)
	Q_PROPERTY(FacebookSettings*           facebook         MEMBER facebook CONSTANT)
	Q_PROPERTY(GeocodingPreferences*       geocoding        MEMBER geocoding CONSTANT)
	Q_PROPERTY(ProxySettings*              proxy            MEMBER proxy CONSTANT)
	Q_PROPERTY(CloudStorageSettings*       cloud_storage    MEMBER cloud_storage CONSTANT)
	Q_PROPERTY(DivePlannerSettings*        planner          MEMBER planner_settings CONSTANT)
	Q_PROPERTY(UnitsSettings*              units            MEMBER unit_settings CONSTANT)

	Q_PROPERTY(GeneralSettingsObjectWrapper*         general   MEMBER general_settings CONSTANT)
	Q_PROPERTY(DisplaySettingsObjectWrapper*         display   MEMBER display_settings CONSTANT)
	Q_PROPERTY(LanguageSettingsObjectWrapper*        language  MEMBER language_settings CONSTANT)
	Q_PROPERTY(AnimationsSettingsObjectWrapper*      animation MEMBER animation_settings CONSTANT)
	Q_PROPERTY(LocationServiceSettingsObjectWrapper* Location  MEMBER location_settings CONSTANT)

	Q_PROPERTY(UpdateManagerSettings* update MEMBER update_manager_settings CONSTANT)
	Q_PROPERTY(DiveComputerSettings* dive_computer MEMBER dive_computer_settings CONSTANT)
public:
	static SettingsObjectWrapper *instance();

	TechnicalDetailsSettings *techDetails;
	PartialPressureGasSettings *pp_gas;
	FacebookSettings *facebook;
	GeocodingPreferences *geocoding;
	ProxySettings *proxy;
	CloudStorageSettings *cloud_storage;
	DivePlannerSettings *planner_settings;
	UnitsSettings *unit_settings;
	GeneralSettingsObjectWrapper *general_settings;
	DisplaySettingsObjectWrapper *display_settings;
	LanguageSettingsObjectWrapper *language_settings;
	AnimationsSettingsObjectWrapper *animation_settings;
	LocationServiceSettingsObjectWrapper *location_settings;
	UpdateManagerSettings *update_manager_settings;
	DiveComputerSettings *dive_computer_settings;

	void sync();
	void load();
private:
	SettingsObjectWrapper(QObject *parent = NULL);
};

#endif
