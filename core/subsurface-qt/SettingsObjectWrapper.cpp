// SPDX-License-Identifier: GPL-2.0
#include "SettingsObjectWrapper.h"
#include <QSettings>
#include <QApplication>
#include <QFont>
#include <QDate>

#include "core/qthelper.h"
#include "core/prefs-macros.h"

PartialPressureGasSettings::PartialPressureGasSettings(QObject* parent):
	QObject(parent)
{

}

bool PartialPressureGasSettings::showPo2() const
{
	return prefs.pp_graphs.po2;
}

bool PartialPressureGasSettings::showPn2() const
{
	return prefs.pp_graphs.pn2;
}

bool PartialPressureGasSettings::showPhe() const
{
	return prefs.pp_graphs.phe;
}

double PartialPressureGasSettings::po2ThresholdMin() const
{
	return prefs.pp_graphs.po2_threshold_min;
}

double PartialPressureGasSettings::po2ThresholdMax() const
{
	return prefs.pp_graphs.po2_threshold_max;
}


double PartialPressureGasSettings::pn2Threshold() const
{
	return prefs.pp_graphs.pn2_threshold;
}

double PartialPressureGasSettings::pheThreshold() const
{
	return prefs.pp_graphs.phe_threshold;
}

void PartialPressureGasSettings::setShowPo2(bool value)
{
	if (value == prefs.pp_graphs.po2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("po2graph", value);
	prefs.pp_graphs.po2 = value;
	emit showPo2Changed(value);
}

void PartialPressureGasSettings::setShowPn2(bool value)
{
	if (value == prefs.pp_graphs.pn2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("pn2graph", value);
	prefs.pp_graphs.pn2 = value;
	emit showPn2Changed(value);
}

void PartialPressureGasSettings::setShowPhe(bool value)
{
	if (value == prefs.pp_graphs.phe)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("phegraph", value);
	prefs.pp_graphs.phe = value;
	emit showPheChanged(value);
}

void PartialPressureGasSettings::setPo2ThresholdMin(double value)
{
	if (value == prefs.pp_graphs.po2_threshold_min)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("po2thresholdmin", value);
	prefs.pp_graphs.po2_threshold_min = value;
	emit po2ThresholdMinChanged(value);
}

void PartialPressureGasSettings::setPo2ThresholdMax(double value)
{
	if (value == prefs.pp_graphs.po2_threshold_max)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("po2thresholdmax", value);
	prefs.pp_graphs.po2_threshold_max = value;
	emit po2ThresholdMaxChanged(value);
}

void PartialPressureGasSettings::setPn2Threshold(double value)
{
	if (value == prefs.pp_graphs.pn2_threshold)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("pn2threshold", value);
	prefs.pp_graphs.pn2_threshold = value;
	emit pn2ThresholdChanged(value);
}

void PartialPressureGasSettings::setPheThreshold(double value)
{
	if (value == prefs.pp_graphs.phe_threshold)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("phethreshold", value);
	prefs.pp_graphs.phe_threshold = value;
	emit pheThresholdChanged(value);
}


TechnicalDetailsSettings::TechnicalDetailsSettings(QObject* parent): QObject(parent)
{

}

deco_mode TechnicalDetailsSettings::deco() const
{

	return prefs.display_deco_mode;
}

void TechnicalDetailsSettings::setDecoMode(deco_mode d)
{
	if (prefs.display_deco_mode == d)
		return;

	prefs.display_deco_mode = d;
	QSettings s;
	s.beginGroup(group);
	s.setValue("display_deco_mode", d);
	emit decoModeChanged(d);
}

double TechnicalDetailsSettings:: modpO2() const
{
	return prefs.modpO2;
}

bool TechnicalDetailsSettings::ead() const
{
	return prefs.ead;
}

bool TechnicalDetailsSettings::dcceiling() const
{
	return prefs.dcceiling;
}

bool TechnicalDetailsSettings::redceiling() const
{
	return prefs.redceiling;
}

bool TechnicalDetailsSettings::calcceiling() const
{
	return prefs.calcceiling;
}

bool TechnicalDetailsSettings::calcceiling3m() const
{
	return prefs.calcceiling3m;
}

bool TechnicalDetailsSettings::calcalltissues() const
{
	return prefs.calcalltissues;
}

bool TechnicalDetailsSettings::calcndltts() const
{
	return prefs.calcndltts;
}

int TechnicalDetailsSettings::gflow() const
{
	return prefs.gflow;
}

int TechnicalDetailsSettings::gfhigh() const
{
	return prefs.gfhigh;
}

short TechnicalDetailsSettings::vpmbConservatism() const
{
	return prefs.vpmb_conservatism;
}

bool TechnicalDetailsSettings::hrgraph() const
{
	return prefs.hrgraph;
}

bool TechnicalDetailsSettings::tankBar() const
{
	return prefs.tankbar;
}

bool TechnicalDetailsSettings::percentageGraph() const
{
	return prefs.percentagegraph;
}

bool TechnicalDetailsSettings::rulerGraph() const
{
	return prefs.rulergraph;
}

bool TechnicalDetailsSettings::showSCROCpO2() const
{
	return prefs.show_scr_ocpo2;
}

bool TechnicalDetailsSettings::showCCRSetpoint() const
{
	return prefs.show_ccr_setpoint;
}

bool TechnicalDetailsSettings::showCCRSensors() const
{
	return prefs.show_ccr_sensors;
}

bool TechnicalDetailsSettings::zoomedPlot() const
{
	return prefs.zoomed_plot;
}

bool TechnicalDetailsSettings::showSac() const
{
	return prefs.show_sac;
}

bool TechnicalDetailsSettings::displayUnusedTanks() const
{
	return prefs.display_unused_tanks;
}

bool TechnicalDetailsSettings::showAverageDepth() const
{
	return prefs.show_average_depth;
}

bool TechnicalDetailsSettings::showIcd() const
{
	return prefs.show_icd;
}

bool TechnicalDetailsSettings::mod() const
{
	return prefs.mod;
}

bool TechnicalDetailsSettings::showPicturesInProfile() const
{
	return prefs.show_pictures_in_profile;
}

void TechnicalDetailsSettings::setModpO2(double value)
{
	if (value == prefs.modpO2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("modpO2", value);
	prefs.modpO2 = value;
	emit modpO2Changed(value);
}

void TechnicalDetailsSettings::setShowPicturesInProfile(bool value)
{
	if (value == prefs.show_pictures_in_profile)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("show_pictures_in_profile", value);
	prefs.show_pictures_in_profile = value;
	emit showPicturesInProfileChanged(value);
}

void TechnicalDetailsSettings::setEad(bool value)
{
	if (value == prefs.ead)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("ead", value);
	prefs.ead = value;
	emit eadChanged(value);
}

void TechnicalDetailsSettings::setMod(bool value)
{
	if (value == prefs.mod)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("mod", value);
	prefs.mod = value;
	emit modChanged(value);
}

void TechnicalDetailsSettings::setDCceiling(bool value)
{
	if (value == prefs.dcceiling)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("dcceiling", value);
	prefs.dcceiling = value;
	emit dcceilingChanged(value);
}

void TechnicalDetailsSettings::setRedceiling(bool value)
{
	if (value == prefs.redceiling)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("redceiling", value);
	prefs.redceiling = value;
	emit redceilingChanged(value);
}

void TechnicalDetailsSettings::setCalcceiling(bool value)
{
	if (value == prefs.calcceiling)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcceiling", value);
	prefs.calcceiling = value;
	emit calcceilingChanged(value);
}

void TechnicalDetailsSettings::setCalcceiling3m(bool value)
{
	if (value == prefs.calcceiling3m)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcceiling3m", value);
	prefs.calcceiling3m = value;
	emit calcceiling3mChanged(value);
}

void TechnicalDetailsSettings::setCalcalltissues(bool value)
{
	if (value == prefs.calcalltissues)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcalltissues", value);
	prefs.calcalltissues = value;
	emit calcalltissuesChanged(value);
}

void TechnicalDetailsSettings::setCalcndltts(bool value)
{
	if (value == prefs.calcndltts)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("calcndltts", value);
	prefs.calcndltts = value;
	emit calcndlttsChanged(value);
}

void TechnicalDetailsSettings::setGflow(int value)
{
	if (value == prefs.gflow)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("gflow", value);
	prefs.gflow = value;
	set_gf(prefs.gflow, prefs.gfhigh);
	emit gflowChanged(value);
}

void TechnicalDetailsSettings::setGfhigh(int value)
{
	if (value == prefs.gfhigh)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("gfhigh", value);
	prefs.gfhigh = value;
	set_gf(prefs.gflow, prefs.gfhigh);
	emit gfhighChanged(value);
}

void TechnicalDetailsSettings::setVpmbConservatism(short value)
{
	if (value == prefs.vpmb_conservatism)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("vpmb_conservatism", value);
	prefs.vpmb_conservatism = value;
	set_vpmb_conservatism(value);
	emit vpmbConservatismChanged(value);
}

void TechnicalDetailsSettings::setHRgraph(bool value)
{
	if (value == prefs.hrgraph)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("hrgraph", value);
	prefs.hrgraph = value;
	emit hrgraphChanged(value);
}

void TechnicalDetailsSettings::setTankBar(bool value)
{
	if (value == prefs.tankbar)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("tankbar", value);
	prefs.tankbar = value;
	emit tankBarChanged(value);
}

void TechnicalDetailsSettings::setPercentageGraph(bool value)
{
	if (value == prefs.percentagegraph)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("percentagegraph", value);
	prefs.percentagegraph = value;
	emit percentageGraphChanged(value);
}

void TechnicalDetailsSettings::setRulerGraph(bool value)
{
	if (value == prefs.rulergraph)
		return;
	/* TODO: search for the QSettings of the RulerBar */
	QSettings s;
	s.beginGroup(group);
	s.setValue("RulerBar", value);
	prefs.rulergraph = value;
	emit rulerGraphChanged(value);
}

void TechnicalDetailsSettings::setShowCCRSetpoint(bool value)
{
	if (value == prefs.show_ccr_setpoint)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("show_ccr_setpoint", value);
	prefs.show_ccr_setpoint = value;
	emit showCCRSetpointChanged(value);
}

void TechnicalDetailsSettings::setShowSCROCpO2(bool value)
{
	if (value == prefs.show_scr_ocpo2)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("show_scr_ocpo2", value);
	prefs.show_scr_ocpo2 = value;
	emit showSCROCpO2Changed(value);
}

void TechnicalDetailsSettings::setShowCCRSensors(bool value)
{
	if (value == prefs.show_ccr_sensors)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_ccr_sensors", value);
	prefs.show_ccr_sensors = value;
	emit showCCRSensorsChanged(value);
}

void TechnicalDetailsSettings::setZoomedPlot(bool value)
{
	if (value == prefs.zoomed_plot)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("zoomed_plot", value);
	prefs.zoomed_plot = value;
	emit zoomedPlotChanged(value);
}

void TechnicalDetailsSettings::setShowSac(bool value)
{
	if (value == prefs.show_sac)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_sac", value);
	prefs.show_sac = value;
	emit showSacChanged(value);
}

void TechnicalDetailsSettings::setDisplayUnusedTanks(bool value)
{
	if (value == prefs.display_unused_tanks)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("display_unused_tanks", value);
	prefs.display_unused_tanks = value;
	emit displayUnusedTanksChanged(value);
}

void TechnicalDetailsSettings::setShowAverageDepth(bool value)
{
	if (value == prefs.show_average_depth)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_average_depth", value);
	prefs.show_average_depth = value;
	emit showAverageDepthChanged(value);
}

void TechnicalDetailsSettings::setShowIcd(bool value)
{
	if (value == prefs.show_icd)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("show_icd", value);
	prefs.show_icd = value;
	emit showIcdChanged(value);
}

GeocodingPreferences::GeocodingPreferences(QObject *parent) :
	QObject(parent)
{

}

taxonomy_category GeocodingPreferences::firstTaxonomyCategory() const
{
	return prefs.geocoding.category[0];
}

taxonomy_category GeocodingPreferences::secondTaxonomyCategory() const
{
	return prefs.geocoding.category[1];
}

taxonomy_category GeocodingPreferences::thirdTaxonomyCategory() const
{
	return prefs.geocoding.category[2];
}

void GeocodingPreferences::setFirstTaxonomyCategory(taxonomy_category value)
{
	if (value == prefs.geocoding.category[0])
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cat0", value);
	prefs.geocoding.category[0] = value;
	emit firstTaxonomyCategoryChanged(value);
}

void GeocodingPreferences::setSecondTaxonomyCategory(taxonomy_category value)
{
	if (value == prefs.geocoding.category[1])
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cat1", value);
	prefs.geocoding.category[1]= value;
	emit secondTaxonomyCategoryChanged(value);
}

void GeocodingPreferences::setThirdTaxonomyCategory(taxonomy_category value)
{
	if (value == prefs.geocoding.category[2])
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("cat2", value);
	prefs.geocoding.category[2] = value;
	emit thirdTaxonomyCategoryChanged(value);
}

GeneralSettingsObjectWrapper::GeneralSettingsObjectWrapper(QObject *parent) :
	QObject(parent)
{
}

QString GeneralSettingsObjectWrapper::defaultFilename() const
{
	return prefs.default_filename;
}

QString GeneralSettingsObjectWrapper::defaultCylinder() const
{
	return prefs.default_cylinder;
}

short GeneralSettingsObjectWrapper::defaultFileBehavior() const
{
	return prefs.default_file_behavior;
}

bool GeneralSettingsObjectWrapper::useDefaultFile() const
{
	return prefs.use_default_file;
}

int GeneralSettingsObjectWrapper::defaultSetPoint() const
{
	return prefs.defaultsetpoint;
}

int GeneralSettingsObjectWrapper::o2Consumption() const
{
	return prefs.o2consumption;
}

int GeneralSettingsObjectWrapper::pscrRatio() const
{
	return prefs.pscr_ratio;
}

bool GeneralSettingsObjectWrapper::autoRecalculateThumbnails() const
{
	return prefs.auto_recalculate_thumbnails;
}

bool GeneralSettingsObjectWrapper::extractVideoThumbnails() const
{
	return prefs.extract_video_thumbnails;
}

int GeneralSettingsObjectWrapper::extractVideoThumbnailsPosition() const
{
	return prefs.extract_video_thumbnails_position;
}

QString GeneralSettingsObjectWrapper::ffmpegExecutable() const
{
	return prefs.ffmpeg_executable;
}

void GeneralSettingsObjectWrapper::setDefaultFilename(const QString& value)
{
	if (value == prefs.default_filename)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("default_filename", value);
	free((void *)prefs.default_filename);
	prefs.default_filename = copy_qstring(value);
	emit defaultFilenameChanged(value);
}

void GeneralSettingsObjectWrapper::setDefaultCylinder(const QString& value)
{
	if (value == prefs.default_cylinder)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("default_cylinder", value);
	free((void *)prefs.default_cylinder);
	prefs.default_cylinder = copy_qstring(value);
	emit defaultCylinderChanged(value);
}

void GeneralSettingsObjectWrapper::setDefaultFileBehavior(short value)
{
	if (value == prefs.default_file_behavior && prefs.default_file_behavior != UNDEFINED_DEFAULT_FILE)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("default_file_behavior", value);

	prefs.default_file_behavior = value;
	if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		if (QString(prefs.default_filename).isEmpty())
			prefs.default_file_behavior = NO_DEFAULT_FILE;
		else
			prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
	}
	emit defaultFileBehaviorChanged(value);
}

void GeneralSettingsObjectWrapper::setUseDefaultFile(bool value)
{
	if (value == prefs.use_default_file)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("use_default_file", value);
	prefs.use_default_file = value;
	emit useDefaultFileChanged(value);
}

void GeneralSettingsObjectWrapper::setDefaultSetPoint(int value)
{
	if (value == prefs.defaultsetpoint)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("defaultsetpoint", value);
	prefs.defaultsetpoint = value;
	emit defaultSetPointChanged(value);
}

void GeneralSettingsObjectWrapper::setO2Consumption(int value)
{
	if (value == prefs.o2consumption)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("o2consumption", value);
	prefs.o2consumption = value;
	emit o2ConsumptionChanged(value);
}

void GeneralSettingsObjectWrapper::setPscrRatio(int value)
{
	if (value == prefs.pscr_ratio)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("pscr_ratio", value);
	prefs.pscr_ratio = value;
	emit pscrRatioChanged(value);
}

void GeneralSettingsObjectWrapper::setAutoRecalculateThumbnails(bool value)
{
	if (value == prefs.auto_recalculate_thumbnails)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("auto_recalculate_thumbnails", value);
	prefs.auto_recalculate_thumbnails = value;
	emit autoRecalculateThumbnailsChanged(value);
}

void GeneralSettingsObjectWrapper::setExtractVideoThumbnails(bool value)
{
	if (value == prefs.extract_video_thumbnails)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("extract_video_thumbnails", value);
	prefs.extract_video_thumbnails = value;
	emit extractVideoThumbnailsChanged(value);
}

void GeneralSettingsObjectWrapper::setExtractVideoThumbnailsPosition(int value)
{
	if (value == prefs.extract_video_thumbnails_position)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("extract_video_thumbnails_position", value);
	prefs.extract_video_thumbnails_position = value;
	emit extractVideoThumbnailsPositionChanged(value);
}

void GeneralSettingsObjectWrapper::setFfmpegExecutable(const QString &value)
{
	if (value == prefs.ffmpeg_executable)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("ffmpeg_executable", value);
	free((void *)prefs.ffmpeg_executable);
	prefs.ffmpeg_executable = copy_qstring(value);
	emit ffmpegExecutableChanged(value);
}

LanguageSettingsObjectWrapper::LanguageSettingsObjectWrapper(QObject *parent) :
	QObject(parent)
{
}

QString LanguageSettingsObjectWrapper::language() const
{
	return prefs.locale.language;
}

QString LanguageSettingsObjectWrapper::timeFormat() const
{
	return prefs.time_format;
}

QString LanguageSettingsObjectWrapper::dateFormat() const
{
	return prefs.date_format;
}

QString LanguageSettingsObjectWrapper::dateFormatShort() const
{
	return prefs.date_format_short;
}

bool LanguageSettingsObjectWrapper::timeFormatOverride() const
{
	return prefs.time_format_override;
}

bool LanguageSettingsObjectWrapper::dateFormatOverride() const
{
	return prefs.date_format_override;
}

bool LanguageSettingsObjectWrapper::useSystemLanguage() const
{
	return prefs.locale.use_system_language;
}

QString LanguageSettingsObjectWrapper::langLocale() const
{
	return prefs.locale.lang_locale;
}

void LanguageSettingsObjectWrapper::setUseSystemLanguage(bool value)
{
	if (value == prefs.locale.use_system_language)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("UseSystemLanguage", value);
	prefs.locale.use_system_language = value;
	emit useSystemLanguageChanged(value);
}

void  LanguageSettingsObjectWrapper::setLangLocale(const QString &value)
{
	if (value == prefs.locale.lang_locale)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("UiLangLocale", value);
	free((void *)prefs.locale.lang_locale);
	prefs.locale.lang_locale = copy_qstring(value);
	emit langLocaleChanged(value);
}

void  LanguageSettingsObjectWrapper::setLanguage(const QString& value)
{
	if (value == prefs.locale.language)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("UiLanguage", value);
	free((void *)prefs.locale.language);
	prefs.locale.language = copy_qstring(value);
	emit languageChanged(value);
}

void  LanguageSettingsObjectWrapper::setTimeFormat(const QString& value)
{
	if (value == prefs.time_format)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("time_format", value);
	free((void *)prefs.time_format);
	prefs.time_format = copy_qstring(value);
	emit timeFormatChanged(value);
}

void  LanguageSettingsObjectWrapper::setDateFormat(const QString& value)
{
	if (value == prefs.date_format)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("date_format", value);
	free((void *)prefs.date_format);
	prefs.date_format = copy_qstring(value);
	emit dateFormatChanged(value);
}

void  LanguageSettingsObjectWrapper::setDateFormatShort(const QString& value)
{
	if (value == prefs.date_format_short)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("date_format_short", value);
	free((void *)prefs.date_format_short);
	prefs.date_format_short = copy_qstring(value);
	emit dateFormatShortChanged(value);
}

void  LanguageSettingsObjectWrapper::setTimeFormatOverride(bool value)
{
	if (value == prefs.time_format_override)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("time_format_override", value);
	prefs.time_format_override = value;
	emit timeFormatOverrideChanged(value);
}

void  LanguageSettingsObjectWrapper::setDateFormatOverride(bool value)
{
	if (value == prefs.date_format_override)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("date_format_override", value);
	prefs.date_format_override = value;
	emit dateFormatOverrideChanged(value);
}


LocationServiceSettingsObjectWrapper::LocationServiceSettingsObjectWrapper(QObject* parent):
	QObject(parent)
{
}

int LocationServiceSettingsObjectWrapper::distanceThreshold() const
{
	return prefs.distance_threshold;
}

int LocationServiceSettingsObjectWrapper::timeThreshold() const
{
	return prefs.time_threshold;
}

void LocationServiceSettingsObjectWrapper::setDistanceThreshold(int value)
{
	if (value == prefs.distance_threshold)
		return;
	QSettings s;
	s.beginGroup(group);
	s.setValue("distance_threshold", value);
	prefs.distance_threshold = value;
	emit distanceThresholdChanged(value);
}

void LocationServiceSettingsObjectWrapper::setTimeThreshold(int value)
{
	if (value == prefs.time_threshold)
		return;

	QSettings s;
	s.beginGroup(group);
	s.setValue("time_threshold", value);
	prefs.time_threshold = value;
	emit timeThresholdChanged(value);
}

SettingsObjectWrapper::SettingsObjectWrapper(QObject* parent):
QObject(parent),
	techDetails(new TechnicalDetailsSettings(this)),
	pp_gas(new PartialPressureGasSettings(this)),
	facebook(new qPrefFacebook(this)),
	geocoding(new GeocodingPreferences(this)),
	proxy(new qPrefProxy(this)),
	cloud_storage(new qPrefCloudStorage(this)),
	planner_settings(new qPrefDivePlanner(this)),
	unit_settings(new qPrefUnits(this)),
	general_settings(new GeneralSettingsObjectWrapper(this)),
	display_settings(new qPrefDisplay(this)),
	language_settings(new LanguageSettingsObjectWrapper(this)),
	animation_settings(new qPrefAnimations(this)),
	location_settings(new LocationServiceSettingsObjectWrapper(this)),
	update_manager_settings(new qPrefUpdateManager(this)),
	dive_computer_settings(new qPrefDiveComputer(this))
{
}

void SettingsObjectWrapper::load()
{
	QSettings s;
	QVariant v;

	uiLanguage(NULL);

	qPrefUnits::instance()->load();

	s.beginGroup("TecDetails");
	GET_BOOL("po2graph", pp_graphs.po2);
	GET_BOOL("pn2graph", pp_graphs.pn2);
	GET_BOOL("phegraph", pp_graphs.phe);
	GET_DOUBLE("po2thresholdmin", pp_graphs.po2_threshold_min);
	GET_DOUBLE("po2thresholdmax", pp_graphs.po2_threshold_max);
	GET_DOUBLE("pn2threshold", pp_graphs.pn2_threshold);
	GET_DOUBLE("phethreshold", pp_graphs.phe_threshold);
	GET_BOOL("mod", mod);
	GET_DOUBLE("modpO2", modpO2);
	GET_BOOL("ead", ead);
	GET_BOOL("redceiling", redceiling);
	GET_BOOL("dcceiling", dcceiling);
	GET_BOOL("calcceiling", calcceiling);
	GET_BOOL("calcceiling3m", calcceiling3m);
	GET_BOOL("calcndltts", calcndltts);
	GET_BOOL("calcalltissues", calcalltissues);
	GET_BOOL("hrgraph", hrgraph);
	GET_BOOL("tankbar", tankbar);
	GET_BOOL("RulerBar", rulergraph);
	GET_BOOL("percentagegraph", percentagegraph);
	GET_INT("gflow", gflow);
	GET_INT("gfhigh", gfhigh);
	GET_INT("vpmb_conservatism", vpmb_conservatism);
	GET_BOOL("gf_low_at_maxdepth", gf_low_at_maxdepth);
	GET_BOOL("show_ccr_setpoint",show_ccr_setpoint);
	GET_BOOL("show_ccr_sensors",show_ccr_sensors);
	GET_BOOL("show_scr_ocpo2",show_scr_ocpo2);
	GET_BOOL("zoomed_plot", zoomed_plot);
	set_gf(prefs.gflow, prefs.gfhigh);
	set_vpmb_conservatism(prefs.vpmb_conservatism);
	GET_BOOL("show_sac", show_sac);
	GET_BOOL("display_unused_tanks", display_unused_tanks);
	GET_BOOL("show_average_depth", show_average_depth);
	GET_BOOL("show_icd", show_icd);
	GET_BOOL("show_pictures_in_profile", show_pictures_in_profile);
	prefs.display_deco_mode =  (deco_mode) s.value("display_deco_mode").toInt();
	s.endGroup();

	s.beginGroup("GeneralSettings");
	GET_TXT("default_filename", default_filename);
	GET_INT("default_file_behavior", default_file_behavior);
	if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		if (QString(prefs.default_filename).isEmpty())
			prefs.default_file_behavior = NO_DEFAULT_FILE;
		else
			prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
	}
	GET_TXT("default_cylinder", default_cylinder);
	GET_BOOL("use_default_file", use_default_file);
	GET_INT("defaultsetpoint", defaultsetpoint);
	GET_INT("o2consumption", o2consumption);
	GET_INT("pscr_ratio", pscr_ratio);
	GET_BOOL("auto_recalculate_thumbnails", auto_recalculate_thumbnails);
	s.endGroup();

	qPrefAnimations::instance()->load();
	qPrefCloudStorage::instance()->load();
	qPrefDisplay::instance()->load();
	qPrefProxy::instance()->load();

	// GeoManagement
	s.beginGroup("geocoding");

	GET_ENUM("cat0", taxonomy_category, geocoding.category[0]);
	GET_ENUM("cat1", taxonomy_category, geocoding.category[1]);
	GET_ENUM("cat2", taxonomy_category, geocoding.category[2]);
	s.endGroup();

	// GPS service time and distance thresholds
	s.beginGroup("LocationService");
	GET_INT("time_threshold", time_threshold);
	GET_INT("distance_threshold", distance_threshold);
	s.endGroup();

	qPrefDivePlanner::instance()->load();
	qPrefDiveComputer::instance()->load();
	qPrefUpdateManager::instance()->load();

	s.beginGroup("Language");
	GET_BOOL("UseSystemLanguage", locale.use_system_language);
	GET_TXT("UiLanguage", locale.language);
	GET_TXT("UiLangLocale", locale.lang_locale);
	GET_TXT("time_format", time_format);
	GET_TXT("date_format", date_format);
	GET_TXT("date_format_short", date_format_short);
	GET_BOOL("time_format_override", time_format_override);
	GET_BOOL("date_format_override", date_format_override);
	s.endGroup();
}

void SettingsObjectWrapper::sync()
{
	qPrefDisplay::instance()->sync();
}

SettingsObjectWrapper* SettingsObjectWrapper::instance()
{
	static SettingsObjectWrapper settings;
	return &settings;
}
