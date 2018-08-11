// SPDX-License-Identifier: GPL-2.0
#include "SettingsObjectWrapper.h"
#include <QSettings>
#include <QApplication>
#include <QFont>
#include <QDate>

#include "core/qthelper.h"
#include "core/prefs-macros.h"


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

SettingsObjectWrapper::SettingsObjectWrapper(QObject* parent):
QObject(parent),
	techDetails(new qPrefTechnicalDetails(this)),
	pp_gas(new qPrefPartialPressureGas(this)),
	facebook(new qPrefFacebook(this)),
	geocoding(new qPrefGeocoding(this)),
	proxy(new qPrefProxy(this)),
	cloud_storage(new qPrefCloudStorage(this)),
	planner_settings(new qPrefDivePlanner(this)),
	unit_settings(new qPrefUnits(this)),
	general_settings(new GeneralSettingsObjectWrapper(this)),
	display_settings(new qPrefDisplay(this)),
	language_settings(new qPrefLanguage(this)),
	animation_settings(new qPrefAnimations(this)),
	location_settings(new qPrefLocationService(this)),
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
	qPrefPartialPressureGas::instance()->load();

	s.beginGroup("TecDetails");
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
	qPrefGeocoding::instance()->load();

	// GPS service time and distance thresholds
	qPrefLocationService::instance()->load();

	qPrefDivePlanner::instance()->load();
	qPrefDiveComputer::instance()->load();
	qPrefUpdateManager::instance()->load();

	qPrefLanguage::instance()->load();
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
