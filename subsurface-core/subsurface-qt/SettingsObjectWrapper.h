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

public:
	PartialPressureGasSettings(QObject *parent);
	short showPo2() const;
	short showPn2() const;
	short showPhe() const;
	double po2Threshold() const;
	double pn2Threshold() const;
	double pheThreshold() const;
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

public slots:
	void setShowPo2(short value);
	void setShowPn2(short value);
	void setShowPhe(short value);
	void setPo2Threshold(double value);
	void setPn2Threshold(double value);
	void setPheThreshold(double value);
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

signals:
	void showPo2Changed(short value);
	void showPn2Changed(short value);
	void showPheChanged(short value);
	void po2ThresholdChanged(double value);
	void pn2ThresholdChanged(double value);
	void pheThresholdChanged(double value);
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
};

/* Control the state of the Facebook preferences */
class FacebookSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(access_token READ WRITE setAccessToken NOTIFY accessTokenChanged);
	Q_PROPERTY(user_id READ WRITE setUserId NOTIFY userIdChanged)
	Q_PROPERTY(album_id READ WRITE setAlbumId NOTIFY albumIdChanged)
public:
	FacebookSettings(QObject *parent);
};

/* Control the state of the Geocoding preferences */
class GeocodingPreferences : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool enable_geocoding       READ enableGeocoding        WRITE setEnableGeocoding        NOTIFY enableGeocodingChanged)
	Q_PROPERTY(bool parse_dive_without_gps READ parseDiveWithoutGps    WRITE setParseDiveWithoutGps    NOTIFY parseDiveWithoutGpsChanged)
	Q_PROPERTY(bool tag_existing_dives     READ tagExistingDives       WRITE setTagExistingDives       NOTIFY tagExistingDivesChanged)
	Q_PROPERTY(taxonomy_category first     READ firstTaxonomyCategory  WRITE setFirstTaxonomyCategory  NOTIFY firstTaxonomyCategoryChanged)
	Q_PROPERTY(taxonomy_category second    READ secondTaxonomyCategory WRITE setSecondTaxonomyCategory NOTIFY secondTaxonomyCategoryChanged)
	Q_PROPERTY(taxonomy_category third     READ thirdTaxonomyCategory  WRITE setThirdTaxonomyCategory  NOTIFY thirdTaxonomyCategoryChanged)
public:
	GeocodingPreferences(QObject *parent);
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
};

class CloudStorageSettings : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString password          READ password           WRITE setPassword           NOTIFY passwordChanged)
	Q_PROPERTY(QString newpassword       READ newPassword        WRITE setNewPassword        NOTIFY newPasswordChanged)
	Q_PROPERTY(QString email             READ email              WRITE setEmail              NOTIFY emailChanged)
	Q_PROPERTY(QString email_encoded     READ emailEncoded       WRITE setEmailEncoded       NOTIFY emailEncodedChanged)
	Q_PROPERTY(bool save_password_local  READ savePasswordLocal  WRITE setSavePasswordLocal  NOTIFY savePasswordLocalChanged)
	Q_PROPERTY(short verification_status READ verificationStatus WRITE setVerificationStatus NOTIFY verificationStatusChanged)
	Q_PROPERTY(bool background_sync      READ backgroundSync     WRITE setBackgroundSync     NOTIFY backgroundSyncChanged)
public:
	CloudStorageSettings(QObject *parent);
};

/* Monster class, should be breaken into a few more understandable classes later, wich will be easy to do:
* grab the Q_PROPERTYES and create a wrapper class like the ones above.
*/
class SettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString divelist_font     READ divelistFont       WRITE setDivelistFont       NOTIFY divelistFontChanged)
	Q_PROPERTY(QString default_filename  READ defaultFilename    WRITE setDefaultFilename    NOTIFY defaultFilenameChanged)
	Q_PROPERTY(QString default_cylinder  READ defaultCylinder    WRITE setDefaultCylinder    NOTIFY defaultCylinderChanged)
	Q_PROPERTY(QString cloud_base_url    READ cloudBaseUrl       WRITE setCloudBaseURL       NOTIFY cloudBaseUrlChanged)
	Q_PROPERTY(QString cloud_git_url     READ cloudGitUrl        WRITE setCloudGitUrl        NOTIFY cloudGitUrlChanged)
	Q_PROPERTY(QString time_format       READ timeFormat         WRITE setTimeFormat         NOTIFY timeFormatChanged)
	Q_PROPERTY(QString date_format       READ dateFormat         WRITE setDateFormat         NOTIFY dateFormatChanged)
	Q_PROPERTY(QString date_format_short READ dateFormatShort    WRITE setDateFormatShort    NOTIFY dateFormatShortChanged)
	Q_PROPERTY(bool time_format_override READ timeFormatOverride WRITE setTimeFormatOverride NOTIFY timeFormatOverrideChanged)
	Q_PROPERTY(bool date_format_override READ dateFormatOverride WRITE setDateFormatOverride NOTIFY dateFormatOverrideChanged)
	Q_PROPERTY(double font_size          READ fontSize           WRITE setFontSize           NOTIFY fontSizeChanged)
	Q_PROPERTY(int animation_speed       READ animationSpeed     WRITE setAnimationSpeed       NOTIFY animationSpeedChanged)
	Q_PROPERTY(short display_invalid_dives  READ displayInvalidDives     WRITE setDisplayInvalidDives       NOTIFY displayInvalidDivesChanged)
	Q_PROPERTY(short unit_system            READ unitSystem              WRITE setUnitSystem                NOTIFY uintSystemChanged)
	Q_PROPERTY(bool coordinates_traditional READ coordinatesTraditional  WRITE setCoordinatesTraditional    NOTIFY coordinatesTraditionalChanged)
	Q_PROPERTY(short save_userid_local  READ saveUserIdLocal WRITE setSaveUserIdLocal NOTIFY saveUserIdLocalChanged)
	Q_PROPERTY(QString userid           READ userId          WRITE setUserId          NOTIFY userIdChanged)
	Q_PROPERTY(int ascrate75            READ ascrate75       WRITE setAscrate75       NOTIFY ascrate75Changed)
	Q_PROPERTY(int ascrate50            READ ascrate50       WRITE setAscrate50       NOTIFY ascrate50Changed)
	Q_PROPERTY(int ascratestops         READ ascratestops    WRITE setAscratestops    NOTIFY ascratestopsChanged)
	Q_PROPERTY(int ascratelast6m        READ ascratelast6m   WRITE setAscratelast6m   NOTIFY ascratelast6mChanged)
	Q_PROPERTY(int descrate             READ descrate        WRITE setDescrate        NOTIFY descrateChanged)
	Q_PROPERTY(int bottompo2            READ bottompo2       WRITE setBottompo2       NOTIFY bottompo2Changed)
	Q_PROPERTY(int decopo2              READ decopo2         WRITE setDecopo2         NOTIFY decopo2Changed)
	Q_PROPERTY(bool doo2breaks          READ doo2breaks      WRITE setDoo2breaks      NOTIFY doo2breaksChanged)
	Q_PROPERTY(bool drop_stone_mode     READ dropStoneMode   WRITE setDropStoneMode   NOTIFY dropStoneModeChanged)
	Q_PROPERTY(bool last_stop           READ lastStop        WRITE setLastStop        NOTIFY lastStopChanged)
	Q_PROPERTY(bool verbatim_plan       READ verbatimPlan    WRITE setVerbatimPlan    NOTIFY verbatimPlanChanged)
	Q_PROPERTY(bool display_runtime          READ displayRuntime        WRITE setDisplayRuntime        NOTIFY displayRuntimeChanged)
	Q_PROPERTY(bool display_duration         READ displayDuration       WRITE setDisplayDuration       NOTIFY displayDurationChanged)
	Q_PROPERTY(bool display_transitions      READ displayTransitions    WRITE setDisplayTransitions    NOTIFY displayTransitionsChanged)
	Q_PROPERTY(bool safetystop               READ safetyStop            WRITE setSafetyStop            NOTIFY safetyStopChanged)
	Q_PROPERTY(bool switch_at_req_stop       READ switchAtRequiredStop  WRITE setSwitchAtRequiredStop  NOTIFY switchAtRequiredStopChanged)
	Q_PROPERTY(int reserve_gas               READ reserveGas            WRITE setReserveGas            NOTIFY reserveGasChanged)
	Q_PROPERTY(int min_switch_duration       READ minSwitchDuration     WRITE setMinSwitchDuration     NOTIFY minSwitchDurationChanged)
	Q_PROPERTY(int bottomsac                 READ bottomSac             WRITE setBottomSac             NOTIFY bottomSacChanged)
	Q_PROPERTY(int decosac                   READ decoSac               WRITE setSecoSac               NOTIFY decoSacChanged)
	Q_PROPERTY(int o2consumption             READ o2Consumption         WRITE setO2Consumption         NOTIFY o2ConsumptionChanged)
	Q_PROPERTY(int pscr_ratio                READ pscrRatio             WRITE setPscrRatio             NOTIFY pscrRatioChanged)
	Q_PROPERTY(int defaultsetpoint           READ defaultSetPoint       WRITE setDefaultSetPoint       NOTIFY defaultSetPointChanged)
	Q_PROPERTY(bool show_pictures_in_profile READ showPicturesInProfile WRITE setShowPicturesInProfile NOTIFY showPicturesInProfileChanged)
	Q_PROPERTY(bool use_default_file         READ useDefaultFile        WRITE setUseDefaultFile        NOTIFY useDefaultFileChanged)
	Q_PROPERTY(short default_file_behavior   READ defaultFileBehavior   WRITE setDefaultFileBehavior   NOTIFY defaultFileBehaviorChanged)
	Q_PROPERTY(short conservatism_level      READ conservatism_level    WRITE setConservatismLevel     NOTIFY conservatismLevelChanged)
	Q_PROPERTY(int time_threshold            READ timeThreshold         WRITE setTimeThreshold         NOTIFY timeThresholdChanged)
	Q_PROPERTY(int distance_threshold        READ distanceThreshold     WRITE setDistanceThreshold     NOTIFY distanceThresholdChanged)
	Q_PROPERTY(bool git_local_only           READ gitLocalOnly          WRITE setGitLocalOnly          NOTIFY gitLocalOnlyChanged)

	PartialPressureGasSettings *pp_gas;
	FacebookSettings *facebook;
	GeocodingPreferences *geocoding;
	ProxySettings *proxy;
	// Units
	struct units units;

	// Decompression Mode
	enum deco_mode deco_mode;
};

public:
	SettingsObjectWrapper(QObject *parent = NULL);
};

#endif