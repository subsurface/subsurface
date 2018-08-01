// SPDX-License-Identifier: GPL-2.0
#ifndef SETTINGSOBJECTWRAPPER_H
#define SETTINGSOBJECTWRAPPER_H

#include <QObject>
#include <QDate>

#include "core/pref.h"
#include "core/settings/qPref.h"

/* Wrapper class for the Settings. This will allow
 * seamlessy integration of the settings with the QML
 * and QWidget frontends. This class will be huge, since
 * I need tons of properties, one for each option. */

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
	Q_PROPERTY(bool   show_po2          READ showPo2         WRITE setShowPo2         NOTIFY showPo2Changed)
	Q_PROPERTY(bool   show_pn2          READ showPn2         WRITE setShowPn2         NOTIFY showPn2Changed)
	Q_PROPERTY(bool   show_phe	    READ showPhe         WRITE setShowPhe         NOTIFY showPheChanged)
	Q_PROPERTY(double po2_threshold_min READ po2ThresholdMin WRITE setPo2ThresholdMin NOTIFY po2ThresholdMinChanged)
	Q_PROPERTY(double po2_threshold_max READ po2ThresholdMax WRITE setPo2ThresholdMax NOTIFY po2ThresholdMaxChanged)
	Q_PROPERTY(double pn2_threshold     READ pn2Threshold    WRITE setPn2Threshold    NOTIFY pn2ThresholdChanged)
	Q_PROPERTY(double phe_threshold     READ pheThreshold    WRITE setPheThreshold    NOTIFY pheThresholdChanged)

public:
	PartialPressureGasSettings(QObject *parent);
	bool showPo2() const;
	bool showPn2() const;
	bool showPhe() const;
	double po2ThresholdMin() const;
	double po2ThresholdMax() const;
	double pn2Threshold() const;
	double pheThreshold() const;

public slots:
	void setShowPo2(bool value);
	void setShowPn2(bool value);
	void setShowPhe(bool value);
	void setPo2ThresholdMin(double value);
	void setPo2ThresholdMax(double value);
	void setPn2Threshold(double value);
	void setPheThreshold(double value);

signals:
	void showPo2Changed(bool value);
	void showPn2Changed(bool value);
	void showPheChanged(bool value);
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
	Q_PROPERTY(bool show_scr_ocpo2   READ showSCROCpO2    WRITE setShowSCROCpO2    NOTIFY showSCROCpO2Changed)
	Q_PROPERTY(bool zoomed_plot      READ zoomedPlot      WRITE setZoomedPlot      NOTIFY zoomedPlotChanged)
	Q_PROPERTY(bool show_sac             READ showSac            WRITE setShowSac            NOTIFY showSacChanged)
	Q_PROPERTY(bool display_unused_tanks READ displayUnusedTanks WRITE setDisplayUnusedTanks NOTIFY displayUnusedTanksChanged)
	Q_PROPERTY(bool show_average_depth   READ showAverageDepth   WRITE setShowAverageDepth   NOTIFY showAverageDepthChanged)
	Q_PROPERTY(bool show_icd         READ showIcd         WRITE setShowIcd         NOTIFY showIcdChanged)
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
	bool showSCROCpO2() const;
	bool zoomedPlot() const;
	bool showSac() const;
	bool displayUnusedTanks() const;
	bool showAverageDepth() const;
	bool showIcd() const;
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
	void setShowSCROCpO2(bool value);
	void setZoomedPlot(bool value);
	void setShowSac(bool value);
	void setDisplayUnusedTanks(bool value);
	void setShowAverageDepth(bool value);
	void setShowIcd(bool value);
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
	void showSCROCpO2Changed(bool value);
	void zoomedPlotChanged(bool value);
	void showSacChanged(bool value);
	void displayUnusedTanksChanged(bool value);
	void showAverageDepthChanged(bool value);
	void showIcdChanged(bool value);
	void showPicturesInProfileChanged(bool value);
	void decoModeChanged(deco_mode m);

private:
	const QString group = QStringLiteral("TecDetails");
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

class GeneralSettingsObjectWrapper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString default_filename              READ defaultFilename                WRITE setDefaultFilename                NOTIFY defaultFilenameChanged)
	Q_PROPERTY(QString default_cylinder              READ defaultCylinder                WRITE setDefaultCylinder                NOTIFY defaultCylinderChanged)
	Q_PROPERTY(short default_file_behavior           READ defaultFileBehavior            WRITE setDefaultFileBehavior            NOTIFY defaultFileBehaviorChanged)
	Q_PROPERTY(bool use_default_file                 READ useDefaultFile                 WRITE setUseDefaultFile                 NOTIFY useDefaultFileChanged)
	Q_PROPERTY(int defaultsetpoint                   READ defaultSetPoint                WRITE setDefaultSetPoint                NOTIFY defaultSetPointChanged)
	Q_PROPERTY(int o2consumption                     READ o2Consumption                  WRITE setO2Consumption                  NOTIFY o2ConsumptionChanged)
	Q_PROPERTY(int pscr_ratio                        READ pscrRatio                      WRITE setPscrRatio                      NOTIFY pscrRatioChanged)
	Q_PROPERTY(bool auto_recalculate_thumbnails      READ autoRecalculateThumbnails      WRITE setAutoRecalculateThumbnails      NOTIFY autoRecalculateThumbnailsChanged)
	Q_PROPERTY(bool extract_video_thumbnails         READ extractVideoThumbnails         WRITE setExtractVideoThumbnails         NOTIFY extractVideoThumbnailsChanged)
	Q_PROPERTY(int extract_video_thumbnails_position READ extractVideoThumbnailsPosition WRITE setExtractVideoThumbnailsPosition NOTIFY extractVideoThumbnailsPositionChanged)
	Q_PROPERTY(QString ffmpeg_executable             READ ffmpegExecutable               WRITE setFfmpegExecutable               NOTIFY ffmpegExecutableChanged)

public:
	GeneralSettingsObjectWrapper(QObject *parent);
	QString defaultFilename() const;
	QString defaultCylinder() const;
	short defaultFileBehavior() const;
	bool useDefaultFile() const;
	int defaultSetPoint() const;
	int o2Consumption() const;
	int pscrRatio() const;
	bool autoRecalculateThumbnails() const;
	bool extractVideoThumbnails() const;
	int extractVideoThumbnailsPosition() const;
	QString ffmpegExecutable() const;

public slots:
	void setDefaultFilename           (const QString& value);
	void setDefaultCylinder           (const QString& value);
	void setDefaultFileBehavior       (short value);
	void setUseDefaultFile            (bool value);
	void setDefaultSetPoint           (int value);
	void setO2Consumption             (int value);
	void setPscrRatio                 (int value);
	void setAutoRecalculateThumbnails (bool value);
	void setExtractVideoThumbnails    (bool value);
	void setExtractVideoThumbnailsPosition (int value);
	void setFfmpegExecutable          (const QString &value);

signals:
	void defaultFilenameChanged(const QString& value);
	void defaultCylinderChanged(const QString& value);
	void defaultFileBehaviorChanged(short value);
	void useDefaultFileChanged(bool value);
	void defaultSetPointChanged(int value);
	void o2ConsumptionChanged(int value);
	void pscrRatioChanged(int value);
	void autoRecalculateThumbnailsChanged(int value);
	void extractVideoThumbnailsChanged(bool value);
	void extractVideoThumbnailsPositionChanged(int value);
	void ffmpegExecutableChanged(const QString &value);
private:
	const QString group = QStringLiteral("GeneralSettings");
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
	Q_PROPERTY(qPrefFacebook*           facebook         MEMBER facebook CONSTANT)
	Q_PROPERTY(GeocodingPreferences*       geocoding        MEMBER geocoding CONSTANT)
	Q_PROPERTY(qPrefProxy*              proxy            MEMBER proxy CONSTANT)
	Q_PROPERTY(qPrefCloudStorage*       cloud_storage    MEMBER cloud_storage CONSTANT)
	Q_PROPERTY(qPrefDivePlanner*        planner          MEMBER planner_settings CONSTANT)
	Q_PROPERTY(qPrefUnits*              units            MEMBER unit_settings CONSTANT)
	Q_PROPERTY(GeneralSettingsObjectWrapper*         general   MEMBER general_settings CONSTANT)
	Q_PROPERTY(qPrefDisplay*         display   MEMBER display_settings CONSTANT)
	Q_PROPERTY(LanguageSettingsObjectWrapper*        language  MEMBER language_settings CONSTANT)
	Q_PROPERTY(qPrefAnimations*      animation MEMBER animation_settings CONSTANT)
	Q_PROPERTY(LocationServiceSettingsObjectWrapper* Location  MEMBER location_settings CONSTANT)

	Q_PROPERTY(UpdateManagerSettings* update MEMBER update_manager_settings CONSTANT)
	Q_PROPERTY(qPrefDiveComputer* dive_computer MEMBER dive_computer_settings CONSTANT)
public:
	static SettingsObjectWrapper *instance();

	TechnicalDetailsSettings *techDetails;
	PartialPressureGasSettings *pp_gas;
	qPrefFacebook *facebook;
	GeocodingPreferences *geocoding;
	qPrefProxy *proxy;
	qPrefCloudStorage *cloud_storage;
	qPrefDivePlanner *planner_settings;
	qPrefUnits *unit_settings;
	GeneralSettingsObjectWrapper *general_settings;
	qPrefDisplay *display_settings;
	LanguageSettingsObjectWrapper *language_settings;
	qPrefAnimations *animation_settings;
	LocationServiceSettingsObjectWrapper *location_settings;
	UpdateManagerSettings *update_manager_settings;
	qPrefDiveComputer *dive_computer_settings;

	void sync();
	void load();
private:
	SettingsObjectWrapper(QObject *parent = NULL);
};

#endif
