#ifndef SETTINGSOBJECTWRAPPER_H
#define SETTINGSOBJECTWRAPPER_H

#include <QObject>

#include "../pref.h"
#include "../prefs-macros.h"

/* Wrapper class for the Settings. This will allow
 * seamlessy integration of the settings with the QML
 * and QWidget frontends. This class will be huge, since
 * I need tons of properties, one for each option. */

/* Control the state of the Partial Pressure Graphs preferences */
class PartialPressureGasSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(short show_po2       READ showPo2      WRITE setShowPo2      NOTIFY showPo2Changed)
	Q_PROPERTY(short show_pn2       READ showPn2      WRITE setShowPn2      NOTIFY showPn2Changed)
	Q_PROPERTY(short show_phe       READ showPhe      WRITE setShowPhe      NOTIFY showPheChanged)
	Q_PROPERTY(double po2_threshold READ po2Threshold WRITE setPo2Threshold NOTIFY po2ThresholdChanged)
	Q_PROPERTY(double pn2_threshold READ pn2Threshold WRITE setPn2Threshold NOTIFY pn2ThresholdChanged)
	Q_PROPERTY(double phe_threshold READ pheThreshold WRITE setPheThreshold NOTIFY pheThresholdChanged)

public:
	PartialPressureGasSettings(QObject *parent);
	short showPo2() const;
	short showPn2() const;
	short showPhe() const;
	double po2Threshold() const;
	double pn2Threshold() const;
	double pheThreshold() const;

public slots:
	void setShowPo2(short value);
	void setShowPn2(short value);
	void setShowPhe(short value);
	void setPo2Threshold(double value);
	void setPn2Threshold(double value);
	void setPheThreshold(double value);

signals:
	void showPo2Changed(short value);
	void showPn2Changed(short value);
	void showPheChanged(short value);
	void po2ThresholdChanged(double value);
	void pn2ThresholdChanged(double value);
	void pheThresholdChanged(double value);
private:
	QString group;
};

class TechnicalDetailsSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(double modpO2          READ modp02          WRITE setModp02          NOTIFY modpO2Changed)
	Q_PROPERTY(short ead              READ ead             WRITE setEad             NOTIFY eadChanged)
	Q_PROPERTY(short mod              READ mod                WRITE setMod                NOTIFY modChanged);
	Q_PROPERTY(short dcceiling        READ dcceiling       WRITE setDCceiling       NOTIFY dcceilingChanged)
	Q_PROPERTY(short redceiling       READ redceiling      WRITE setRedceiling      NOTIFY redceilingChanged)
	Q_PROPERTY(short calcceiling      READ calcceiling     WRITE setCalcceiling     NOTIFY calcceilingChanged)
	Q_PROPERTY(short calcceiling3m    READ calcceiling3m   WRITE setCalcceiling3m   NOTIFY calcceiling3mChanged)
	Q_PROPERTY(short calcalltissues   READ calcalltissues  WRITE setCalcalltissues  NOTIFY calcalltissuesChanged)
	Q_PROPERTY(short calcndltts       READ calcndltts      WRITE setCalcndltts      NOTIFY calcndlttsChanged)
	Q_PROPERTY(short gflow            READ gflow           WRITE setGflow           NOTIFY gflowChanged)
	Q_PROPERTY(short gfhigh           READ gfhigh          WRITE setGfhigh          NOTIFY gfhighChanged)
	Q_PROPERTY(short hrgraph          READ hrgraph         WRITE setHRgraph         NOTIFY hrgraphChanged)
	Q_PROPERTY(short tankbar          READ tankBar         WRITE setTankBar         NOTIFY tankBarChanged)
	Q_PROPERTY(short percentagegraph  READ percentageGraph WRITE setPercentageGraph NOTIFY percentageGraphChanged)
	Q_PROPERTY(short rulergraph       READ rulerGraph      WRITE setRulerGraph      NOTIFY rulerGraphChanged)
	Q_PROPERTY(bool show_ccr_setpoint READ showCCRSetpoint WRITE setShowCCRSetpoint NOTIFY showCCRSetpointChanged)
	Q_PROPERTY(bool show_ccr_sensors  READ showCCRSensors  WRITE setShowCCRSensors  NOTIFY showCCRSensorsChanged)
	Q_PROPERTY(short zoomed_plot      READ zoomedPlot      WRITE setZoomedPlot      NOTIFY zoomedPlotChanged)
	Q_PROPERTY(short show_sac             READ showSac            WRITE setShowSac            NOTIFY showSacChanged)
	Q_PROPERTY(bool gf_low_at_maxdepth    READ gfLowAtMaxDepth    WRITE setGfLowAtMaxDepth    NOTIFY gfLowAtMaxDepthChanged)
	Q_PROPERTY(short display_unused_tanks READ displayUnusedTanks WRITE setDisplayUnusedTanks NOTIFY displayUnusedTanksChanged)
	Q_PROPERTY(short show_average_depth   READ showAverageDepth   WRITE setShowAverageDepth   NOTIFY showAverageDepthChanged)
	Q_PROPERTY(bool show_pictures_in_profile READ showPicturesInProfile WRITE setShowPicturesInProfile NOTIFY showPicturesInProfileChanged)
public:
	TechnicalDetailsSettings(QObject *parent);

	double modp02() const;
	short ead() const;
	short mod() const;
	short dcceiling() const;
	short redceiling() const;
	short calcceiling() const;
	short calcceiling3m() const;
	short calcalltissues() const;
	short calcndltts() const;
	short gflow() const;
	short gfhigh() const;
	short hrgraph() const;
	short tankBar() const;
	short percentageGraph() const;
	short rulerGraph() const;
	bool showCCRSetpoint() const;
	bool showCCRSensors() const;
	short zoomedPlot() const;
	short showSac() const;
	bool gfLowAtMaxDepth() const;
	short displayUnusedTanks() const;
	short showAverageDepth() const;
	bool showPicturesInProfile() const;

public slots:
	void setMod(short value);
	void setModp02(double value);
	void setEad(short value);
	void setDCceiling(short value);
	void setRedceiling(short value);
	void setCalcceiling(short value);
	void setCalcceiling3m(short value);
	void setCalcalltissues(short value);
	void setCalcndltts(short value);
	void setGflow(short value);
	void setGfhigh(short value);
	void setHRgraph(short value);
	void setTankBar(short value);
	void setPercentageGraph(short value);
	void setRulerGraph(short value);
	void setShowCCRSetpoint(bool value);
	void setShowCCRSensors(bool value);
	void setZoomedPlot(short value);
	void setShowSac(short value);
	void setGfLowAtMaxDepth(bool value);
	void setDisplayUnusedTanks(short value);
	void setShowAverageDepth(short value);
	void setShowPicturesInProfile(bool value);

signals:
	void modpO2Changed(double value);
	void eadChanged(short value);
	void modChanged(short value);
	void dcceilingChanged(short value);
	void redceilingChanged(short value);
	void calcceilingChanged(short value);
	void calcceiling3mChanged(short value);
	void calcalltissuesChanged(short value);
	void calcndlttsChanged(short value);
	void gflowChanged(short value);
	void gfhighChanged(short value);
	void hrgraphChanged(short value);
	void tankBarChanged(short value);
	void percentageGraphChanged(short value);
	void rulerGraphChanged(short value);
	void showCCRSetpointChanged(bool value);
	void showCCRSensorsChanged(bool value);
	void zoomedPlotChanged(short value);
	void showSacChanged(short value);
	void gfLowAtMaxDepthChanged(bool value);
	void displayUnusedTanksChanged(short value);
	void showAverageDepthChanged(short value);
	void showPicturesInProfileChanged(bool value);
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
	Q_PROPERTY(bool enable_geocoding       READ enableGeocoding        WRITE setEnableGeocoding        NOTIFY enableGeocodingChanged)
	Q_PROPERTY(bool parse_dive_without_gps READ parseDiveWithoutGps    WRITE setParseDiveWithoutGps    NOTIFY parseDiveWithoutGpsChanged)
	Q_PROPERTY(bool tag_existing_dives     READ tagExistingDives       WRITE setTagExistingDives       NOTIFY tagExistingDivesChanged)
	Q_PROPERTY(taxonomy_category first_category     READ firstTaxonomyCategory  WRITE setFirstTaxonomyCategory  NOTIFY firstTaxonomyCategoryChanged)
	Q_PROPERTY(taxonomy_category second_category    READ secondTaxonomyCategory WRITE setSecondTaxonomyCategory NOTIFY secondTaxonomyCategoryChanged)
	Q_PROPERTY(taxonomy_category third_category     READ thirdTaxonomyCategory  WRITE setThirdTaxonomyCategory  NOTIFY thirdTaxonomyCategoryChanged)
public:
	GeocodingPreferences(QObject *parent);
	bool enableGeocoding() const;
	bool parseDiveWithoutGps() const;
	bool tagExistingDives() const;
	taxonomy_category firstTaxonomyCategory() const;
	taxonomy_category secondTaxonomyCategory() const;
	taxonomy_category thirdTaxonomyCategory() const;

public slots:
	void setEnableGeocoding(bool value);
	void setParseDiveWithoutGps(bool value);
	void setTagExistingDives(bool value);
	void  setFirstTaxonomyCategory(taxonomy_category value);
	void  setSecondTaxonomyCategory(taxonomy_category value);
	void  setThirdTaxonomyCategory(taxonomy_category value);

signals:
	void enableGeocodingChanged(bool value);
	void parseDiveWithoutGpsChanged(bool value);
	void tagExistingDivesChanged(bool value);
	void firstTaxonomyCategoryChanged(taxonomy_category value);
	void secondTaxonomyCategoryChanged(taxonomy_category value);
	void thirdTaxonomyCategoryChanged(taxonomy_category value);
private:
	QString group;
};

class ProxySettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(int type     READ type WRITE setType NOTIFY typeChanged)
	Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
	Q_PROPERTY(int port     READ port WRITE setPort NOTIFY portChanged)
	Q_PROPERTY(short auth   READ auth WRITE setAuth NOTIFY authChanged)
	Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)
	Q_PROPERTY(QString pass READ pass WRITE setPass NOTIFY passChanged)

public:
	ProxySettings(QObject *parent);
	int type() const;
	QString host() const;
	int port() const;
	short auth() const;
	QString user() const;
	QString pass() const;

public slots:
	void setType(int value);
	void setHost(const QString& value);
	void setPort(int value);
	void setAuth(short value);
	void setUser(const QString& value);
	void setPass(const QString& value);

signals:
	void typeChanged(int value);
	void hostChanged(const QString& value);
	void portChanged(int value);
	void authChanged(short value);
	void userChanged(const QString& value);
	void passChanged(const QString& value);
private:
	QString group;
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
private:
	QString group;
};

class DivePlannerSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool last_stop           READ lastStop             WRITE setLastStop             NOTIFY lastStopChanged)
	Q_PROPERTY(bool verbatim_plan       READ verbatimPlan         WRITE setVerbatimPlan         NOTIFY verbatimPlanChanged)
	Q_PROPERTY(bool display_runtime     READ displayRuntime       WRITE setDisplayRuntime       NOTIFY displayRuntimeChanged)
	Q_PROPERTY(bool display_duration    READ displayDuration      WRITE setDisplayDuration      NOTIFY displayDurationChanged)
	Q_PROPERTY(bool display_transitions READ displayTransitions   WRITE setDisplayTransitions   NOTIFY displayTransitionsChanged)
	Q_PROPERTY(bool doo2breaks          READ doo2breaks           WRITE setDoo2breaks           NOTIFY doo2breaksChanged)
	Q_PROPERTY(bool drop_stone_mode     READ dropStoneMode        WRITE setDropStoneMode        NOTIFY dropStoneModeChanged)
	Q_PROPERTY(bool safetystop          READ safetyStop           WRITE setSafetyStop           NOTIFY safetyStopChanged)
	Q_PROPERTY(bool switch_at_req_stop  READ switchAtRequiredStop WRITE setSwitchAtRequiredStop NOTIFY switchAtRequiredStopChanged)
	Q_PROPERTY(int ascrate75            READ ascrate75            WRITE setAscrate75            NOTIFY ascrate75Changed)
	Q_PROPERTY(int ascrate50            READ ascrate50            WRITE setAscrate50            NOTIFY ascrate50Changed)
	Q_PROPERTY(int ascratestops         READ ascratestops         WRITE setAscratestops         NOTIFY ascratestopsChanged)
	Q_PROPERTY(int ascratelast6m        READ ascratelast6m        WRITE setAscratelast6m        NOTIFY ascratelast6mChanged)
	Q_PROPERTY(int descrate             READ descrate             WRITE setDescrate             NOTIFY descrateChanged)
	Q_PROPERTY(int bottompo2            READ bottompo2            WRITE setBottompo2            NOTIFY bottompo2Changed)
	Q_PROPERTY(int decopo2              READ decopo2              WRITE setDecopo2              NOTIFY decopo2Changed)
	Q_PROPERTY(int reserve_gas          READ reserveGas           WRITE setReserveGas           NOTIFY reserveGasChanged)
	Q_PROPERTY(int min_switch_duration  READ minSwitchDuration    WRITE setMinSwitchDuration    NOTIFY minSwitchDurationChanged)
	Q_PROPERTY(int bottomsac            READ bottomSac            WRITE setBottomSac            NOTIFY bottomSacChanged)
	Q_PROPERTY(int decosac              READ decoSac              WRITE setSecoSac              NOTIFY decoSacChanged)
	Q_PROPERTY(short conservatism_level READ conservatismLevel    WRITE setConservatismLevel    NOTIFY conservatismLevelChanged)
	Q_PROPERTY(deco_mode decoMode       READ decoMode             WRITE setDecoMode             NOTIFY decoModeChanged)

public:
	DivePlannerSettings(QObject *parent = 0);
	bool lastStop() const;
	bool verbatimPlan() const;
	bool displayRuntime() const;
	bool displayDuration() const;
	bool displayTransitions() const;
	bool doo2breaks() const;
	bool dropStoneMode() const;
	bool safetyStop() const;
	bool switchAtRequiredStop() const;
	int ascrate75() const;
	int ascrate50() const;
	int ascratestops() const;
	int ascratelast6m() const;
	int descrate() const;
	int bottompo2() const;
	int decopo2() const;
	int reserveGas() const;
	int minSwitchDuration() const;
	int bottomSac() const;
	int decoSac() const;
	short conservatismLevel() const;
	deco_mode decoMode() const;

public slots:
	void setLastStop(bool value);
	void setVerbatimPlan(bool value);
	void setDisplayRuntime(bool value);
	void setDisplayDuration(bool value);
	void setDisplayTransitions(bool value);
	void setDoo2breaks(bool value);
	void setDropStoneMode(bool value);
	void setSafetyStop(bool value);
	void setSwitchAtRequiredStop(bool value);
	void setAscrate75(int value);
	void setAscrate50(int value);
	void setAscratestops(int value);
	void setAscratelast6m(int value);
	void setDescrate(int value);
	void setBottompo2(int value);
	void setDecopo2(int value);
	void setReserveGas(int value);
	void setMinSwitchDuration(int value);
	void setBottomSac(int value);
	void setSecoSac(int value);
	void setConservatismLevel(int value);
	void setDecoMode(deco_mode value);

signals:
	void lastStopChanged(bool value);
	void verbatimPlanChanged(bool value);
	void displayRuntimeChanged(bool value);
	void displayDurationChanged(bool value);
	void displayTransitionsChanged(bool value);
	void doo2breaksChanged(bool value);
	void dropStoneModeChanged(bool value);
	void safetyStopChanged(bool value);
	void switchAtRequiredStopChanged(bool value);
	void ascrate75Changed(int value);
	void ascrate50Changed(int value);
	void ascratestopsChanged(int value);
	void ascratelast6mChanged(int value);
	void descrateChanged(int value);
	void bottompo2Changed(int value);
	void decopo2Changed(int value);
	void reserveGasChanged(int value);
	void minSwitchDurationChanged(int value);
	void bottomSacChanged(int value);
	void decoSacChanged(int value);
	void conservatismLevelChanged(int value);
	void decoModeChanged(deco_mode value);

private:
	QString group;
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

public:
	UnitsSettings(QObject *parent = 0);
	int length() const;
	int pressure() const;
	int volume() const;
	int temperature() const;
	int weight() const;
	int verticalSpeedTime() const;
	QString unitSystem() const;
	bool coordinatesTraditional() const;

public slots:
	void setLength(int value);
	void setPressure(int value);
	void setVolume(int value);
	void setTemperature(int value);
	void setWeight(int value);
	void setVerticalSpeedTime(int value);
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
private:
	QString group;
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
	QString group;
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
	QString group;
};

class LanguageSettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString language          READ language           WRITE setLanguage           NOTIFY languageChanged)
	Q_PROPERTY(QString time_format       READ timeFormat         WRITE setTimeFormat         NOTIFY timeFormatChanged)
	Q_PROPERTY(QString date_format       READ dateFormat         WRITE setDateFormat         NOTIFY dateFormatChanged)
	Q_PROPERTY(QString date_format_short READ dateFormatShort    WRITE setDateFormatShort    NOTIFY dateFormatShortChanged)
	Q_PROPERTY(bool time_format_override READ timeFormatOverride WRITE setTimeFormatOverride NOTIFY timeFormatOverrideChanged)
	Q_PROPERTY(bool date_format_override READ dateFormatOverride WRITE setDateFormatOverride NOTIFY dateFormatOverrideChanged)
	Q_PROPERTY(bool use_system_language  READ useSystemLanguage  WRITE setUseSystemLanguage  NOTIFY useSystemLanguageChanged)

public:
	LanguageSettingsObjectWrapper(QObject *parent);
	QString language() const;
	QString timeFormat() const;
	QString dateFormat() const;
	QString dateFormatShort() const;
	bool timeFormatOverride() const;
	bool dateFormatOverride() const;
	bool useSystemLanguage() const;

public slots:
	void  setLanguage           (const QString& value);
	void  setTimeFormat         (const QString& value);
	void  setDateFormat         (const QString& value);
	void  setDateFormatShort    (const QString& value);
	void  setTimeFormatOverride (bool value);
	void  setDateFormatOverride (bool value);
	void  setUseSystemLanguage  (bool value);
signals:
	void languageChanged(const QString& value);
	void timeFormatChanged(const QString& value);
	void dateFormatChanged(const QString& value);
	void dateFormatShortChanged(const QString& value);
	void timeFormatOverrideChanged(bool value);
	void dateFormatOverrideChanged(bool value);
	void useSystemLanguageChanged(bool value);

private:
	QString group;
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
	QString group;
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
	QString group;
};

class SettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(short save_userid_local  READ saveUserIdLocal WRITE setSaveUserIdLocal NOTIFY saveUserIdLocalChanged)

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
public:
	static SettingsObjectWrapper *instance();
	short saveUserIdLocal() const;

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

public slots:
	void setSaveUserIdLocal(short value);
private:
	SettingsObjectWrapper(QObject *parent = NULL);
signals:
	void saveUserIdLocalChanged(short value);
};

#endif
